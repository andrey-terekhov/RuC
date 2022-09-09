/*
 *	Copyright 2021 Andrey Terekhov, Ilya Andreev
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include "parser.h"
#include <stdbool.h>
#include "builder.h"
#include "lexer.h"
#include "writer.h"


static const char *const DEFAULT_TREE = "tree.txt";


/** Parser */
typedef struct parser
{
	syntax *sx;							/**< Syntax structure */

	builder bld;						/**< AST builder */
	lexer lxr;							/**< Lexer */
	token tk;							/**< Current 'peek token' */

	size_t array_dimensions;			/**< Array dimensions counter */

	int func_def;						/**< @c 0 for function without arguments,
											@c 1 for function definition,
											@c 2 for function declaration,
											@c 3 for others */

	int flag_empty_bounds;				/**< Set, if array declaration has empty bounds */

	bool is_in_switch;					/**< Set, if parser is in switch body */
	bool is_in_loop;					/**< Set, if parser is in loop body */

	bool was_return;					/**< Set, if was return in parsed function */
	bool was_type_def;					/**< Set, if was type definition */
} parser;


static item_t parse_struct_or_union_specifier(parser *const prs, node *const parent);
static item_t parse_struct_declaration_list(parser *const prs, node *const parent, const size_t repr);
static item_t parse_enum_specifier(parser *const prs, node *const parent);
static location consume_token(parser *const prs);
static node parse_expression(parser *const prs);
static node parse_initializer(parser *const prs);
static node parse_statement(parser *const prs);


/*
 *	 __  __     ______   __     __         ______
 *	/\ \/\ \   /\__  _\ /\ \   /\ \       /\  ___\
 *	\ \ \_\ \  \/_/\ \/ \ \ \  \ \ \____  \ \___  \
 *	 \ \_____\    \ \_\  \ \_\  \ \_____\  \/\_____\
 *	  \/_____/     \/_/   \/_/   \/_____/   \/_____/
 */


/**
 *	Create parser
 *
 *	@param	sx			Syntax structure
 *
 *	@return	Parser
 */
static inline parser parser_create(syntax *const sx)
{
	parser prs;
	prs.sx = sx;
	prs.bld = builder_create(sx);
	prs.lxr = lexer_create(sx);

	prs.is_in_loop = false;
	prs.is_in_switch = false;

	consume_token(&prs);

	return prs;
}

/**
 *	Free allocated memory
 *
 *	@param	prs			Parser
 */
static inline void parser_clear(parser *const prs)
{
	lexer_clear(&prs->lxr);
}

/**
 *	Emit a syntax error from parser
 *
 *	@param	prs			Parser
 *	@param	num			Error code
 */
static void parser_error(parser *const prs, err_t num, ...)
{
	const location loc = token_get_location(&prs->tk);

	va_list args;
	va_start(args, num);

	report_error(&prs->sx->rprt, prs->sx->io, loc, num, args);

	va_end(args);
}

/**
 *	Consume the current 'peek token' and lex the next one
 *
 *	@param	prs			Parser
 *
 *	@return	Location of consumed token
 */
static location consume_token(parser *const prs)
{
	location prev_loc = token_get_location(&prs->tk);
	prs->tk = lex(&prs->lxr);
	return prev_loc;
}

/**
 *	Peek ahead token without consuming it
 *
 *	@param	prs			Parser
 *
 *	@return	Peeked token kind
 */
static inline token_t peek_token(parser *const prs)
{
	return peek(&prs->lxr);
}

/**
 *	Consume the current 'peek token' if it is expected
 *
 *	@param	prs			Parser
 *	@param	expected	Expected token kind
 *
 *	@return	@c 1 on consuming 'peek token', @c 0 on otherwise
 */
static bool try_consume_token(parser *const prs, const token_t expected)
{
	if (token_is(&prs->tk, expected))
	{
		consume_token(prs);
		return true;
	}

	return false;
}

/**
 *	Try to consume the current 'peek token'
 *	If 'peek token' is expected, consume it, otherwise emit an error
 *
 *	@param	prs			Parser
 *	@param	expected	Expected token kind
 *	@param	err			Error to emit
 */
static void expect_and_consume(parser *const prs, const token_t expected, const err_t err)
{
	if (try_consume_token(prs, expected))
	{
		return;
	}

	if (expected == TK_SEMICOLON && peek_token(prs) == TK_SEMICOLON)
	{
		const token_t curr_token = token_get_kind(&prs->tk);
		if (curr_token == TK_R_PAREN || curr_token == TK_R_SQUARE)
		{
			parser_error(prs, extraneous_bracket_before_semi);
			consume_token(prs);	// ')' or ']'
			consume_token(prs);	// ';'
			return;
		}
	}

	parser_error(prs, err);
}

/** Check if the set of tokens has token in it */
static inline bool has_token_set(const uint8_t tokens, const token_t token)
{
	return (tokens & token) != 0;
}

/**
 *	Read tokens until one of the specified tokens
 *
 *	@param	prs			Parser
 *	@param	tokens		Set of specified token kinds
 */
static void skip_until(parser *const prs, const uint8_t tokens)
{
	while (token_is_not(&prs->tk, TK_EOF))
	{
		switch (token_get_kind(&prs->tk))
		{
			case TK_L_PAREN:
				consume_token(prs);
				skip_until(prs, TK_R_PAREN);
				break;

			case TK_L_SQUARE:
				consume_token(prs);
				skip_until(prs, TK_R_SQUARE);
				break;

			case TK_L_BRACE:
				consume_token(prs);
				skip_until(prs, TK_R_BRACE);
				break;

			case TK_QUESTION:
				consume_token(prs);
				skip_until(prs, TK_COLON);
				break;

			case TK_R_PAREN:
			case TK_R_SQUARE:
			case TK_R_BRACE:
			case TK_COLON:
			case TK_SEMICOLON:
				if (has_token_set(tokens, token_get_kind(&prs->tk)))
				{
					return;
				}

			default:
				consume_token(prs);
				break;
		}
	}
}

/**
 *	Add new item to identifiers table
 *
 *	@param	prs			Parser
 *	@param	repr		New identifier index in representations table
 *	@param	type		@c -1 for function as parameter,
 *						@c  0 for variable,
 *						@c  1 for label,
 *						@c  funcnum for function,
 *						@c  >= @c 100 for type specifier
 *	@param	mode		New identifier mode
 *
 *	@return	Index of the last item in identifiers table
 */
static size_t to_identab(parser *const prs, const size_t repr, const item_t type, const item_t mode)
{
	const size_t ret = ident_add(prs->sx, repr, type, mode, prs->func_def);

	if (ret == SIZE_MAX)
	{
		parser_error(prs, redefinition_of_main);
	}
	else if (ret == SIZE_MAX - 1)
	{
		parser_error(prs, repeated_decl, repr_get_name(prs->sx, repr));
	}

	return ret;
}


/*
 *	 ______     __  __     ______   ______     ______     ______     ______     __     ______     __   __     ______
 *	/\  ___\   /\_\_\_\   /\  == \ /\  == \   /\  ___\   /\  ___\   /\  ___\   /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \  __\   \/_/\_\/_  \ \  _-/ \ \  __<   \ \  __\   \ \___  \  \ \___  \  \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \_____\   /\_\/\_\  \ \_\    \ \_\ \_\  \ \_____\  \/\_____\  \/\_____\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/_____/   \/_/\/_/   \/_/     \/_/ /_/   \/_____/   \/_____/   \/_____/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */


/**
 *	Parse primary expression
 *
 *	primary-expression:
 *		identifier
 *		literal
 *		'NULL'
 *		'(' expression ')'
 *
 *	@param	prs			Parser
 *
 *	@return	Primary expression
 */
static node parse_primary_expression(parser *const prs)
{
	switch (token_get_kind(&prs->tk))
	{
		case TK_IDENTIFIER:
		{
			const size_t name = token_get_ident_name(&prs->tk);
			const location loc = consume_token(prs);

			return build_identifier_expression(&prs->bld, name, loc);
		}

		case TK_CHAR_LITERAL:
		{
			const char32_t value = token_get_char_value(&prs->tk);
			const location loc = consume_token(prs);

			return build_character_literal_expression(&prs->bld, value, loc);
		}

		case TK_INT_LITERAL:
		{
			const item_t value = (item_t)token_get_int_value(&prs->tk);
			const location loc = consume_token(prs);

			return build_integer_literal_expression(&prs->bld, value, loc);
		}

		case TK_FLOAT_LITERAL:
		{
			const double value = token_get_float_value(&prs->tk);
			const location loc = consume_token(prs);

			return build_floating_literal_expression(&prs->bld, value, loc);
		}

		case TK_STRING_LITERAL:
		{
			const size_t value = token_get_string_num(&prs->tk);
			const location loc = consume_token(prs);

			return build_string_literal_expression(&prs->bld, value, loc);
		}

		case TK_NULL:
			return build_null_literal_expression(&prs->bld, consume_token(prs));

		case TK_TRUE:
			return build_boolean_literal_expression(&prs->bld, true, consume_token(prs));

		case TK_FALSE:
			return build_boolean_literal_expression(&prs->bld, false, consume_token(prs));

		case TK_L_PAREN:
		{
			const location l_loc = consume_token(prs);
			const node subexpr = parse_expression(prs);

			if (!try_consume_token(prs, TK_R_PAREN))
			{
				parser_error(prs, expected_r_paren, l_loc);
				return node_broken();
			}

			return subexpr;
		}

		default:
			parser_error(prs, expected_expression);
			return node_broken();
	}
}

/**
 *	Parse initializer list
 *
 *	initializer-list:
 *		initializer
 *		expression-list ',' initializer
 *
 *	@param	prs			Parser
 *
 *	@return	Expression vector
 */
static node_vector parse_initializer_list(parser *const prs)
{
	node_vector result = node_vector_create();

	do
	{
		const node initializer = parse_initializer(prs);
		node_vector_add(&result, &initializer);
	} while (try_consume_token(prs, TK_COMMA));

	return result;
}

/**
 *	Parse postfix expression
 *
 *	postfix-expression:
 *		primary-expression
 *		postfix-expression '[' expression ']'
 *		postfix-expression '(' initializer-list[opt] ')'
 *		postfix-expression '.' identifier
 *		postfix-expression '->' identifier
 *		postfix-expression '++'
 *		postfix-expression '--'
 *
 *	@param	prs			Parser
 *
 *	@return	Postfix expression
 */
static node parse_postfix_expression(parser *const prs)
{
	node operand = parse_primary_expression(prs);

	while (true)
	{
		switch (token_get_kind(&prs->tk))
		{
			default:
				return operand;

			case TK_L_SQUARE:
			{
				const location l_loc = consume_token(prs);
				node index = parse_expression(prs);

				if (token_is(&prs->tk, TK_R_SQUARE))
				{
					const location r_loc = consume_token(prs);
					operand = build_subscript_expression(&prs->bld, &operand, &index, l_loc, r_loc);
				}
				else
				{
					parser_error(prs, expected_r_square, l_loc);
					skip_until(prs, TK_R_SQUARE | TK_SEMICOLON);
					try_consume_token(prs, TK_R_SQUARE);
					operand = node_broken();
				}

				continue;
			}

			case TK_L_PAREN:
			{
				const location l_loc = consume_token(prs);

				if (token_is(&prs->tk, TK_R_PAREN))
				{
					const location r_loc = consume_token(prs);
					operand = build_call_expression(&prs->bld, &operand, NULL, l_loc, r_loc);

					continue;
				}

				node_vector args = parse_initializer_list(prs);
				if (token_is(&prs->tk, TK_R_PAREN))
				{
					const location r_loc = consume_token(prs);
					operand = build_call_expression(&prs->bld, &operand, &args, l_loc, r_loc);
				}
				else
				{
					parser_error(prs, expected_r_paren, l_loc);
					skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
					try_consume_token(prs, TK_R_PAREN);
					operand = node_broken();
				}

				node_vector_clear(&args);
				continue;
			}

			case TK_PERIOD:
			case TK_ARROW:
			{
				const bool is_arrow = token_is(&prs->tk, TK_ARROW);
				const location op_loc = consume_token(prs);

				if (token_is(&prs->tk, TK_IDENTIFIER))
				{
					const size_t name = token_get_ident_name(&prs->tk);
					const location id_loc = consume_token(prs);

					operand = build_member_expression(&prs->bld, &operand, name, is_arrow, op_loc, id_loc);
				}
				else
				{
					parser_error(prs, expected_identifier_in_member_expr);
					operand = node_broken();
				}

				continue;
			}

			case TK_PLUS_PLUS:
			{
				const location op_loc = consume_token(prs);
				operand = build_unary_expression(&prs->bld, &operand, UN_POSTINC, op_loc);
				continue;
			}

			case TK_MINUS_MINUS:
			{
				const location op_loc = consume_token(prs);
				operand = build_unary_expression(&prs->bld, &operand, UN_POSTDEC, op_loc);
				continue;
			}
		}
	}
}

/**
 *	Parse unary expression
 *
 *	unary-expression:
 *		postfix-expression
 *		'++' unary-expression
 *		'--' unary-expression
 *		unary-operator unary-expression
 *
 *	unary-operator: one of
 *		'&', '*', '-', '~', '!', 'abs'
 *
 *	@param	prs			Parser
 *
 *	@return	Unary expression
 */
static node parse_unary_expression(parser *const prs)
{
	switch (token_get_kind(&prs->tk))
	{
		default:
			return parse_postfix_expression(prs);

		case TK_PLUS_PLUS:
		case TK_MINUS_MINUS:
		case TK_AMP:
		case TK_STAR:
		case TK_MINUS:
		case TK_TILDE:
		case TK_EXCLAIM:
		case TK_ABS:
		case TK_UPB:
		{
			const unary_t operator = token_to_unary(token_get_kind(&prs->tk));
			const location op_loc = consume_token(prs);
			node operand = parse_unary_expression(prs);

			return build_unary_expression(&prs->bld, &operand, operator, op_loc);
		}
	}
}

/**
 *	Parse right hand side of binary expression
 *
 *	@param	prs			Parser
 *	@param	LHS			Start of a binary expression
 *	@param	min_prec	Minimal precedence level
 *
 *	@return Binary expression
 */
static node parse_RHS_of_binary_expression(parser *const prs, node *const LHS, const precedence_t min_prec)
{
	precedence_t next_token_prec = get_operator_precedence(token_get_kind(&prs->tk));
	while (next_token_prec >= min_prec)
	{
		const token_t op_token_kind = token_get_kind(&prs->tk);
		location op_loc = consume_token(prs);

		const bool is_binary = next_token_prec != PREC_CONDITIONAL;
		node middle = node_broken();
		if (!is_binary)
		{
			middle = parse_initializer(prs);

			if (token_is(&prs->tk, TK_COLON))
			{
				op_loc = consume_token(prs);
			}
			else
			{
				parser_error(prs, expected_colon_in_conditional_expr, op_loc);
			}
		}

		node RHS = token_is(&prs->tk, TK_L_BRACE) ? parse_initializer(prs) : parse_unary_expression(prs);

		const precedence_t this_prec = next_token_prec;
		next_token_prec = get_operator_precedence(token_get_kind(&prs->tk));

		const bool is_right_associative = this_prec == PREC_CONDITIONAL || this_prec == PREC_ASSIGNMENT;
		if (this_prec < next_token_prec || (this_prec == next_token_prec && is_right_associative))
		{
			RHS = parse_RHS_of_binary_expression(prs, &RHS, (this_prec + !is_right_associative));
			next_token_prec = get_operator_precedence((token_get_kind(&prs->tk)));
		}

		if (is_binary)
		{
			const binary_t op_kind = token_to_binary(op_token_kind);
			*LHS = build_binary_expression(&prs->bld, LHS, &RHS, op_kind, op_loc);
		}
		else
		{
			*LHS = build_ternary_expression(&prs->bld, LHS, &middle, &RHS, op_loc);
		}
	}

	return *LHS;
}

/**
 *	Parse assignment expression
 *
 *	assignment-expression:
 *		conditional-expression
 *		unary-expression assignment-operator assignment-expression
 *
 *	assignment-operator: one of
 *		'=', '*=', '/=', '%=', '+=', '-=', '<<=', '>>=', '&=', 'ˆ=', '|='
 *
 *	@param	prs			Parser
 *
 *	@return	Assignment expression
 */
static node parse_assignment_expression(parser *const prs)
{
	node LHS = parse_unary_expression(prs);
	return parse_RHS_of_binary_expression(prs, &LHS, PREC_ASSIGNMENT);
}

/**
 *	Parse expression
 *
 *	expression:
 *		assignment-expression
 *		expression ',' assignment-expression
 *
 *	@param	prs			Parser
 *
 *	@return Expression
 */
static node parse_expression(parser *const prs)
{
	node LHS = parse_assignment_expression(prs);
	return parse_RHS_of_binary_expression(prs, &LHS, PREC_COMMA);
}

/**
 *	Parse constant expression
 *
 *	constant-expression:
 *		conditional-expression
 *
 *	@param	prs			Parser
 *
 *	@return	Constant expression
 */
static node parse_constant_expression(parser *const prs)
{
	node LHS = parse_unary_expression(prs);
	LHS = parse_RHS_of_binary_expression(prs, &LHS, PREC_CONDITIONAL);

	return build_constant_expression(&prs->bld, &LHS);
}

/**
 *	Parse initializer
 *
 *	initializer:
 *		assignment-expression
 *		'{' expression-list[opt] '}'
 *
 *	@param	prs			Parser
 *
 *	@return Initializer
 */
static node parse_initializer(parser *const prs)
{
	if (token_is(&prs->tk, TK_L_BRACE))
	{
		const location l_loc = consume_token(prs);

		if (try_consume_token(prs, TK_R_BRACE))
		{
			parser_error(prs, empty_initializer);
			return node_broken();
		}

		node_vector inits = parse_initializer_list(prs);
		if (token_is(&prs->tk, TK_R_BRACE))
		{
			const location r_loc = consume_token(prs);
			const node result = build_initializer(&prs->bld, &inits, l_loc, r_loc);

			node_vector_clear(&inits);
			return result;
		}
		else
		{
			parser_error(prs, expected_r_brace, l_loc);
			skip_until(prs, TK_R_BRACE | TK_SEMICOLON);
			try_consume_token(prs, TK_R_BRACE);

			node_vector_clear(&inits);
			return node_broken();
		}
	}

	return parse_assignment_expression(prs);
}

/**
 *	Parse condition
 *
 *	'(' expression ')'
 *
 *	@param	prs			Parser
 *
 *	@return Condition
 */
static node parse_condition(parser *const prs)
{
	if (token_is_not(&prs->tk, TK_L_PAREN))
	{
		parser_error(prs, expected_l_paren_in_condition);
		skip_until(prs, TK_SEMICOLON);
		return node_broken();
	}

	const location l_loc = consume_token(prs);

	node condition = parse_expression(prs);
	if (!node_is_correct(&condition))
	{
		skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
		try_consume_token(prs, TK_R_PAREN);
		return node_broken();
	}

	if (!try_consume_token(prs, TK_R_PAREN))
	{
		parser_error(prs, expected_r_paren, l_loc);
		skip_until(prs, TK_SEMICOLON);
		return node_broken();
	}

	return build_condition(&prs->bld, &condition);
}


/*
 *	 _____     ______     ______     __         ______     ______     ______     ______   __     ______     __   __     ______
 *	/\  __-.  /\  ___\   /\  ___\   /\ \       /\  __ \   /\  == \   /\  __ \   /\__  _\ /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \ \/\ \ \ \  __\   \ \ \____  \ \ \____  \ \  __ \  \ \  __<   \ \  __ \  \/_/\ \/ \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \____-  \ \_____\  \ \_____\  \ \_____\  \ \_\ \_\  \ \_\ \_\  \ \_\ \_\    \ \_\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/____/   \/_____/   \/_____/   \/_____/   \/_/\/_/   \/_/ /_/   \/_/\/_/     \/_/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */


/**
 *	Parse type specifier [C99 6.7.2]
 *
 *	type-specifier:
 *		'void'
 *		'bool'
 *		'char'
 *		'short'
 *		'int'
 *		'long'
 *		'float'
 *		'double'
 *		struct-or-union-specifier
 *		enum-specifier
 *		typedef-name
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *
 *	@return	Standard type or index of the types table
 */
static item_t parse_type_specifier(parser *const prs, node *const parent)
{
	switch (token_get_kind(&prs->tk))
	{
		case TK_VOID:
			consume_token(prs);
			return TYPE_VOID;

		case TK_BOOL:
			consume_token(prs);
			return TYPE_BOOLEAN;

		case TK_CHAR:
			consume_token(prs);
			return TYPE_CHARACTER;

		case TK_INT:
		case TK_LONG:
			consume_token(prs);
			return TYPE_INTEGER;

		case TK_FLOAT:
		case TK_DOUBLE:
			consume_token(prs);
			return TYPE_FLOATING;

		case TK_FILE:
			consume_token(prs);
			return TYPE_FILE;

		case TK_IDENTIFIER:
		{
			const size_t name = token_get_ident_name(&prs->tk);
			const item_t id = repr_get_reference(prs->sx, name);
			consume_token(prs);

			if (id == ITEM_MAX || !ident_is_type_specifier(prs->sx, (size_t)id))
			{
				parser_error(prs, ident_not_type);
				return TYPE_UNDEFINED;
			}

			return ident_get_type(prs->sx, (size_t)id);
		}

		case TK_STRUCT:
			consume_token(prs);
			return parse_struct_or_union_specifier(prs, parent);

		case TK_ENUM:
		{
			consume_token(prs);
			return parse_enum_specifier(prs, parent);
		}

		case TK_TYPEDEF:
		{
			consume_token(prs);
			item_t type = parse_type_specifier(prs, parent);
			if (type_is_undefined(type))
			{
				skip_until(prs, TK_SEMICOLON);
				return TYPE_UNDEFINED;
			}
			if (token_is(&prs->tk, TK_STAR))
			{
				consume_token(prs);
				type = type_pointer(prs->sx, type);
			}
			if (token_is(&prs->tk, TK_IDENTIFIER))
			{
				const size_t repr = token_get_ident_name(&prs->tk);
				consume_token(prs);
				to_identab(prs, repr, 1000, type);
				prs->was_type_def = true;
				if (token_is_not(&prs->tk, TK_SEMICOLON))
				{
					parser_error(prs, expected_semi_after_decl);
					return TYPE_UNDEFINED;

				}
				return type;
			}
			else
			{
				parser_error(prs, typedef_requires_a_name);
				skip_until(prs, TK_SEMICOLON);
				return TYPE_UNDEFINED;
			}
		}

		default:
			parser_error(prs, not_decl);
			return TYPE_UNDEFINED;
	}
}

/**
 *	Parse struct or union specifier [C99 6.7.2.1p1]
 *
 *	struct-or-union-specifier:
 *		struct-or-union identifier[opt] '{' struct-contents '}'
 *		struct-or-union identifier
 *
 *	struct-or-union:
 *		'struct'
 *		'union'
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *
 *	@return	Index of types table, @c type_undefined on failure
 */
static item_t parse_struct_or_union_specifier(parser *const prs, node *const parent)
{
	switch (token_get_kind(&prs->tk))
	{
		case TK_L_BRACE:
			return parse_struct_declaration_list(prs, parent, SIZE_MAX);

		case TK_IDENTIFIER:
		{
			const size_t repr = token_get_ident_name(&prs->tk);
			consume_token(prs);

			if (token_is(&prs->tk, TK_L_BRACE))
			{
				const item_t type = parse_struct_declaration_list(prs, parent, repr);
				if (type == ITEM_MAX)
				{
					return TYPE_UNDEFINED;
				}
				const item_t id = repr_get_reference(prs->sx, repr);

				prs->was_type_def = true;

				return ident_get_type(prs->sx, (size_t)id);
			}
			else // if (parser->next_token != l_brace)
			{
				const item_t id = repr_get_reference(prs->sx, repr);

				if (id == ITEM_MAX)
				{
					parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, repr));
					return TYPE_UNDEFINED;
				}

				// TODO: what if it was not a struct name?
				return ident_get_type(prs->sx, (size_t)id);
			}
		}

		default:
			parser_error(prs, wrong_struct);
			return TYPE_UNDEFINED;
	}
}

/**
 *	Parse array definition
 *
 *	direct-abstract-declarator:
 *		direct-abstract-declarator[opt] '[' constant-expression[opt] ']'
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	type		Type of variable in declaration
 *
 *	@return	Index of the types table
 */
static item_t parse_array_definition(parser *const prs, node *const parent, item_t type)
{
	prs->array_dimensions = 0;
	prs->flag_empty_bounds = 1;

	if (type_is_pointer(prs->sx, type))
	{
		parser_error(prs, pnt_before_array);
	}

	while (try_consume_token(prs, TK_L_SQUARE))
	{
		prs->array_dimensions++;
		if (try_consume_token(prs, TK_R_SQUARE))
		{
			if (token_is(&prs->tk, TK_L_SQUARE))
			{
				// int a[][] = {{ 1, 2, 3 }, { 4, 5, 6 }};	// нельзя
				parser_error(prs, empty_init);
			}
			prs->flag_empty_bounds = 0;
		}
		else
		{
			node_copy(&prs->bld.context, parent);
			const node size = parse_assignment_expression(prs);
			const item_t size_type = expression_get_type(&size);
			if (!type_is_integer(prs->sx, size_type))
			{
				parser_error(prs, array_size_must_be_int);
			}

			if (!try_consume_token(prs, TK_R_SQUARE))
			{
				parser_error(prs, wait_right_sq_br);
				skip_until(prs, TK_R_SQUARE | TK_COMMA | TK_SEMICOLON);
				try_consume_token(prs, TK_R_SQUARE);
			}
		}
		type = type_array(prs->sx, type);
	}

	return type;
}

/**
 *	Parse struct declaration list [C99 6.7.2.1p2]
 *
 *	struct-declaration-list:
 *		struct-declaration
 *		struct-declaration-list struct-declaration
 *
 *	struct-declaration:
 *		type-specifier struct-declarator-list ';'
 *
 *	struct-declarator-list:
 *		declarator
 *		struct-declarator-list ',' declarator
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	repr		Structure identifier index in representations table
 *
 *	@return	Index of types table, @c type_undefined on failure
 */
static item_t parse_struct_declaration_list(parser *const prs, node *const parent, const size_t repr)
{
	consume_token(prs);
	if (try_consume_token(prs, TK_R_BRACE))
	{
		parser_error(prs, empty_struct);
		return TYPE_UNDEFINED;
	}

	item_t local_modetab[100];
	size_t local_md = 3;
	size_t fields = 0;
	size_t displ = 0;

	node nd;
	bool created = false;

	do
	{
		item_t element_type = parse_type_specifier(prs, parent);
		if (type_is_void(element_type))
		{
			parser_error(prs, only_functions_may_have_type_VOID);
			element_type = TYPE_UNDEFINED;
		}

		item_t type = element_type;
		if (try_consume_token(prs, TK_STAR))
		{
			type = type_pointer(prs->sx, element_type);
		}

		if (!created)
		{
			nd = node_add_child(parent, OP_DECL_TYPE);
			node_add_arg(&nd, TYPE_UNDEFINED);
			node_add_arg(&nd, ITEM_MAX);
			created = true;
		}

		if (token_is(&prs->tk, TK_IDENTIFIER))
		{
			const size_t inner_repr = token_get_ident_name(&prs->tk);
			consume_token(prs);

			if (token_is(&prs->tk, TK_L_SQUARE))
			{
				node decl = node_add_child(&nd, OP_DECL_VAR);
				node_add_arg(&decl, 0);
				node_add_arg(&decl, 0);
				node_add_arg(&decl, 0);
				// Меняем тип (увеличиваем размерность массива)
				type = parse_array_definition(prs, &decl, element_type);
				node_set_arg(&decl, 0, type);
				node_set_arg(&decl, 1, (item_t)fields);
			}

			local_modetab[local_md++] = type;
			local_modetab[local_md++] = (item_t)inner_repr;
			fields++;
			displ += type_size(prs->sx, type);
		}
		else
		{
			parser_error(prs, wait_ident_after_semicolon_in_struct);
			skip_until(prs, TK_SEMICOLON | TK_R_BRACE);
		}

		expect_and_consume(prs, TK_SEMICOLON, no_semicolon_in_struct);
	} while (!try_consume_token(prs, TK_R_BRACE));

	local_modetab[0] = TYPE_STRUCTURE;
	local_modetab[1] = (item_t)displ;
	local_modetab[2] = (item_t)fields * 2;

	const item_t result = type_add(prs->sx, local_modetab, local_md);

	if (created)
	{
		node_set_arg(&nd, 0, result);

		const size_t id = to_identab(prs, repr, 1000, result);
		node_set_arg(&nd, 1, (item_t)id);
	}

	return result;
}

/**
 *	Parse declarator with optional initializer
 *
 *	init-declarator:
 *		direct-declarator initializer[opt]
 *
 *	direct-declarator:
 *		identifier
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	type		Type of variable in declaration
 */
static void parse_init_declarator(parser *const prs, node *const parent, item_t type)
{
	const size_t name = token_get_ident_name(&prs->tk);
	const size_t id = to_identab(prs, name, 0, type);
	consume_token(prs);

	prs->flag_empty_bounds = 1;
	prs->array_dimensions = 0;

	node nd = node_add_child(parent, OP_DECL_VAR);
	node_add_arg(&nd, (item_t)id);
	node_add_arg(&nd, 0);	// Тут будет размерность
	node_add_arg(&nd, 0);	// Тут будет флаг наличия инициализатора

	if (token_is(&prs->tk, TK_L_SQUARE))
	{
		// Меняем тип (увеличиваем размерность массива)
		type = parse_array_definition(prs, &nd, type);
		ident_set_type(prs->sx, id, type);
		node_set_arg(&nd, 1, (item_t)prs->array_dimensions);
		if (!prs->flag_empty_bounds && token_is_not(&prs->tk, TK_EQUAL))
		{
			parser_error(prs, empty_bound_without_init);
		}
	}

	if (try_consume_token(prs, TK_EQUAL))
	{
		node_set_arg(&nd, 2, true);

		node initializer = parse_initializer(prs);
		if (!node_is_correct(&initializer))
		{
			skip_until(prs, TK_SEMICOLON);
			return;
		}

		check_assignment_operands(&prs->bld, type, &initializer);

		node temp = node_add_child(&nd, OP_NOP);
		node_swap(&initializer, &temp);
		node_remove(&temp);
	}
}

static void parse_init_enum_field_declarator(parser *const prs, item_t type, item_t number, size_t name)
{
	const size_t old_id = to_identab(prs, name, 0, type);
	ident_set_displ(prs->sx, old_id, number);
}

static item_t parse_enum_declaration_list(parser *const prs, node *const parent)
{
	consume_token(prs);
	if (try_consume_token(prs, TK_R_BRACE))
	{
		parser_error(prs, empty_enum);
		return TYPE_UNDEFINED;
	}

	size_t local_md = 2;
	item_t field_value = 0;
	item_t local_modetab[100];

	const item_t type = type_add(prs->sx, (item_t[]){ TYPE_ENUM }, 1);

	do
	{
		if (token_is_not(&prs->tk, TK_IDENTIFIER))
		{
			parser_error(prs, wait_ident_after_comma_in_enum);
			skip_until(prs, TK_SEMICOLON | TK_R_BRACE);
		}

		const size_t name = token_get_ident_name(&prs->tk);
		consume_token(prs);

		if (token_is(&prs->tk, TK_EQUAL))
		{

			consume_token(prs);
			node_copy(&prs->bld.context, parent);

			node expr = parse_constant_expression(prs);
			if (!node_is_correct(&expr))
			{
				continue;
			}
			const item_t type_expr = expression_get_type(&expr);
			field_value = expression_literal_get_integer(&expr);
			node_remove(&expr);

			if (field_value == INT_MAX || (type_expr != TYPE_INTEGER && type_expr != type))
			{
				parser_error(prs, not_const_int_expr);
				return TYPE_UNDEFINED;
			}
			parse_init_enum_field_declarator(prs, -type, field_value++, name);
		}
		else
		{
			parse_init_enum_field_declarator(prs, -type, field_value++, name);
		}

		local_modetab[local_md++] = field_value - 1;
		local_modetab[local_md++] = (item_t)name;
		if (token_is(&prs->tk, TK_R_BRACE))
		{
			continue;
		}
		expect_and_consume(prs, TK_COMMA, no_comma_in_enum);
	} while (!try_consume_token(prs, TK_R_BRACE));

	local_modetab[1] = (item_t)(local_md - 2);
	local_modetab[0] = local_modetab[2] / 2;

	type_enum_add_fields(prs->sx, local_modetab, local_md);
	return type;
}

static item_t parse_enum_specifier(parser *const prs, node *const parent)
{
	switch (token_get_kind(&prs->tk))
	{
		case TK_L_BRACE:
		{
			const item_t type = parse_enum_declaration_list(prs, parent);
			prs->was_type_def = true;
			return type;
		}
		case TK_IDENTIFIER:
		{
			const size_t repr = token_get_ident_name(&prs->tk);
			consume_token(prs);

			if (token_is(&prs->tk, TK_L_BRACE))
			{
				const item_t type = parse_enum_declaration_list(prs, parent);
				const size_t id = to_identab(prs, repr, 1000, type);
				prs->was_type_def = true;
				return ident_get_type(prs->sx, (size_t)id);
			}
			else // if (parser->next_token != l_brace)
			{
				const item_t id = repr_get_reference(prs->sx, repr);

				if (id == ITEM_MAX)
				{
					parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, repr));
					return TYPE_UNDEFINED;
				}
				return ident_get_type(prs->sx, (size_t)id);
			}
		}

		default:
			parser_error(prs, wrong_struct);
			return TYPE_UNDEFINED;
	}
}


/**
 *	Parse declaration
 *
 *	declaration:
 *		type-specifier init-declarator-list[opt] ';'
 *
 *	init-declarator-list:
 *		init-declarator
 *		init-declarator-list ',' init-declarator
 *
 *	@param	prs			Parser
 *
 *	@return Declaration
 */
static node parse_declaration(parser *const prs)
{
	// TODO: рефакторинг разбора объявлений
	node parent = node_add_child(&prs->bld.context, OP_DECLSTMT);

	prs->was_type_def = 0;
	item_t group_type = parse_type_specifier(prs, &parent);

	if (type_is_void(group_type))
	{
		parser_error(prs, only_functions_may_have_type_VOID);
		group_type = TYPE_UNDEFINED;
	}
	else if (prs->was_type_def && try_consume_token(prs, TK_SEMICOLON))
	{
		return parent;
	}

	do
	{
		item_t type = group_type;
		if (try_consume_token(prs, TK_STAR))
		{
			type = type_pointer(prs->sx, group_type);
		}

		if (token_is(&prs->tk, TK_IDENTIFIER))
		{
			parse_init_declarator(prs, &parent, type);
		}
		else
		{
			parser_error(prs, after_type_must_be_ident);
			skip_until(prs, TK_COMMA | TK_SEMICOLON);
		}
	} while (try_consume_token(prs, TK_COMMA));

	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_decl);
	return parent;
}


/*
 *	 ______     ______   ______     ______   ______     __    __     ______     __   __     ______   ______
 *	/\  ___\   /\__  _\ /\  __ \   /\__  _\ /\  ___\   /\ "-./  \   /\  ___\   /\ "-.\ \   /\__  _\ /\  ___\
 *	\ \___  \  \/_/\ \/ \ \  __ \  \/_/\ \/ \ \  __\   \ \ \-./\ \  \ \  __\   \ \ \-.  \  \/_/\ \/ \ \___  \
 *	 \/\_____\    \ \_\  \ \_\ \_\    \ \_\  \ \_____\  \ \_\ \ \_\  \ \_____\  \ \_\\"\_\    \ \_\  \/\_____\
 *	  \/_____/     \/_/   \/_/\/_/     \/_/   \/_____/   \/_/  \/_/   \/_____/   \/_/ \/_/     \/_/   \/_____/
 */


/** Check if current token is part of a declaration specifier */
static bool is_declaration_specifier(parser *const prs)
{
	switch (token_get_kind(&prs->tk))
	{
		case TK_VOID:
		case TK_BOOL:
		case TK_CHAR:
		case TK_INT:
		case TK_LONG:
		case TK_FLOAT:
		case TK_DOUBLE:
		case TK_STRUCT:
		case TK_ENUM:
		case TK_FILE:
		case TK_TYPEDEF:
			return true;

		case TK_IDENTIFIER:
		{
			const size_t name = token_get_ident_name(&prs->tk);
			const item_t id = repr_get_reference(prs->sx, name);
			if (id == ITEM_MAX)
			{
				return false;
			}

			return ident_is_type_specifier(prs->sx, (size_t)id);
		}

		default:
			return false;
	}
}

/**
 *	Parse case statement
 *
 *	labeled-statement:
 *		'case' constant-expression ':' statement
 *
 *	@param	prs			Parser
 *
 *	@return	Case statement
 */
static node parse_case_statement(parser *const prs)
{
	if (!prs->is_in_switch)
	{
		parser_error(prs, case_not_in_switch);
		skip_until(prs, TK_SEMICOLON);
		return node_broken();
	}

	const location case_loc = consume_token(prs);

	node expr = parse_constant_expression(prs);
	if (!node_is_correct(&expr))
	{
		skip_until(prs, TK_SEMICOLON | TK_R_BRACE);
		return node_broken();
	}

	expect_and_consume(prs, TK_COLON, expected_colon_after_case);
	node substmt = parse_statement(prs);

	return build_case_statement(&prs->bld, &expr, &substmt, case_loc);
}

/**
 *	Parse default statement
 *
 *	labeled-statement:
 *		'default' ':' statement
 *
 *	@param	prs			Parser
 *
 *	@return	Default statement
 */
static node parse_default_statement(parser *const prs)
{
	if (!prs->is_in_switch)
	{
		parser_error(prs, default_not_in_switch);
		skip_until(prs, TK_SEMICOLON);
		return node_broken();
	}

	const location default_loc = consume_token(prs);

	expect_and_consume(prs, TK_COLON, expected_colon_after_default);
	node substmt = parse_statement(prs);

	return build_default_statement(&prs->bld, &substmt, default_loc);
}

/**
 *	Parse compound statement
 *
 *	compound-statement:
 *  	'{' block-item-list[opt] '}'
 *
 *	block-item-list:
 *		block-item
 *		block-item-list block-item
 *
 *	block-item:
 *		declaration
 *		statement
 *
 *	@param	prs			Parser
 *
 *	@return	Compound statement
 */
static node parse_compound_statement(parser *const prs, const bool is_function_body)
{
	scope scp = { ITEM_MAX, ITEM_MAX };
	if (!is_function_body)
	{
		scp = scope_block_enter(prs->sx);
	}

	const location l_loc = consume_token(prs);

	node_vector stmts = node_vector_create();
	while (token_is_not(&prs->tk, TK_R_BRACE) && token_is_not(&prs->tk, TK_EOF))
	{
		const node stmt = is_declaration_specifier(prs)
			? parse_declaration(prs)
			: parse_statement(prs);

		node_vector_add(&stmts, &stmt);
	}

	if (!is_function_body)
	{
		scope_block_exit(prs->sx, scp);
	}

	if (token_is_not(&prs->tk, TK_R_BRACE))
	{
		parser_error(prs, expected_r_brace, l_loc);
		node_vector_clear(&stmts);
		return node_broken();
	}

	const location r_loc = consume_token(prs);
	node result = build_compound_statement(&prs->bld, &stmts, l_loc, r_loc);
	node_vector_clear(&stmts);
	return result;
}

/**
 *	Parse expression statement
 *
 *	expression-statement:
 *		expression ';'
 *
 *	@param	prs			Parser
 *
 *	@return	Expression statement
 */
static node parse_expression_statement(parser *const prs)
{
	const node expr = parse_expression(prs);
	if (!node_is_correct(&expr))
	{
		skip_until(prs, TK_SEMICOLON);
		return node_broken();
	}

	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_expr);
	return expr;
}

/**
 *	Parse if statement
 *
 *	if-statement:
 *		'if' '(' expression ')' statement
 *		'if' '(' expression ')' statement 'else' statement
 *
 *	@param	prs			Parser
 *
 *	@return	If statement
 */
static node parse_if_statement(parser *const prs)
{
	const location if_loc = consume_token(prs);

	node condition = parse_condition(prs);
	node then_stmt = parse_statement(prs);

	if (try_consume_token(prs, TK_ELSE))
	{
		node else_stmt = parse_statement(prs);
		return build_if_statement(&prs->bld, &condition, &then_stmt, &else_stmt, if_loc);
	}

	return build_if_statement(&prs->bld, &condition, &then_stmt, NULL, if_loc);
}

/**
 *	Parse switch statement
 *
 *	switch-statement:
 *		'switch' '(' expression ')' statement
 *
 *	@param	prs			Parser
 *
 *	@return	Switch statement
 */
static node parse_switch_statement(parser *const prs)
{
	const location switch_loc = consume_token(prs);

	node condition = parse_condition(prs);

	const bool old_in_switch = prs->is_in_switch;
	prs->is_in_switch = true;
	node body = parse_statement(prs);
	prs->is_in_switch = old_in_switch;

	return build_switch_statement(&prs->bld, &condition, &body, switch_loc);
}

/**
 *	Parse while statement
 *
 *	while-statement:
 *		'while' '(' expression ')' statement
 *
 *	@param	prs			Parser
 *
 *	@return	While statement
 */
static node parse_while_statement(parser *const prs)
{
	const location while_loc = consume_token(prs);

	node condition = parse_condition(prs);

	const bool old_in_switch = prs->is_in_switch;
	prs->is_in_loop = true;
	node body = parse_statement(prs);
	prs->is_in_loop = old_in_switch;

	return build_while_statement(&prs->bld, &condition, &body, while_loc);
}

/**
 *	Parse do statement
 *
 *	do-statement:
 *		'do' statement 'while' '(' expression ')' ';'
 *
 *	@param	prs			Parser
 *
 *	@return	Do statement
 */
static node parse_do_statement(parser *const prs)
{
	const location do_loc = consume_token(prs);

	const bool old_in_loop = prs->is_in_loop;
	prs->is_in_loop = true;
	node body = parse_statement(prs);
	prs->is_in_loop = old_in_loop;

	if (!try_consume_token(prs, TK_WHILE))
	{
		parser_error(prs, expected_while, do_loc);
		skip_until(prs, TK_SEMICOLON);
		return node_broken();
	}

	node condition = parse_condition(prs);

	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
	return build_do_statement(&prs->bld, &body, &condition, do_loc);
}

/**
 *	Parse for statement
 *
 *	for-statement:
 *		'for' '(' expression[opt] ';' expression[opt] ';' expression[opt] ')' statement
 *		'for' '(' declaration expression[opt] ';' expression[opt] ')' statement
 *
 *	@param	prs			Parser
 *
 *	@return	For statement
 */
static node parse_for_statement(parser *const prs)
{
	const location for_loc = consume_token(prs);
	const scope scp = scope_block_enter(prs->sx);

	if (token_is_not(&prs->tk, TK_L_PAREN))
	{
		parser_error(prs, expected_l_paren_after_for);
		skip_until(prs, TK_SEMICOLON);
		return node_broken();
	}

	const location l_loc = consume_token(prs);

	node init;
	const bool has_init = !try_consume_token(prs, TK_SEMICOLON);
	if (has_init)
	{
		if (is_declaration_specifier(prs))
		{
			init = parse_declaration(prs);
		}
		else
		{
			init = parse_expression(prs);
			if (!node_is_correct(&init))
			{
				skip_until(prs, TK_SEMICOLON);
				return node_broken();
			}

			if (!try_consume_token(prs, TK_SEMICOLON))
			{
				parser_error(prs, expected_semi_in_for_specifier);
				skip_until(prs, TK_SEMICOLON);
				return node_broken();
			}
		}
	}

	node cond;
	const bool has_cond = !try_consume_token(prs, TK_SEMICOLON);
	if (has_cond)
	{
		cond = parse_expression(prs);
		if (!node_is_correct(&cond))
		{
			skip_until(prs, TK_SEMICOLON);
			return node_broken();
		}

		if (!try_consume_token(prs, TK_SEMICOLON))
		{
			parser_error(prs, expected_semi_in_for_specifier);
			skip_until(prs, TK_SEMICOLON);
			return node_broken();
		}
	}

	node incr;
	const bool has_incr = token_is_not(&prs->tk, TK_R_PAREN);
	if (has_incr)
	{
		incr = parse_expression(prs);
		if (!node_is_correct(&incr))
		{
			skip_until(prs, TK_SEMICOLON);
			return node_broken();
		}
	}

	if (!try_consume_token(prs, TK_R_PAREN))
	{
		parser_error(prs, expected_r_paren, l_loc);
		skip_until(prs, TK_SEMICOLON);
		return node_broken();
	}

	const bool old_in_loop = prs->is_in_loop;
	prs->is_in_loop = true;
	node body = parse_statement(prs);
	prs->is_in_loop = old_in_loop;

	scope_block_exit(prs->sx, scp);
	return build_for_statement(&prs->bld, has_init ? &init : NULL
		, has_cond ? &cond : NULL, has_incr ? &incr : NULL, &body, for_loc);
}

/**
 *	Parse continue statement
 *
 *	jump-statement:
 *		'continue' ';'
 *
 *	@param	prs			Parser
 *
 *	@return	Continue statement
 */
static node parse_continue_statement(parser *const prs)
{
	if (!prs->is_in_loop)
	{
		parser_error(prs, continue_not_in_loop);
		skip_until(prs, TK_SEMICOLON);
		return node_broken();
	}

	const location continue_loc = consume_token(prs);
	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
	return build_continue_statement(&prs->bld, continue_loc);
}

/**
 *	Parse break statement
 *
 *	jump-statement:
 *		'break' ';'
 *
 *	@param	prs			Parser
 *
 *	@return	Break statement
 */
static node parse_break_statement(parser *const prs)
{
	if (!(prs->is_in_loop || prs->is_in_switch))
	{
		parser_error(prs, break_not_in_loop_or_switch);
		skip_until(prs, TK_SEMICOLON);
		return node_broken();
	}

	const location break_loc = consume_token(prs);
	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
	return build_break_statement(&prs->bld, break_loc);
}

/**
 *	Parse return statement
 *
 *	jump-statement:
 *		'return' expression[opt] ';'
 *
 *	@param	prs			Parser
 *
 *	@return	Return statement
 */
static node parse_return_statement(parser *const prs)
{
	prs->was_return = true;
	const location return_loc = consume_token(prs);
	if (try_consume_token(prs, TK_SEMICOLON))
	{
		return build_return_statement(&prs->bld, NULL, return_loc);
	}

	node expr = parse_expression(prs);
	if (node_is_correct(&expr))
	{
		expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
		return build_return_statement(&prs->bld, &expr, return_loc);
	}

	skip_until(prs, TK_SEMICOLON);
	return node_broken();
}

/**
 *	Parse statement
 *
 *	statement:
 *		labeled-statement
 *		compound-statement
 *		expression-statement
 *		selection-statement
 *		iteration-statement
 *		jump-statement
 *
 *	@param	prs			Parser
 *
 *	@return	Statement
 */
static node parse_statement(parser *const prs)
{
	switch (token_get_kind(&prs->tk))
	{
		case TK_CASE:
			return parse_case_statement(prs);

		case TK_DEFAULT:
			return parse_default_statement(prs);

		case TK_L_BRACE:
			return parse_compound_statement(prs, /*is_function_body=*/false);

		case TK_SEMICOLON:
			return build_null_statement(&prs->bld, consume_token(prs));

		case TK_IF:
			return parse_if_statement(prs);

		case TK_SWITCH:
			return parse_switch_statement(prs);

		case TK_WHILE:
			return parse_while_statement(prs);

		case TK_DO:
			return parse_do_statement(prs);

		case TK_FOR:
			return parse_for_statement(prs);

		case TK_CONTINUE:
			return parse_continue_statement(prs);

		case TK_BREAK:
			return parse_break_statement(prs);

		case TK_RETURN:
			return parse_return_statement(prs);

		default:
			return parse_expression_statement(prs);
	}
}


/*
 *	 ______     __  __     ______   ______     ______     __   __     ______     __
 *	/\  ___\   /\_\_\_\   /\__  _\ /\  ___\   /\  == \   /\ "-.\ \   /\  __ \   /\ \
 *	\ \  __\   \/_/\_\/_  \/_/\ \/ \ \  __\   \ \  __<   \ \ \-.  \  \ \  __ \  \ \ \____
 *	 \ \_____\   /\_\/\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\\"\_\  \ \_\ \_\  \ \_____\
 *	  \/_____/   \/_/\/_/     \/_/   \/_____/   \/_/ /_/   \/_/ \/_/   \/_/\/_/   \/_____/
 *
 *	 _____     ______     ______   __     __   __     __     ______   __     ______     __   __     ______
 *	/\  __-.  /\  ___\   /\  ___\ /\ \   /\ "-.\ \   /\ \   /\__  _\ /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \ \/\ \ \ \  __\   \ \  __\ \ \ \  \ \ \-.  \  \ \ \  \/_/\ \/ \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \____-  \ \_____\  \ \_\    \ \_\  \ \_\\"\_\  \ \_\    \ \_\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/____/   \/_____/   \/_/     \/_/   \/_/ \/_/   \/_/     \/_/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */


/**
 *	Parse function declarator
 *
 *	@param	prs			Parser structure
 *	@param	level		Level of declarator
 *	@param	func_def	@c 0 for function without arguments,
 *						@c 1 for function definition,
 *						@c 2 for function declaration,
 *						@c 3 for others
 *	@param	return_type	Return type of declarated function
 *
 *	@return	Index of types table, @c type_undefined on failure
 */
static item_t parse_function_declarator(parser *const prs, const int level, int func_def, const item_t return_type)
{
	item_t local_modetab[100];
	size_t local_md = 3;
	size_t args = 0;

	if (try_consume_token(prs, TK_R_PAREN))
	{
		prs->func_def = 0;
	}
	else
	{
		do
		{
			int arg_func = 0;	/**< Тип возврата функции-параметра:
									 @c 0 - обычный тип,
									 @c 1 - была '*',
									 @c 2 - была '[' */
			item_t type = parse_type_specifier(prs, NULL);

			if (try_consume_token(prs, TK_STAR))
			{
				arg_func = 1;
				type = type_pointer(prs->sx, type);
			}

			// На 1 уровне это может быть определением функции или предописанием;
			// На остальных уровнях - только декларатором (без идентов)
			bool was_ident = false;
			if (level)
			{
				if (token_is(&prs->tk, TK_IDENTIFIER))
				{
					was_ident = true;
					func_add(prs->sx, (item_t)token_get_ident_name(&prs->tk));
					consume_token(prs);
				}
			}
			else if (token_is(&prs->tk, TK_IDENTIFIER))
			{
				parser_error(prs, ident_in_declarator);
				skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
				try_consume_token(prs, TK_R_PAREN);
				return TYPE_UNDEFINED;
			}

			if (type_is_void(type) && token_is_not(&prs->tk, TK_L_PAREN))
			{
				parser_error(prs, par_type_void_with_nofun);
			}

			if (token_is(&prs->tk, TK_L_SQUARE))
			{
				arg_func = 2;
				if (type_is_pointer(prs->sx, type) && !was_ident)
				{
					parser_error(prs, aster_with_row);
				}

				while (try_consume_token(prs, TK_L_SQUARE))
				{
					type = type_array(prs->sx, type);
					if (!try_consume_token(prs, TK_R_SQUARE))
					{
						parser_error(prs, wait_right_sq_br);
						skip_until(prs, TK_R_SQUARE | TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
						try_consume_token(prs, TK_R_SQUARE);
					}
				}
			}

			if (try_consume_token(prs, TK_L_PAREN))
			{
				expect_and_consume(prs, TK_STAR, wrong_func_as_arg);
				if (token_is(&prs->tk, TK_IDENTIFIER))
				{
					if (level)
					{
						if (!was_ident)
						{
							was_ident = true;
						}
						else
						{
							parser_error(prs, two_idents_for_1_declarer);
							return TYPE_UNDEFINED;
						}
						func_add(prs->sx, -((item_t)token_get_ident_name(&prs->tk)));
						consume_token(prs);
					}
					else
					{
						parser_error(prs, ident_in_declarator);
						return TYPE_UNDEFINED;
					}
				}

				expect_and_consume(prs, TK_R_PAREN, no_right_br_in_arg_func);
				expect_and_consume(prs, TK_L_PAREN, wrong_func_as_arg);
				if (arg_func == 1)
				{
					parser_error(prs, aster_before_func);
					skip_until(prs, TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
				}
				else if (arg_func == 2)
				{
					parser_error(prs, array_before_func);
					skip_until(prs, TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
				}

				const int old_func_def = prs->func_def;
				type = parse_function_declarator(prs, 0, 2, type);
				prs->func_def = old_func_def;
			}
			if (func_def == 3)
			{
				func_def = was_ident ? 1 : 2;
			}
			else if (func_def == 2 && was_ident)
			{
				parser_error(prs, wait_declarator);
				skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
				// На случай, если после этого заголовка стоит тело функции
				if (try_consume_token(prs, TK_L_BRACE))
				{
					skip_until(prs, TK_R_BRACE);
					try_consume_token(prs, TK_R_BRACE);
				}
				return TYPE_UNDEFINED;
			}
			else if (func_def == 1 && !was_ident)
			{
				parser_error(prs, wait_definition);
				skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
				return TYPE_UNDEFINED;
			}

			args++;
			local_modetab[local_md++] = type;
		} while (try_consume_token(prs, TK_COMMA));

		expect_and_consume(prs, TK_R_PAREN, wrong_param_list);
		prs->func_def = func_def;
	}

	local_modetab[0] = TYPE_FUNCTION;
	local_modetab[1] = return_type;
	local_modetab[2] = (item_t)args;

	return type_add(prs->sx, local_modetab, local_md);
}

/**
 *	Parse function definition
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	function_id	Function number
 */
static void parse_function_definition(parser *const prs, node *const parent, const size_t function_id)
{
	if (function_id == SIZE_MAX)
	{
		// skip whole function body
		skip_until(prs, TK_R_BRACE);
		try_consume_token(prs, TK_R_BRACE);
		return;
	}

	prs->bld.func_type = ident_get_type(prs->sx, function_id);
	const size_t function_number = (size_t)ident_get_displ(prs->sx, function_id);
	const size_t param_number = type_function_get_parameter_amount(prs->sx, prs->bld.func_type);

	prs->was_return = 0;

	const item_t prev = ident_get_prev(prs->sx, function_id);
	if (prev > 1 && prev != ITEM_MAX - 1) // Был прототип
	{
		if (prs->bld.func_type != ident_get_type(prs->sx, (size_t)prev))
		{
			parser_error(prs, decl_and_def_have_diff_type);
			skip_until(prs, TK_R_BRACE);
			try_consume_token(prs, TK_R_BRACE);
			return;
		}
		ident_set_displ(prs->sx, (size_t)prev, (item_t)function_number);
	}

	node nd = node_add_child(parent, OP_FUNC_DEF);
	node_add_arg(&nd, (item_t)function_id);
	node_add_arg(&nd, 0); // for max_displ

	const item_t old_displ = scope_func_enter(prs->sx);

	for (size_t i = 0; i < param_number; i++)
	{
		item_t type = type_function_get_parameter_type(prs->sx, prs->bld.func_type, i);
		const item_t repr = func_get(prs->sx, function_number + i + 1);

		const size_t id = to_identab(prs, (size_t)llabs(repr), repr > 0 ? 0 : -1, type);

		size_t dim = 0;
		while (type_is_array(prs->sx, type))
		{
			dim++;
			type = type_array_get_element_type(prs->sx, type);
		}

		node param = node_add_child(&nd, OP_DECL_VAR);
		node_add_arg(&param, (item_t)id);	// id
		node_add_arg(&param, (item_t)dim);	// dim
		node_add_arg(&param, false);		// has init
	}

	func_set(prs->sx, function_number, (item_t)node_save(&nd)); // Ссылка на расположение в дереве

	node_copy(&prs->bld.context, &nd);
	node body = parse_compound_statement(prs, /*is_function_body=*/true);

	node temp = node_add_child(&nd, OP_NOP);
	node_swap(&body, &temp);
	node_remove(&temp);

	if (type_function_get_return_type(prs->sx, prs->bld.func_type) != TYPE_VOID && !prs->was_return)
	{
		parser_error(prs, nonvoid_func_void_return);
	}

	const item_t max_displ = scope_func_exit(prs->sx, old_displ);
	node_set_arg(&nd, 1, max_displ);
}

/**
 *	Parse function declaration
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	type		Return type of a function
 */
static void parse_function_declaration(parser *const prs, node *const parent, const item_t type)
{
	const size_t function_num = func_reserve(prs->sx);
	const size_t function_repr = token_get_ident_name(&prs->tk);

	consume_token(prs);	// TK_IDENTIFIER
	consume_token(prs);	// TK_L_PAREN
	const item_t function_mode = parse_function_declarator(prs, 1, 3, type);

	if (prs->func_def == 0 && token_is(&prs->tk, TK_L_BRACE))
	{
		prs->func_def = 1;
	}
	else if (prs->func_def == 0)
	{
		prs->func_def = 2;
	}

	const size_t function_id = to_identab(prs, function_repr, (item_t)function_num, function_mode);

	if (token_is(&prs->tk, TK_L_BRACE))
	{
		if (prs->func_def == 1)
		{
			parse_function_definition(prs, parent, function_id);
		}
		else
		{
			parser_error(prs, func_decl_req_params);
			skip_until(prs, TK_R_BRACE);
			try_consume_token(prs, TK_R_BRACE);
		}
	}
	else if (prs->func_def == 1)
	{
		parser_error(prs, function_has_no_body);
		// На тот случай, если после неправильного декларатора стоит ';'
		try_consume_token(prs, TK_SEMICOLON);
	}
}

/**
 *	Parse external definition
 *
 *	@param	prs			Parser
 *	@param	root		Root node in AST
 */
static void parse_external_definition(parser *const prs, node *const root)
{
	prs->was_type_def = 0;
	prs->func_def = 3;
	const item_t group_type = parse_type_specifier(prs, root);

	if (prs->was_type_def && try_consume_token(prs, TK_SEMICOLON))
	{
		return;
	}

	do
	{
		item_t type = group_type;
		if (token_is(&prs->tk, TK_STAR))
		{
			consume_token(prs);
			type = type_pointer(prs->sx, group_type);
		}

		if (token_is(&prs->tk, TK_IDENTIFIER))
		{
			if (peek_token(prs) == TK_L_PAREN)
			{
				parse_function_declaration(prs, root, type);
			}
			else if (type_is_void(group_type))
			{
				parser_error(prs, only_functions_may_have_type_VOID);
			}
			else
			{
				parse_init_declarator(prs, root, type);
			}
		}
		else
		{
			parser_error(prs, after_type_must_be_ident);
			skip_until(prs, TK_COMMA | TK_SEMICOLON);
		}
	} while (try_consume_token(prs, TK_COMMA));

	if (prs->func_def != 1)
	{
		expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_decl);
	}
}

/**
 *	Parse translation unit
 *
 *	@param	prs			Parser
 *	@param	root		Root node
 */
static void parse_translation_unit(parser *const prs, node *const root)
{
	do
	{
		parse_external_definition(prs, root);
	} while (token_is_not(&prs->tk, TK_EOF));
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int parse(syntax *const sx)
{
	if (sx == NULL)
	{
		return -1;
	}

	parser prs = parser_create(sx);
	node root = node_get_root(&sx->tree);

	parse_translation_unit(&prs, &root);

#ifndef NDEBUG
	write_tree(DEFAULT_TREE, sx);
#endif

	parser_clear(&prs);
	// Временное решение - парсер не проверяет таблицы
	return sx->rprt.errors == 0 ? 0 : -1;
}
