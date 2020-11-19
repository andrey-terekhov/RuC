#include "utf8.h"


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
	if ((symbol & 0xFFE00000) != 0x00000000)
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

int utf8_is_russian(const char32_t symbol)
{
	return  symbol == U'Ё' || symbol == U'ё'
		|| (symbol >= U'А' && symbol <= U'Я')
		|| (symbol >= U'а' && symbol <= U'п')
		|| (symbol >= U'р' && symbol <= U'я');
}
