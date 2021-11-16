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


#define MAX_PRINTF_ARGS 20


static const char *const DEFAULT_TREE = "tree.txt";
static const size_t MAX_LABELS = 10000;


/** Parser */
typedef struct parser
{
	syntax *sx;							/**< Syntax structure */

	lexer lxr;							/**< Lexer */

	vector labels;						/**< Labels table */

	token_t token;						/**< Current 'peek token' */
	size_t function_mode;				/**< Mode of current parsed function */
	size_t array_dimensions;			/**< Array dimensions counter */

	int func_def;						/**< @c 0 for function without arguments,
											@c 1 for function definition,
											@c 2 for function declaration,
											@c 3 for others */

	int flag_strings_only;				/**< @c 0 for non-string initialization,
											@c 1 for string initialization,
											@c 2 for parsing before initialization */

	int flag_array_in_struct;			/**< Set, if parsed struct declaration has an array */
	int flag_empty_bounds;				/**< Set, if array declaration has empty bounds */

	bool is_in_switch;					/**< Set, if parser is in switch body */
	bool is_in_loop;					/**< Set, if parser is in loop body */

	bool was_return;					/**< Set, if was return in parsed function */
	bool was_type_def;					/**< Set, if was type definition */
} parser;


static item_t parse_struct_or_union_specifier(parser *const prs, node *const parent);
static item_t parse_struct_declaration_list(parser *const prs, node *const parent);
static item_t parse_enum_specifier(parser *const prs, node *const parent);
static location consume_token(parser *const prs);
static node parse_expression(parser *const prs);
static node parse_initializer(parser *const prs);
static void parse_statement(parser *const prs, node *const parent);


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
 *	@param	ws			Compiler workspace
 *	@param	sx			Syntax structure
 *
 *	@return	Parser
 */
static inline parser prs_create(const workspace *const ws, syntax *const sx)
{
	parser prs;
	prs.sx = sx;
	prs.lxr = lexer_create(ws, sx);

	prs.labels = vector_create(MAX_LABELS);
	consume_token(&prs);

	return prs;
}

/**
 *	Free allocated memory
 *
 *	@param	prs			Parser
 */
static inline void prs_clear(parser *const prs)
{
	vector_clear(&prs->labels);
	lexer_clear(&prs->lxr);
}

/**
 *	Emit a syntax error from parser
 *
 *	@param	prs			Parser
 *	@param	num			Error code
 */
static void parser_error(parser *const prs, error_t num, ...)
{
	if (prs->lxr.is_recovery_disabled && prs->sx->was_error)
	{
		return;
	}

	va_list args;
	va_start(args, num);

	verror(prs->sx->io, num, args);
	prs->sx->was_error = true;

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
	// TODO: брать положение из лексера
	const size_t token_start = prs->lxr.location;
	prs->token = lex(&prs->lxr);
	return (location){ token_start, in_get_position(prs->sx->io) };
}

/**
 *	Peek ahead token without consuming it
 *
 *	@param	prs			Parser
 *
 *	@return	Peeked token
 */
static inline token_t peek_token(parser *const prs)
{
	return peek(&prs->lxr);
}

/**
 *	Consume the current 'peek token' if it is expected
 *
 *	@param	prs			Parser
 *	@param	expected	Expected token
 *
 *	@return	@c 1 on consuming 'peek token', @c 0 on otherwise
 */
static bool try_consume_token(parser *const prs, const token_t expected)
{
	if (prs->token == expected)
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
 *	@param	expected	Expected token
 *	@param	err			Error to emit
 */
static void expect_and_consume(parser *const prs, const token_t expected, const error_t err)
{
	if (!try_consume_token(prs, expected))
	{
		parser_error(prs, err);
	}
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
 *	@param	tokens		Set of specified tokens
 */
static void skip_until(parser *const prs, const uint8_t tokens)
{
	while (prs->token != TK_EOF)
	{
		switch (prs->token)
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
				if (has_token_set(tokens, prs->token))
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
 *		constant
 *		string-literal
 *		'NULL'
 *		'(' expression ')'
 *
 *	@param	prs			Parser
 *
 *	@return	Primary expression
 */
static node parse_primary_expression(parser *const prs)
{
	switch (prs->token)
	{
		case TK_IDENTIFIER:
		{
			const size_t name = prs->lxr.repr;
			const location loc = consume_token(prs);

			return build_identifier_expression(prs->sx, name, loc);
		}

		case TK_CHAR_CONST:
		case TK_INT_CONST:
		{
			const item_t value = prs->lxr.num;
			const location loc = consume_token(prs);

			return build_integer_literal_expression(prs->sx, value, loc);
		}

		case TK_FLOAT_CONST:
		{
			const double value = prs->lxr.num_double;
			const location loc = consume_token(prs);

			return build_floating_literal_expression(prs->sx, value, loc);
		}

		case TK_STRING:
		{
			const size_t value = prs->lxr.num_string;
			const location loc = consume_token(prs);

			return build_string_literal_expression(prs->sx, value, loc);
		}

		case TK_NULL:
		{
			const location loc = consume_token(prs);
			return build_null_pointer_literal_expression(prs->sx, loc);
		}

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
		switch (prs->token)
		{
			default:
				return operand;

			case TK_L_SQUARE:
			{
				const location l_loc = consume_token(prs);
				node index = parse_expression(prs);

				if (prs->token == TK_R_SQUARE)
				{
					const location r_loc = consume_token(prs);
					operand = build_subscript_expression(prs->sx, &operand, &index, l_loc, r_loc);
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

				if (prs->token == TK_R_PAREN)
				{
					const location r_loc = consume_token(prs);
					operand = build_call_expression(prs->sx, &operand, NULL, l_loc, r_loc);

					continue;
				}

				node_vector args = parse_initializer_list(prs);
				if (prs->token == TK_R_PAREN)
				{
					const location r_loc = consume_token(prs);
					operand = build_call_expression(prs->sx, &operand, &args, l_loc, r_loc);
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
				const bool is_arrow = prs->token == TK_ARROW;
				const location op_loc = consume_token(prs);

				if (prs->token == TK_IDENTIFIER)
				{
					const size_t name = prs->lxr.repr;
					const location id_loc = consume_token(prs);

					operand = build_member_expression(prs->sx, &operand, name, is_arrow, op_loc, id_loc);
				}
				else
				{
					parser_error(prs, expected_identifier);
					operand = node_broken();
				}

				continue;
			}

			case TK_PLUS_PLUS:
			{
				const location op_loc = consume_token(prs);
				operand = build_unary_expression(prs->sx, &operand, UN_POSTINC, op_loc);
				continue;
			}

			case TK_MINUS_MINUS:
			{
				const location op_loc = consume_token(prs);
				operand = build_unary_expression(prs->sx, &operand, UN_POSTDEC, op_loc);
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
 *		'&', '*', '+', '-', '~', '!', 'abs'
 *
 *	@param	prs			Parser
 *
 *	@return	Unary expression
 */
static node parse_unary_expression(parser *const prs)
{
	switch (prs->token)
	{
		default:
			return parse_postfix_expression(prs);

		case TK_PLUS_PLUS:
		case TK_MINUS_MINUS:
		case TK_AMP:
		case TK_STAR:
		case TK_PLUS:
		case TK_MINUS:
		case TK_TILDE:
		case TK_EXCLAIM:
		case TK_ABS:
		{
			const unary_t operator = token_to_unary(prs->token);
			const location op_loc = consume_token(prs);
			node operand = parse_unary_expression(prs);

			return build_unary_expression(prs->sx, &operand, operator, op_loc);
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
	precedence_t next_token_prec = get_operator_precedence(prs->token);
	while (next_token_prec >= min_prec)
	{
		const token_t op_token = prs->token;
		location op_loc = consume_token(prs);

		const bool is_binary = next_token_prec != PREC_CONDITIONAL;
		node middle = node_broken();
		if (!is_binary)
		{
			middle = parse_expression(prs);

			if (prs->token != TK_COLON)
			{
				parser_error(prs, expected_colon_in_conditional, op_loc);
			}

			op_loc = consume_token(prs);
		}

		// FIXME: случай 'a > 0 ? p : { 5, 0 };
		node RHS = (prs->token == TK_L_BRACE) ? parse_initializer(prs) : parse_unary_expression(prs);

		const precedence_t this_prec = next_token_prec;
		next_token_prec = get_operator_precedence(prs->token);

		const bool is_right_associative = this_prec == PREC_CONDITIONAL || this_prec == PREC_ASSIGNMENT;
		if (this_prec < next_token_prec || (this_prec == next_token_prec && is_right_associative))
		{
			RHS = parse_RHS_of_binary_expression(prs, &RHS, (this_prec + !is_right_associative));
			next_token_prec = get_operator_precedence(prs->token);
		}

		if (is_binary)
		{
			// Отказ от node_copy, так как node_broken все равно нужно скопировать
			const binary_t op_kind = token_to_binary(op_token);
			*LHS = build_binary_expression(prs->sx, LHS, &RHS, op_kind, op_loc);
		}
		else
		{
			*LHS = build_ternary_expression(prs->sx, LHS, &middle, &RHS, op_loc);
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
	if (expression_get_class(&LHS) != EXPR_LITERAL)
	{
		parser_error(prs, not_const_expr);
	}

	return LHS;
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
	if (prs->token == TK_L_BRACE)
	{
		const location l_loc = consume_token(prs);

		if (try_consume_token(prs, TK_R_BRACE))
		{
			consume_token(prs);
			parser_error(prs, empty_init);
			return node_broken();
		}

		node_vector inits = parse_initializer_list(prs);
		if (prs->token == TK_R_BRACE)
		{
			const location r_loc = consume_token(prs);
			const node result = build_init_list_expression(prs->sx, &inits, l_loc, r_loc);

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
 *		'char'
 *		'short'
 *		'int'
 *		'long'
 *		'float'
 *		'double'
 *		struct-or-union-specifier
 *		enum-specifier [TODO]
 *		typedef-name [TODO]
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *
 *	@return	Standard type or index of the types table
 */
static item_t parse_type_specifier(parser *const prs, node *const parent)
{
	prs->flag_array_in_struct = 0;
	switch (prs->token)
	{
		case TK_VOID:
			consume_token(prs);
			return TYPE_VOID;

		case TK_CHAR:
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
			const item_t id = repr_get_reference(prs->sx, prs->lxr.repr);
			consume_token(prs);

			if (id == ITEM_MAX || ident_get_displ(prs->sx, (size_t)id) < 1000)
			{
				parser_error(prs, ident_not_type);
				return TYPE_UNDEFINED;
			}

			prs->flag_array_in_struct = (int)ident_get_displ(prs->sx, (size_t)id) - 1000;
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
			if (prs->token == TK_STAR)
			{
				consume_token(prs);
				type = type_pointer(prs->sx, type);
			}
			if (prs->token == TK_IDENTIFIER)
			{
				const size_t repr = prs->lxr.repr;
				consume_token(prs);
				to_identab(prs, repr, 1000, type);
				prs->was_type_def = true;
				if (prs->token != TK_SEMICOLON)
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
	switch (prs->token)
	{
		case TK_L_BRACE:
			return parse_struct_declaration_list(prs, parent);

		case TK_IDENTIFIER:
		{
			const size_t repr = prs->lxr.repr;
			consume_token(prs);

			if (prs->token == TK_L_BRACE)
			{
				const item_t type = parse_struct_declaration_list(prs, parent);
				const size_t id = to_identab(prs, repr, 1000, type);
				ident_set_displ(prs->sx, id, 1000 + prs->flag_array_in_struct);
				prs->was_type_def = true;

				return ident_get_type(prs->sx, id);
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
				prs->flag_array_in_struct = (int)ident_get_displ(prs->sx, (size_t)id) - 1000;
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
			if (prs->token == TK_L_SQUARE)
			{
				// int a[][] = {{ 1, 2, 3 }, { 4, 5, 6 }};	// нельзя
				parser_error(prs, empty_init);
			}
			prs->flag_empty_bounds = 0;
		}
		else
		{
			node_copy(&prs->sx->nd, parent);
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
 *
 *	@return	Index of types table, @c type_undefined on failure
 */
static item_t parse_struct_declaration_list(parser *const prs, node *const parent)
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
	bool was_array = false;

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

		const size_t repr = prs->lxr.repr;
		if (try_consume_token(prs, TK_IDENTIFIER))
		{
			if (prs->token == TK_L_SQUARE)
			{
				if (!was_array)
				{
					nd = node_add_child(parent, OP_DECL_TYPE);
					node_add_arg(&nd, 0);
					was_array = true;
				}

				node decl = node_add_child(&nd, OP_DECL_VAR);
				node_add_arg(&decl, 0);
				node_add_arg(&decl, 0);
				node_add_arg(&decl, 0);
				// Меняем тип (увеличиваем размерность массива)
				type = parse_array_definition(prs, &decl, element_type);
				node_set_arg(&decl, 0, type);
				node_set_arg(&decl, 1, (item_t)fields);
			}
		}
		else
		{
			parser_error(prs, wait_ident_after_semicolon_in_struct);
			skip_until(prs, TK_SEMICOLON | TK_R_BRACE);
		}

		local_modetab[local_md++] = type;
		local_modetab[local_md++] = (item_t)repr;
		fields++;
		displ += type_size(prs->sx, type);

		expect_and_consume(prs, TK_SEMICOLON, no_semicolon_in_struct);
	} while (!try_consume_token(prs, TK_R_BRACE));

	local_modetab[0] = TYPE_STRUCTURE;
	local_modetab[1] = (item_t)displ;
	local_modetab[2] = (item_t)fields * 2;

	const item_t result = type_add(prs->sx, local_modetab, local_md);
	if (was_array)
	{
		node_set_arg(&nd, 0, result);
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
	const size_t id = to_identab(prs, prs->lxr.repr, 0, type);

	prs->flag_empty_bounds = 1;
	prs->array_dimensions = 0;

	node nd = node_add_child(parent, OP_DECL_VAR);
	node_add_arg(&nd, (item_t)id);
	node_add_arg(&nd, 0);	// Тут будет размерность
	node_add_arg(&nd, 0);	// Тут будет флаг наличия инициализатора

	if (prs->token == TK_L_SQUARE)
	{
		// Меняем тип (увеличиваем размерность массива)
		type = parse_array_definition(prs, &nd, type);
		ident_set_type(prs->sx, id, type);
		node_set_arg(&nd, 1, (item_t)prs->array_dimensions);
		if (!prs->flag_empty_bounds && prs->token != TK_EQUAL)
		{
			parser_error(prs, empty_bound_without_init);
		}
	}

	if (try_consume_token(prs, TK_EQUAL))
	{
		node_set_arg(&nd, 2, true);
		node_copy(&prs->sx->nd, &nd);

		node initializer = parse_initializer(prs);
		if (!node_is_correct(&initializer))
		{
			skip_until(prs, TK_SEMICOLON);
			return;
		}

		check_assignment_operands(prs->sx, type, &initializer);
	}
}

static void parse_init_enum_field_declarator(parser *const prs, item_t type, item_t number)
{
	const size_t old_id = to_identab(prs, prs->lxr.repr, 0, type);
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
		if (!try_consume_token(prs, TK_IDENTIFIER))
		{
			parser_error(prs, wait_ident_after_comma_in_enum);
			skip_until(prs, TK_SEMICOLON | TK_R_BRACE);
		}

		if (prs->token == TK_EQUAL)
		{
			const size_t repr = prs->lxr.repr;
			consume_token(prs);
			node_copy(&prs->sx->nd, parent);

			node expr = parse_constant_expression(prs);
			const item_t type_expr = expression_get_type(&expr);
			field_value = expression_literal_get_integer(&expr);
			node_remove(&expr);

			if (field_value == INT_MAX || (type_expr != TYPE_INTEGER && type_expr != type))
			{
				parser_error(prs, not_const_int_expr);
				return TYPE_UNDEFINED;
			}
			prs->lxr.repr = repr;
			parse_init_enum_field_declarator(prs, -type, field_value++);
		}
		else
		{
			parse_init_enum_field_declarator(prs, -type, field_value++);
		}

		local_modetab[local_md++] = field_value - 1;
		local_modetab[local_md++] = (item_t)prs->lxr.repr;
		if (prs->token == TK_R_BRACE)
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
	switch (prs->token)
	{
		case TK_L_BRACE:
		{
			const item_t type = parse_enum_declaration_list(prs, parent);
			prs->was_type_def = true;
			return type;
		}
		case TK_IDENTIFIER:
		{
			const size_t repr = prs->lxr.repr;
			consume_token(prs);

			if (prs->token == TK_L_BRACE)
			{
				const item_t type = parse_enum_declaration_list(prs, parent);
				const size_t id = to_identab(prs, repr, 1000, type);
				ident_set_displ(prs->sx, id, 1000 + prs->flag_array_in_struct);
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
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_declaration(parser *const prs, node *const parent)
{
	prs->was_type_def = 0;
	item_t group_type = parse_type_specifier(prs, parent);

	if (type_is_void(group_type))
	{
		parser_error(prs, only_functions_may_have_type_VOID);
		group_type = TYPE_UNDEFINED;
	}
	else if (prs->was_type_def && try_consume_token(prs, TK_SEMICOLON))
	{
		return;
	}

	do
	{
		item_t type = group_type;
		if (try_consume_token(prs, TK_STAR))
		{
			type = type_pointer(prs->sx, group_type);
		}

		if (try_consume_token(prs, TK_IDENTIFIER))
		{
			parse_init_declarator(prs, parent, type);
		}
		else
		{
			parser_error(prs, after_type_must_be_ident);
			skip_until(prs, TK_COMMA | TK_SEMICOLON);
		}
	} while (try_consume_token(prs, TK_COMMA));

	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_decl);
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
	switch (prs->token)
	{
		case TK_VOID:
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
			const item_t id = repr_get_reference(prs->sx, prs->lxr.repr);
			if (id == ITEM_MAX)
			{
				return false;
			}

			return ident_get_displ(prs->sx, (size_t)id) >= 1000;
		}

		default:
			return false;
	}
}

/**
 *	Parse labeled statement
 *
 *	labeled-statement:
 *		identifier ':' statement
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_labeled_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // identifier
	node nd = node_add_child(parent, OP_LABEL);
	const size_t repr = prs->lxr.repr;
	
	// Не проверяем, что это ':', так как по нему узнали, что это labeled statement
	consume_token(prs);
	for (size_t i = 0; i < vector_size(&prs->labels); i += 2)
	{
		if (repr == (size_t)ident_get_repr(prs->sx, (size_t)vector_get(&prs->labels, i)))
		{
			const item_t id = vector_get(&prs->labels, i);
			node_add_arg(&nd, id);

			if (vector_get(&prs->labels, i + 1) < 0)
			{
				parser_error(prs, repeated_label, repr_get_name(prs->sx, repr));
			}
			else
			{
				vector_set(&prs->labels, i + 1, -1);	// TODO: здесь должен быть номер строки
			}

			ident_set_type(prs->sx, (size_t)id, 1);
			parse_statement(prs, &nd);
			return;
		}
	}

	// Это определение метки, если она встретилась до переходов на нее
	const item_t id = (item_t)to_identab(prs, repr, 1, 0);
	node_add_arg(&nd, id);
	vector_add(&prs->labels, id);
	vector_add(&prs->labels, -1);	// TODO: здесь должен быть номер строки

	ident_set_type(prs->sx, (size_t)id, 1);
	parse_statement(prs, &nd);
}

/**
 *	Parse case statement
 *
 *	labeled-statement:
 *		'case' constant-expression ':' statement
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 */
static void parse_case_statement(parser *const prs, node *const parent)
{
	if (!prs->is_in_switch)
	{
		parser_error(prs, case_not_in_switch);
	}

	consume_token(prs); // kw_case
	node nd = node_add_child(parent, OP_CASE);
	node_copy(&prs->sx->nd, &nd);
	const node condition = parse_constant_expression(prs);
	const item_t condition_type = expression_get_type(&condition);
	if (node_is_correct(&condition) && !type_is_integer(prs->sx, condition_type))
	{
		parser_error(prs, float_in_switch);
	}

	expect_and_consume(prs, TK_COLON, expected_colon_after_case);
	parse_statement(prs, &nd);
}

/**
 *	Parse default statement
 *
 *	labeled-statement:
 *		'default' ':' statement
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 */
static void parse_default_statement(parser *const prs, node *const parent)
{
	if (!prs->is_in_switch)
	{
		parser_error(prs, default_not_in_switch);
	}

	consume_token(prs); // kw_default
	node nd = node_add_child(parent, OP_DEFAULT);
	expect_and_consume(prs, TK_COLON, expected_colon_after_default);
	parse_statement(prs, &nd);
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
 *	@param	parent		Parent node in AST
 */
static void parse_compound_statement(parser *const prs, node *const parent, const bool is_function_body)
{
	consume_token(prs); // '{'
	node nd = node_add_child(parent, OP_BLOCK);

	item_t old_displ = 0;
	item_t old_lg = 0;

	if (!is_function_body)
	{
		scope_block_enter(prs->sx, &old_displ, &old_lg);
	}

	if (!try_consume_token(prs, TK_R_BRACE))
	{
		while (prs->token != TK_EOF && prs->token != TK_R_BRACE)
		{
			if (is_declaration_specifier(prs))
			{
				parse_declaration(prs, &nd);
			}
			else
			{
				parse_statement(prs, &nd);
			}
		}

		expect_and_consume(prs, TK_R_BRACE, expected_end);
	}

	if (!is_function_body)
	{
		scope_block_exit(prs->sx, old_displ, old_lg);
	}
}

/**
 *	Parse expression statement
 *
 *	expression-statement:
 *		expression ';'
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_expression_statement(parser *const prs, node *const parent)
{
	node_copy(&prs->sx->nd, parent);
	const node expr = parse_expression(prs);
	if (!node_is_correct(&expr))
	{
		skip_until(prs, TK_SEMICOLON);
		return;
	}
	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**
 *	Parse if statement
 *
 *	if-statement:
 *		'if' '(' expression ')' statement
 *		'if' '(' expression ')' statement 'else' statement
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_if_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // kw_if
	node nd = node_add_child(parent, OP_IF);
	node_add_arg(&nd, 0); // ref_else

	node_copy(&prs->sx->nd, &nd);
	expect_and_consume(prs, TK_L_PAREN, cond_must_be_in_brkts);
	parse_expression(prs);
	expect_and_consume(prs, TK_R_PAREN, cond_must_be_in_brkts);
	parse_statement(prs, &nd);

	if (try_consume_token(prs, TK_ELSE))
	{
		node_set_arg(&nd, 0, 1);
		parse_statement(prs, &nd);
	}
}

/**
 *	Parse switch statement
 *
 *	switch-statement:
 *		'switch' '(' expression ')' statement
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_switch_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // kw_switch
	node nd = node_add_child(parent, OP_SWITCH);

	node_copy(&prs->sx->nd, &nd);
	expect_and_consume(prs, TK_L_PAREN, cond_must_be_in_brkts);
	const node condition = parse_expression(prs);
	expect_and_consume(prs, TK_R_PAREN, cond_must_be_in_brkts);
	const item_t condition_type = expression_get_type(&condition);
	if (node_is_correct(&condition) && !type_is_integer(prs->sx, condition_type))
	{
		parser_error(prs, float_in_switch);
	}

	const bool old_in_switch = prs->is_in_switch;
	prs->is_in_switch = true;
	parse_statement(prs, &nd);
	prs->is_in_switch = old_in_switch;
}

/**
 *	Parse while statement
 *
 *	while-statement:
 *		'while' '(' expression ')' statement
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_while_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // kw_while
	node nd = node_add_child(parent, OP_WHILE);

	node_copy(&prs->sx->nd, &nd);
	expect_and_consume(prs, TK_L_PAREN, cond_must_be_in_brkts);
	parse_expression(prs);
	expect_and_consume(prs, TK_R_PAREN, cond_must_be_in_brkts);

	const bool old_in_loop = prs->is_in_loop;
	prs->is_in_loop = true;
	parse_statement(prs, &nd);
	prs->is_in_loop = old_in_loop;
}

/**
 *	Parse do statement
 *
 *	do-statement:
 *		'do' statement 'while' '(' expression ')' ';'
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_do_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // kw_do
	node nd = node_add_child(parent, OP_DO);

	const bool old_in_loop = prs->is_in_loop;
	prs->is_in_loop = true;
	parse_statement(prs, &nd);
	prs->is_in_loop = old_in_loop;

	if (try_consume_token(prs, TK_WHILE))
	{
		node_copy(&prs->sx->nd, &nd);
		expect_and_consume(prs, TK_L_PAREN, cond_must_be_in_brkts);
		parse_expression(prs);
		expect_and_consume(prs, TK_R_PAREN, cond_must_be_in_brkts);
	}
	else
	{
		parser_error(prs, expected_while);
		skip_until(prs, TK_SEMICOLON);
	}

	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**
 *	Parse for statement
 *
 *	for-statement:
 *		'for' '(' expression[opt] ';' expression[opt] ';' expression[opt] ')' statement
 *		'for' '(' declaration expression[opt] ';' expression[opt] ')' statement
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 */
static void parse_for_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // kw_for
	node nd = node_add_child(parent, OP_FOR);

	node_add_arg(&nd, 0); // ref_inition
	node_add_arg(&nd, 0); // ref_condition
	node_add_arg(&nd, 0); // ref_increment
	node_add_arg(&nd, 1); // ref_statement
	expect_and_consume(prs, TK_L_PAREN, no_leftbr_in_for);

	item_t old_displ;
	item_t old_lg;
	scope_block_enter(prs->sx, &old_displ, &old_lg);
	if (!try_consume_token(prs, TK_SEMICOLON))
	{
		node_set_arg(&nd, 0, 1); // ref_inition
		if (is_declaration_specifier(prs))
		{
			parse_declaration(prs, &nd);
		}
		else
		{
			node_copy(&prs->sx->nd, &nd);
			parse_expression(prs);
			expect_and_consume(prs, TK_SEMICOLON, no_semicolon_in_for);
		}
	}

	if (!try_consume_token(prs, TK_SEMICOLON))
	{
		node_set_arg(&nd, 1, 1); // ref_condition
		node_copy(&prs->sx->nd, &nd);
		parse_expression(prs);
		expect_and_consume(prs, TK_SEMICOLON, no_semicolon_in_for);
	}

	if (!try_consume_token(prs, TK_R_PAREN))
	{
		node_set_arg(&nd, 2, 1); // ref_increment
		node_copy(&prs->sx->nd, &nd);
		parse_expression(prs);
		expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_for);
	}

	const bool old_in_loop = prs->is_in_loop;
	prs->is_in_loop = true;
	parse_statement(prs, &nd);

	prs->is_in_loop = old_in_loop;
	scope_block_exit(prs->sx, old_displ, old_lg);
}

/**
 *	Parse goto statement
 *
 *	jump-statement:
 *		'goto' identifier ';'
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 */
static void parse_goto_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // kw_goto
	node nd = node_add_child(parent, OP_GOTO);
	expect_and_consume(prs, TK_IDENTIFIER, no_ident_after_goto);
	const size_t repr = prs->lxr.repr;

	for (size_t i = 0; i < vector_size(&prs->labels); i += 2)
	{
		if (repr == (size_t)ident_get_repr(prs->sx, (size_t)vector_get(&prs->labels, i)))
		{
			const item_t id = vector_get(&prs->labels, i);
			node_add_arg(&nd, id);
			if (vector_get(&prs->labels, (size_t)id + 1) >= 0) // Перехода на метку еще не было
			{
				vector_add(&prs->labels, id);
				vector_add(&prs->labels, 1);	// TODO: здесь должен быть номер строки
			}

			expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
			return;
		}
	}

	// Первый раз встретился переход на метку, которой не было,
	// в этом случае ссылка на identtab, стоящая после TGoto,
	// будет отрицательной
	const item_t id = (item_t)to_identab(prs, repr, 1, 0);
	node_add_arg(&nd, -id);
	vector_add(&prs->labels, id);
	vector_add(&prs->labels, 1);	// TODO: здесь должен быть номер строки
	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**
 *	Parse continue statement
 *
 *	jump-statement:
 *		'continue' ';'
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_continue_statement(parser *const prs, node *const parent)
{
	if (!prs->is_in_loop)
	{
		parser_error(prs, continue_not_in_loop);
	}

	consume_token(prs); // kw_continue
	node_add_child(parent, OP_CONTINUE);
	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**
 *	Parse break statement
 *
 *	jump-statement:
 *		'break' ';'
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_break_statement(parser *const prs, node *const parent)
{
	if (!(prs->is_in_loop || prs->is_in_switch))
	{
		parser_error(prs, break_not_in_loop_or_switch);
	}

	consume_token(prs); // kw_break
	node_add_child(parent, OP_BREAK);
	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**
 *	Parse return statement
 *
 *	jump-statement:
 *		'return' expression[opt] ';'
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_return_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // kw_return
	const item_t return_type = type_function_get_return_type(prs->sx, (item_t)prs->function_mode);
	prs->was_return = true;

	node nd = node_add_child(parent, OP_RETURN);

	if (try_consume_token(prs, TK_SEMICOLON))
	{
		if (!type_is_void(return_type))
		{
			parser_error(prs, no_ret_in_func);
		}
	}
	else if (return_type != type_pointer(prs->sx, TYPE_VOID))
	{
		if (type_is_void(return_type))
		{
			parser_error(prs, notvoidret_in_void_func);
		}

		node_copy(&prs->sx->nd, &nd);
		node expr = parse_assignment_expression(prs);
		check_assignment_operands(prs->sx, return_type, &expr);
		// FIXME: надо кинуть ошибку с другой формулировкой

		expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
	}
}

/**	Parse printid statement [RuC] */
static void parse_printid_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // kw_printid
	expect_and_consume(prs, TK_L_PAREN, no_leftbr_in_printid);

	do
	{
		if (try_consume_token(prs, TK_IDENTIFIER))
		{
			const size_t repr = prs->lxr.repr;
			const item_t id = repr_get_reference(prs->sx, repr);
			if (id == ITEM_MAX)
			{
				parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, repr));
			}

			node nd = node_add_child(parent, OP_PRINTID);
			node_add_arg(&nd, id);
		}
		else
		{
			parser_error(prs, no_ident_in_printid);
			skip_until(prs, TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
		}
	} while (try_consume_token(prs, TK_COMMA));

	expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_printid);
	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**	Parse print statement [RuC] */
static void parse_print_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // kw_print
	expect_and_consume(prs, TK_L_PAREN, print_without_br);

	node nd = node_add_child(parent, OP_PRINT);

	node_copy(&prs->sx->nd, &nd);
	const node expr = parse_assignment_expression(prs);
	if (!node_is_correct(&expr))
	{
		skip_until(prs, TK_SEMICOLON);
		try_consume_token(prs, TK_SEMICOLON);
		return;
	}

	const item_t type = expression_get_type(&expr);
	if (type_is_pointer(prs->sx, type))
	{
		parser_error(prs, pointer_in_print);
	}

	if (!try_consume_token(prs, TK_R_PAREN))
	{
		parser_error(prs, print_without_br);
		skip_until(prs, TK_SEMICOLON);
	}

	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**	Parse getid statement [RuC] */
static void parse_getid_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // kw_getid
	expect_and_consume(prs, TK_L_PAREN, no_leftbr_in_getid);

	do
	{
		if (try_consume_token(prs, TK_IDENTIFIER))
		{
			const size_t repr = prs->lxr.repr;
			const item_t id = repr_get_reference(prs->sx, repr);
			if (id == ITEM_MAX)
			{
				parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, repr));
			}

			node nd = node_add_child(parent, OP_GETID);
			node_add_arg(&nd, id);
		}
		else
		{
			parser_error(prs, no_ident_in_getid);
			skip_until(prs, TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
		}
	} while (try_consume_token(prs, TK_COMMA));

	expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_getid);
	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

static size_t evaluate_args(parser *const prs, const vector *const format_str
	, item_t *const format_types, char32_t *const placeholders)
{
	size_t args = 0;
	const size_t length = vector_size(format_str);
	for (size_t i = 0; i < length; i++)
	{
		if (vector_get(format_str, i) == '%')
		{
			i++;
			const char32_t placeholder = (char32_t)vector_get(format_str, i);
			if (placeholder != '%')
			{
				if (args == MAX_PRINTF_ARGS)
				{
					parser_error(prs, too_many_printf_args, (size_t)MAX_PRINTF_ARGS);
					return 0;
				}

				placeholders[args] = placeholder;
			}
			switch (placeholder)
			{
				case 'i':
				case U'ц':
				case 'c':
				case U'л':
					format_types[args++] = TYPE_INTEGER;
					break;

				case 'f':
				case U'в':
					format_types[args++] = TYPE_FLOATING;
					break;

				case 's':
				case U'с':
					format_types[args++] = type_array(prs->sx, TYPE_INTEGER);
					break;

				case '%':
					break;

				case '\0':
					parser_error(prs, printf_no_format_placeholder);
					return 0;

				default:
					parser_error(prs, printf_unknown_format_placeholder, placeholder);
					return 0;
			}
		}
	}

	return args;
}

/**	Parse scanf statement [RuC] */
//static void parse_scanf_statement(parser *const prs, node *const parent);

/**	Parse printf statement [RuC] */
static void parse_printf_statement(parser *const prs, node *const parent)
{
	consume_token(prs); // kw_printf
	char32_t placeholders[MAX_PRINTF_ARGS];
	item_t format_types[MAX_PRINTF_ARGS];

	expect_and_consume(prs, TK_L_PAREN, no_leftbr_in_printf);
	node nd = node_add_child(parent, OP_PRINTF);

	if (prs->token != TK_STRING)
	{
		parser_error(prs, wrong_first_printf_param);
		skip_until(prs, TK_SEMICOLON);
		return;
	}

	const size_t expected_args = evaluate_args(prs, &prs->lxr.lexstr, format_types, placeholders);

	node_copy(&prs->sx->nd, &nd);
	parse_assignment_expression(prs);

	size_t actual_args = 0;
	while (try_consume_token(prs, TK_COMMA) && actual_args != expected_args)
	{
		node_copy(&prs->sx->nd, &nd);
		node expr = parse_assignment_expression(prs);
		check_assignment_operands(prs->sx, format_types[actual_args], &expr);
		// FIXME: кинуть другую ошибку
		actual_args++;
	}

	expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_printf);
	expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);

	if (actual_args != expected_args)
	{
		parser_error(prs, wrong_printf_param_number);
	}
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
 *	@param	parent		Parent node in AST
 */
static void parse_statement(parser *const prs, node *const parent)
{
	switch (prs->token)
	{
		case TK_SEMICOLON:
			consume_token(prs);
			node_add_child(parent, OP_NOP);
			return;

		case TK_CASE:
			parse_case_statement(prs, parent);
			return;
		case TK_DEFAULT:
			parse_default_statement(prs, parent);
			return;

		case TK_L_BRACE:
			parse_compound_statement(prs, parent, /*is_function_body=*/false);
			return;

		case TK_IF:
			parse_if_statement(prs, parent);
			return;
		case TK_SWITCH:
			parse_switch_statement(prs, parent);
			return;

		case TK_WHILE:
			parse_while_statement(prs, parent);
			return;
		case TK_DO:
			parse_do_statement(prs, parent);
			return;
		case TK_FOR:
			parse_for_statement(prs, parent);
			return;

		case TK_GOTO:
			parse_goto_statement(prs, parent);
			return;
		case TK_CONTINUE:
			parse_continue_statement(prs, parent);
			return;
		case TK_BREAK:
			parse_break_statement(prs, parent);
			return;
		case TK_RETURN:
			parse_return_statement(prs, parent);
			return;

		case TK_PRINTID:
			parse_printid_statement(prs, parent);
			return;
		case TK_PRINTF:
			parse_printf_statement(prs, parent);
			return;
		case TK_PRINT:
			parse_print_statement(prs, parent);
			return;
		case TK_GETID:
			parse_getid_statement(prs, parent);
			return;

		case TK_IDENTIFIER:
			if (peek_token(prs) == TK_COLON)
			{
				parse_labeled_statement(prs, parent);
				return;
			}

		default:
			parse_expression_statement(prs, parent);
			return;
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
				if (try_consume_token(prs, TK_IDENTIFIER))
				{
					was_ident = true;
					func_add(prs->sx, (item_t)prs->lxr.repr);
				}
			}
			else if (prs->token == TK_IDENTIFIER)
			{
				parser_error(prs, ident_in_declarator);
				skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
				return TYPE_UNDEFINED;
			}

			if (type_is_void(type) && prs->token != TK_L_PAREN)
			{
				parser_error(prs, par_type_void_with_nofun);
			}

			if (prs->token == TK_L_SQUARE)
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
					}
				}
			}

			if (try_consume_token(prs, TK_L_PAREN))
			{
				expect_and_consume(prs, TK_STAR, wrong_func_as_arg);
				if (prs->token == TK_IDENTIFIER)
				{
					if (level)
					{
						consume_token(prs);
						if (!was_ident)
						{
							was_ident = true;
						}
						else
						{
							parser_error(prs, two_idents_for_1_declarer);
							return TYPE_UNDEFINED;
						}
						func_add(prs->sx, -((item_t)prs->lxr.repr));
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
 *	Parse function body
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	function_id	Function number
 */
static void parse_function_body(parser *const prs, node *const parent, const size_t function_id)
{
	prs->function_mode = (size_t)ident_get_type(prs->sx, function_id);
	const size_t function_number = (size_t)ident_get_displ(prs->sx, function_id);
	const size_t param_number = (size_t)type_get(prs->sx, prs->function_mode + 2);

	vector_resize(&prs->labels, 0);
	prs->was_return = 0;

	const item_t prev = ident_get_prev(prs->sx, function_id);
	if (prev > 1 && prev != ITEM_MAX - 1) // Был прототип
	{
		if (prs->function_mode != (size_t)ident_get_type(prs->sx, (size_t)prev))
		{
			parser_error(prs, decl_and_def_have_diff_type);
			skip_until(prs, TK_R_BRACE);
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
		item_t type = type_get(prs->sx, prs->function_mode + i + 3);
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

	parse_compound_statement(prs, &nd, /*is_function_body=*/true);

	if (type_get(prs->sx, prs->function_mode + 1) != TYPE_VOID && !prs->was_return)
	{
		parser_error(prs, no_ret_in_func);
	}

	const item_t max_displ = scope_func_exit(prs->sx, old_displ);
	node_set_arg(&nd, 1, max_displ);

	for (size_t i = 0; i < vector_size(&prs->labels); i += 2)
	{
		const size_t repr = (size_t)ident_get_repr(prs->sx, (size_t)vector_get(&prs->labels, i));
		const size_t line_number = (size_t)llabs(vector_get(&prs->labels, i + 1));
		if (!ident_get_type(prs->sx, (size_t)vector_get(&prs->labels, i)))
		{
			parser_error(prs, label_not_declared, line_number, repr_get_name(prs->sx, repr));
		}
	}
}

/**
 *	Parse function definition [C99 6.9.1]
 *
 *	function-definition:
 *		declarator declaration-list[opt] compound-statement
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	type		Return type of a function
 */
static void parse_function_definition(parser *const prs, node *const parent, const item_t type)
{
	const size_t function_num = func_reserve(prs->sx);
	const size_t function_repr = prs->lxr.repr;

	consume_token(prs);
	const item_t function_mode = parse_function_declarator(prs, 1, 3, type);

	if (prs->func_def == 0 && prs->token == TK_L_BRACE)
	{
		prs->func_def = 1;
	}
	else if (prs->func_def == 0)
	{
		prs->func_def = 2;
	}

	const size_t function_id = to_identab(prs, function_repr, (item_t)function_num, function_mode);

	if (prs->token == TK_L_BRACE)
	{
		if (prs->func_def == 1)
		{
			parse_function_body(prs, parent, function_id);
		}
		else
		{
			parser_error(prs, func_decl_req_params);
			skip_until(prs, TK_R_BRACE);
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
		if (prs->token == TK_STAR)
		{
			consume_token(prs);
			type = type_pointer(prs->sx, group_type);
		}

		if (try_consume_token(prs, TK_IDENTIFIER))
		{
			if (prs->token == TK_L_PAREN)
			{
				parse_function_definition(prs, root, type);
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
	} while (prs->token != TK_EOF);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int parse(const workspace *const ws, syntax *const sx)
{
	if (!ws_is_correct(ws) || sx == NULL)
	{
		return -1;
	}

	parser prs = prs_create(ws, sx);
	node root = node_get_root(&sx->tree);

	parse_translation_unit(&prs, &root);

#ifndef NDEBUG
	write_tree(DEFAULT_TREE, sx);
#endif

	prs_clear(&prs);
	return !sx_is_correct(sx);
}
