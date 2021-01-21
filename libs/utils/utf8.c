/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev
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

#include "utf8.h"


char char_to_cp866(const char32_t symbol)
{
	if (symbol >= U'А' && symbol <= U'Я')
	{
		return (char)(symbol - U'А' + 0x80);
	}

	if (symbol >= U'а' && symbol <= U'п')
	{
		return (char)(symbol - U'а' + 0xA0);
	}

	if (symbol >= U'р' && symbol <= U'я')
	{
		return (char)(symbol - U'р' + 0xE0);
	}

	switch (symbol)
	{
		case U'░':
			return 0xB0;
		case U'▒':
			return 0xB1;
		case U'▓':
			return 0xB2;
		case U'│':
			return 0xB3;
		case U'┤':
			return 0xB4;
		case U'╡':
			return 0xB5;
		case U'╢':
			return 0xB6;
		case U'╖':
			return 0xB7;
		case U'╕':
			return 0xB8;
		case U'╣':
			return 0xB9;
		case U'║':
			return 0xBA;
		case U'╗':
			return 0xBB;
		case U'╝':
			return 0xBC;
		case U'╜':
			return 0xBD;
		case U'╛':
			return 0xBE;
		case U'┐':
			return 0xBF;
		case U'└':
			return 0xC0;
		case U'┴':
			return 0xC1;
		case U'┬':
			return 0xC2;
		case U'├':
			return 0xC3;
		case U'─':
			return 0xC4;
		case U'┼':
			return 0xC5;
		case U'╞':
			return 0xC6;
		case U'╟':
			return 0xC7;
		case U'╚':
			return 0xC8;
		case U'╔':
			return 0xC9;
		case U'╩':
			return 0xCA;
		case U'╦':
			return 0xCB;
		case U'╠':
			return 0xCC;
		case U'═':
			return 0xCD;
		case U'╬':
			return 0xCE;
		case U'╧':
			return 0xCF;
		case U'╨':
			return 0xD0;
		case U'╤':
			return 0xD1;
		case U'╥':
			return 0xD2;
		case U'╙':
			return 0xD3;
		case U'╘':
			return 0xD4;
		case U'╒':
			return 0xD5;
		case U'╓':
			return 0xD6;
		case U'╫':
			return 0xD7;
		case U'╪':
			return 0xD8;
		case U'┘':
			return 0xD9;
		case U'┌':
			return 0xDA;
		case U'█':
			return 0xDB;
		case U'▄':
			return 0xDC;
		case U'▌':
			return 0xDD;
		case U'▐':
			return 0xDE;
		case U'▀':
			return 0xDF;

		case U'Ё':
			return 0xF0;
		case U'ё':
			return 0xF1;
		case U'Є':
			return 0xF2;
		case U'є':
			return 0xF3;
		case U'Ї':
			return 0xF4;
		case U'ї':
			return 0xF5;
		case U'Ў':
			return 0xF6;
		case U'ў':
			return 0xF7;

		case U'°':
			return 0xF8;
		case U'∙':
			return 0xF9;
		case U'·':
			return 0xFA;
		case U'√':
			return 0xFB;
		case U'№':
			return 0xBC;
		case U'¤':
			return 0xFD;
		case U'■':
			return 0xFE;
		case 0xA0:
			return 0xFF;

		default:
			return 0xFE;
	}
}

char char_to_cp1251(const char32_t symbol)
{
	if (symbol >= U'А' && symbol <= U'Я')
	{
		return (char)(symbol - U'А' + 0xC0);
	}

	if (symbol >= U'а' && symbol <= U'п')
	{
		return (char)(symbol - U'а' + 0xE0);
	}

	if (symbol >= U'р' && symbol <= U'я')
	{
		return (char)(symbol - U'р' + 0xF0);
	}

	switch (symbol)
	{
		case U'Ђ':
			return 0x80;
		case U'Ѓ':
			return 0x81;
		case U'‚':
			return 0x82;
		case U'ѓ':
			return 0x83;
		case U'„':
			return 0x84;
		case U'…':
			return 0x85;
		case U'†':
			return 0x86;
		case U'‡':
			return 0x87;
		case U'€':
			return 0x88;
		case U'‰':
			return 0x89;
		case U'Љ':
			return 0x8A;
		case U'‹':
			return 0x8B;
		case U'Њ':
			return 0x8C;
		case U'Ќ':
			return 0x8D;
		case U'Ћ':
			return 0x8E;
		case U'Џ':
			return 0x8F;
		case U'ђ':
			return 0x90;
		case U'‘':
			return 0x91;
		case U'’':
			return 0x92;
		case U'“':
			return 0x93;
		case U'”':
			return 0x94;
		case U'•':
			return 0x95;
		case U'–':
			return 0x96;
		case U'—':
			return 0x97;

		case U'™':
			return 0x99;
		case U'љ':
			return 0x9A;
		case U'›':
			return 0x9B;
		case U'њ':
			return 0x9C;
		case U'ќ':
			return 0x9D;
		case U'ћ':
			return 0x9E;
		case U'џ':
			return 0x9F;

		case U'Ў':
			return 0xA1;
		case U'ў':
			return 0xA2;
		case U'Ј':
			return 0xA3;

		case U'Ґ':
			return 0xA5;

		case U'Ё':
			return 0xA8;

		case U'Є':
			return 0xAA;

		case U'Ї':
			return 0xAF;

		case U'І':
			return 0xB2;
		case U'і':
			return 0xB3;
		case U'ґ':
			return 0xB4;

		case U'ё':
			return 0xB8;
		case U'№':
			return 0xB9;
		case U'є':
			return 0xBA;

		case U'ј':
			return 0xBC;
		case U'Ѕ':
			return 0xBD;
		case U'ѕ':
			return 0xBE;
		case U'ї':
			return 0xBF;

		case 0xA0:
		case U'¤':
		case U'¦':
		case U'§':
		case U'©':
		case U'«':
		case U'¬':
		case 0xAD:
		case U'®':
		case U'°':
		case U'±':
		case U'µ':
		case U'¶':
		case U'·':
		case U'»':
			return (char)symbol;

		default:
			return 0x98;
	}
}

size_t utf8_to_codepage(const char *const src, char *const dest, char (*char_to_codepage)(const char32_t))
{
	if (src == NULL || dest == NULL)
	{
		return 0;
	}

	size_t i = 0;
	size_t j = 0;
	while (src[i] != '\0')
	{
		const size_t size = utf8_symbol_size(src[i]);
		if (size == 1)
		{
			dest[j++] = src[i];
		}
		else
		{
			dest[j++] = char_to_codepage(utf8_convert(&src[i]));
		}

		i += size;
	}

	dest[j] = '\0';
	return j;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


size_t utf8_symbol_size(const char symbol)
{
	if ((symbol & 0b10000000) == 0b00000000)
	{
		return 1;
	}

	if ((symbol & 0b11100000) == 0b11000000)
	{
		return 2;
	}

	if ((symbol & 0b11110000) == 0b11100000)
	{
		return 3;
	}
	
	if ((symbol & 0b11111000) == 0b11110000)
	{
		return 4;
	}

	return 1;
}

size_t utf8_to_first_byte(const char *const str, const size_t index)
{
	return str == NULL ? 0
		: (str[index] & 0b11000000) == 0b10000000
			? utf8_to_first_byte(str, index - 1) : index;
}

char32_t utf8_convert(const char *const symbol)
{
	if (symbol == NULL)
	{
		return 0;
	}

	const size_t size = utf8_symbol_size(symbol[0]);
	char32_t result = 0x000000FF & symbol[0];

	switch (size)
	{
		case 2:
			result &= 0x0000001F /* 0b00011111 */;
			break;

		case 3:
			result &= 0x0000000F /* 0b00001111 */;
			break;

		case 4:
			result &= 0x00000007 /* 0b00000111 */;
			break;
	}

	for (size_t i = 1; i < size; i++)
	{
		result <<= 6;
		result |= 0x0000003F /* 0b00111111 */ & symbol[i];
	}

	return result;	
}

size_t utf8_to_string(char *const buffer, const char32_t symbol)
{
	if (buffer == NULL || (symbol & 0xFFE00000) != 0x00000000)
	{
		return 0;
	}
	
	if ((symbol & 0xFFFFFF80) == 0x00000000)
	{
		buffer[0] = (char)symbol;
		buffer[1] = '\0';
		return 1;
	}

	char32_t mask = 0x00FC0000;
	size_t octets = 4;
	while ((symbol & mask) == 0 && octets > 1)
	{
		mask >>= 6;
		octets--;
	}

	for (size_t i = 0; i < octets; i++)
	{
		buffer[i] = 0b10000000;
		buffer[i] |= (char)((symbol & mask) >> (6 * (octets - i - 1)));
		mask >>= 6;
	}

	switch (octets)
	{
		case 2:
			buffer[0] |= 0b11000000;
			break;

		case 3:
			buffer[0] |= 0b11100000;
			break;

		case 4:
			buffer[0] |= 0b11110000;
			break;
	}

	buffer[octets] = '\0';
	return octets;
}

size_t utf8_to_cp866(const char *const src, char *const dest)
{
	return utf8_to_codepage(src, dest, &char_to_cp866);
}

size_t utf8_to_cp1251(const char *const src, char *const dest)
{
	return utf8_to_codepage(src, dest, &char_to_cp1251);
}

int utf8_is_russian(const char32_t symbol)
{
	return  symbol == U'Ё' || symbol == U'ё'
		|| (symbol >= U'А' && symbol <= U'Я')
		|| (symbol >= U'а' && symbol <= U'п')
		|| (symbol >= U'р' && symbol <= U'я');
}

int utf8_is_letter(const char32_t symbol)
{
	return  utf8_is_russian(symbol) || symbol == '_'
		|| (symbol >= 'A' && symbol <= 'Z')
		|| (symbol >= 'a' && symbol <= 'z');
}

int utf8_is_digit(const char32_t symbol)
{
	return symbol >= '0' && symbol <= '9';
}

int utf8_is_power(const char32_t symbol)
{
	return symbol == 'e' || symbol == 'E' || symbol == U'е' || symbol == U'Е';
}
