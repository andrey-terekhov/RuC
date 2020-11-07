#include "utf_8.h"


size_t symbol_size(const char symbol)
{
	if ((symbol & 0b11110000) == 0b11110000)
	{
		return 4;
	}

	if ((symbol & 0b11100000) == 0b11100000)
	{
		return 3;
	}
	
	if ((symbol & 0b11000000) == 0b11000000)
	{
		return 2;
	}

	if ((symbol & 0b10000000) == 0b00000000)
	{
		return 1;
	}

	return 0;
}

char32_t to_utf_8(const char *const symbol)
{
	if (symbol == NULL)
	{
		return 0;
	}

	char32_t result = 0x00000000;
	for (size_t i = 0; i < symbol_size(symbol[0]); i++)
	{
		result <<= 8;
		result |= 0x000000FF & symbol[i];
	}

	return result;	
}

size_t to_string(char *const buffer, const char32_t symbol)
{
	char32_t mask = 0xFF000000;
	size_t octets = 4;
	while ((symbol & mask) == 0 & octets > 0)
	{
		mask >>= 8;
		octets--;
	}

	for (size_t i = 0; i < octets; i++)
	{
		buffer[i] = (symbol & mask) >> (8 * (octets - i - 1));
		mask >>= 8;
	}

	buffer[octets] = '\0';
	return octets;
}

/*int is_russian(const char32_t symbol)
{
	return  symbol == 'Ё' || symbol == 'ё'
		|| (symbol >= 'А' && symbol <= 'Я')
		|| (symbol >= 'а' && symbol <= 'п')
		|| (symbol >= 'р' && symbol <= 'я');
}*/
