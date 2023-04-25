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
#include <assert.h>


static char32_t char32_from_cp866(const unsigned char symbol)
{
    if (symbol < 0x80)
    {
        return (char32_t)symbol;
    }

    if (symbol >= 0x80 && symbol <= 0xAF)
    {
        return (char32_t)symbol - 0x80 + U'А';
    }

    if (symbol >= 0xE0 && symbol <= 0xEF)
    {
        return (char32_t)symbol - 0xE0 + U'р';
    }

    switch (symbol)
    {
        case 0xB0:
            return U'░';
        case 0xB1:
            return U'▒';
        case 0xB2:
            return U'▓';
        case 0xB3:
            return U'│';
        case 0xB4:
            return U'┤';
        case 0xB5:
            return U'╡';
        case 0xB6:
            return U'╢';
        case 0xB7:
            return U'╖';
        case 0xB8:
            return U'╕';
        case 0xB9:
            return U'╣';
        case 0xBA:
            return U'║';
        case 0xBB:
            return U'╗';
        case 0xBC:
            return U'╝';
        case 0xBD:
            return U'╜';
        case 0xBE:
            return U'╛';
        case 0xBF:
            return U'┐';
        case 0xC0:
            return U'└';
        case 0xC1:
            return U'┴';
        case 0xC2:
            return U'┬';
        case 0xC3:
            return U'├';
        case 0xC4:
            return U'─';
        case 0xC5:
            return U'┼';
        case 0xC6:
            return U'╞';
        case 0xC7:
            return U'╟';
        case 0xC8:
            return U'╚';
        case 0xC9:
            return U'╔';
        case 0xCA:
            return U'╩';
        case 0xCB:
            return U'╦';
        case 0xCC:
            return U'╠';
        case 0xCD:
            return U'═';
        case 0xCE:
            return U'╬';
        case 0xCF:
            return U'╧';
        case 0xD0:
            return U'╨';
        case 0xD1:
            return U'╤';
        case 0xD2:
            return U'╥';
        case 0xD3:
            return U'╙';
        case 0xD4:
            return U'╘';
        case 0xD5:
            return U'╒';
        case 0xD6:
            return U'╓';
        case 0xD7:
            return U'╫';
        case 0xD8:
            return U'╪';
        case 0xD9:
            return U'┘';
        case 0xDA:
            return U'┌';
        case 0xDB:
            return U'█';
        case 0xDC:
            return U'▄';
        case 0xDD:
            return U'▌';
        case 0xDE:
            return U'▐';
        case 0xDF:
            return U'▀';

        case 0xF0:
            return U'Ё';
        case 0xF1:
            return U'ё';
        case 0xF2:
            return U'Є';
        case 0xF3:
            return U'є';
        case 0xF4:
            return U'Ї';
        case 0xF5:
            return U'ї';
        case 0xF6:
            return U'Ў';
        case 0xF7:
            return U'ў';

        case 0xF8:
            return U'°';
        case 0xF9:
            return U'∙';
        case 0xFA:
            return U'·';
        case 0xFB:
            return U'√';
        case 0xFC:
            return U'№';
        case 0xFD:
            return U'¤';
        case 0xFE:
            return U'■';
        case 0xFF:
            return 0xA0;

        default:
            return U'�';
    }
}

static char32_t char32_from_cp1251(const unsigned char symbol)
{
    if (symbol < 0x80)
    {
        return (char32_t)symbol;
    }

    if (symbol >= 0xC0) // && symbol <= 0xFF
    {
        return (char32_t)symbol - 0xC0 + U'А';
    }

    switch (symbol)
    {
        case 0x80:
            return U'Ђ';
        case 0x81:
            return U'Ѓ';
        case 0x82:
            return U'‚';
        case 0x83:
            return U'ѓ';
        case 0x84:
            return U'„';
        case 0x85:
            return U'…';
        case 0x86:
            return U'†';
        case 0x87:
            return U'‡';
        case 0x88:
            return U'€';
        case 0x89:
            return U'‰';
        case 0x8A:
            return U'Љ';
        case 0x8B:
            return U'‹';
        case 0x8C:
            return U'Њ';
        case 0x8D:
            return U'Ќ';
        case 0x8E:
            return U'Ћ';
        case 0x8F:
            return U'Џ';
        case 0x90:
            return U'ђ';
        case 0x91:
            return U'‘';
        case 0x92:
            return U'’';
        case 0x93:
            return U'“';
        case 0x94:
            return U'”';
        case 0x95:
            return U'•';
        case 0x96:
            return U'–';
        case 0x97:
            return U'—';

        case 0x99:
            return U'™';
        case 0x9A:
            return U'љ';
        case 0x9B:
            return U'›';
        case 0x9C:
            return U'њ';
        case 0x9D:
            return U'ќ';
        case 0x9E:
            return U'ћ';
        case 0x9F:
            return U'џ';

        case 0xA1:
            return U'Ў';
        case 0xA2:
            return U'ў';
        case 0xA3:
            return U'Ј';

        case 0xA5:
            return U'Ґ';

        case 0xA8:
            return U'Ё';

        case 0xAA:
            return U'Є';

        case 0xAF:
            return U'Ї';

        case 0xB2:
            return U'І';
        case 0xB3:
            return U'і';
        case 0xB4:
            return U'ґ';

        case 0xB8:
            return U'ё';
        case 0xB9:
            return U'№';
        case 0xBA:
            return U'є';

        case 0xBC:
            return U'ј';
        case 0xBD:
            return U'Ѕ';
        case 0xBE:
            return U'ѕ';
        case 0xBF:
            return U'ї';

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
            return (char32_t)symbol;

        default:
            return U'�';
    }
}

static inline size_t utf8_from_codepage(const char *const src, char *const dest,
                                        char32_t (*char_from_codepage)(const unsigned char))
{
    if (src == NULL || dest == NULL)
    {
        return 0;
    }

    size_t size = 0;
    for (size_t i = 0; src[i] != '\0'; i++)
    {
        size += utf8_to_string(&dest[size], char_from_codepage((unsigned char)(src[i])));
    }

    dest[size] = '\0';
    return size;
}


static char char_to_cp866(const char32_t symbol)
{
    if (symbol < 0x80)
    {
        return (char)symbol;
    }

    if (symbol >= U'А' && symbol <= U'п')
    {
        return (char)(symbol - U'А' + 0x80);
    }

    if (symbol >= U'р' && symbol <= U'я')
    {
        return (char)(symbol - U'р' + 0xE0);
    }

    switch (symbol)
    {
        case U'░':
            return '\xB0';
        case U'▒':
            return '\xB1';
        case U'▓':
            return '\xB2';
        case U'│':
            return '\xB3';
        case U'┤':
            return '\xB4';
        case U'╡':
            return '\xB5';
        case U'╢':
            return '\xB6';
        case U'╖':
            return '\xB7';
        case U'╕':
            return '\xB8';
        case U'╣':
            return '\xB9';
        case U'║':
            return '\xBA';
        case U'╗':
            return '\xBB';
        case U'╝':
            return '\xBC';
        case U'╜':
            return '\xBD';
        case U'╛':
            return '\xBE';
        case U'┐':
            return '\xBF';
        case U'└':
            return '\xC0';
        case U'┴':
            return '\xC1';
        case U'┬':
            return '\xC2';
        case U'├':
            return '\xC3';
        case U'─':
            return '\xC4';
        case U'┼':
            return '\xC5';
        case U'╞':
            return '\xC6';
        case U'╟':
            return '\xC7';
        case U'╚':
            return '\xC8';
        case U'╔':
            return '\xC9';
        case U'╩':
            return '\xCA';
        case U'╦':
            return '\xCB';
        case U'╠':
            return '\xCC';
        case U'═':
            return '\xCD';
        case U'╬':
            return '\xCE';
        case U'╧':
            return '\xCF';
        case U'╨':
            return '\xD0';
        case U'╤':
            return '\xD1';
        case U'╥':
            return '\xD2';
        case U'╙':
            return '\xD3';
        case U'╘':
            return '\xD4';
        case U'╒':
            return '\xD5';
        case U'╓':
            return '\xD6';
        case U'╫':
            return '\xD7';
        case U'╪':
            return '\xD8';
        case U'┘':
            return '\xD9';
        case U'┌':
            return '\xDA';
        case U'█':
            return '\xDB';
        case U'▄':
            return '\xDC';
        case U'▌':
            return '\xDD';
        case U'▐':
            return '\xDE';
        case U'▀':
            return '\xDF';

        case U'Ё':
            return '\xF0';
        case U'ё':
            return '\xF1';
        case U'Є':
            return '\xF2';
        case U'є':
            return '\xF3';
        case U'Ї':
            return '\xF4';
        case U'ї':
            return '\xF5';
        case U'Ў':
            return '\xF6';
        case U'ў':
            return '\xF7';

        case U'°':
            return '\xF8';
        case U'∙':
            return '\xF9';
        case U'·':
            return '\xFA';
        case U'√':
            return '\xFB';
        case U'№':
            return '\xFC';
        case U'¤':
            return '\xFD';
        case U'■':
            return '\xFE';
        case 0xA0:
            return '\xFF';

        default:
            return '\x1A';
    }
}

static char char_to_cp1251(const char32_t symbol)
{
    if (symbol < 0x80)
    {
        return (char)symbol;
    }

    if (symbol >= U'А' && symbol <= U'я')
    {
        return (char)(symbol - U'А' + 0xC0);
    }

    switch (symbol)
    {
        case U'Ђ':
            return '\x80';
        case U'Ѓ':
            return '\x81';
        case U'‚':
            return '\x82';
        case U'ѓ':
            return '\x83';
        case U'„':
            return '\x84';
        case U'…':
            return '\x85';
        case U'†':
            return '\x86';
        case U'‡':
            return '\x87';
        case U'€':
            return '\x88';
        case U'‰':
            return '\x89';
        case U'Љ':
            return '\x8A';
        case U'‹':
            return '\x8B';
        case U'Њ':
            return '\x8C';
        case U'Ќ':
            return '\x8D';
        case U'Ћ':
            return '\x8E';
        case U'Џ':
            return '\x8F';
        case U'ђ':
            return '\x90';
        case U'‘':
            return '\x91';
        case U'’':
            return '\x92';
        case U'“':
            return '\x93';
        case U'”':
            return '\x94';
        case U'•':
            return '\x95';
        case U'–':
            return '\x96';
        case U'—':
            return '\x97';

        case U'™':
            return '\x99';
        case U'љ':
            return '\x9A';
        case U'›':
            return '\x9B';
        case U'њ':
            return '\x9C';
        case U'ќ':
            return '\x9D';
        case U'ћ':
            return '\x9E';
        case U'џ':
            return '\x9F';

        case U'Ў':
            return '\xA1';
        case U'ў':
            return '\xA2';
        case U'Ј':
            return '\xA3';

        case U'Ґ':
            return '\xA5';

        case U'Ё':
            return '\xA8';

        case U'Є':
            return '\xAA';

        case U'Ї':
            return '\xAF';

        case U'І':
            return '\xB2';
        case U'і':
            return '\xB3';
        case U'ґ':
            return '\xB4';

        case U'ё':
            return '\xB8';
        case U'№':
            return '\xB9';
        case U'є':
            return '\xBA';

        case U'ј':
            return '\xBC';
        case U'Ѕ':
            return '\xBD';
        case U'ѕ':
            return '\xBE';
        case U'ї':
            return '\xBF';

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
            return '\x1A';
    }
}

static inline size_t utf8_to_codepage(const char *const src, char *const dest, char (*char_to_codepage)(const char32_t))
{
    if (src == NULL || dest == NULL)
    {
        return 0;
    }

    size_t size = 0;
    for (size_t i = 0; src[i] != '\0'; i += utf8_symbol_size(src[i]))
    {
        dest[size++] = char_to_codepage(utf8_convert(&src[i]));
    }

    dest[size] = '\0';
    return size;
}


static size_t char_transliteration(char *const buffer, const char32_t symbol)
{
    switch (symbol)
    {
        case U'А':
            buffer[0] = 'A';
            return 1;
        case U'а':
            buffer[0] = 'a';
            return 1;
        case U'Б':
            buffer[0] = 'B';
            return 1;
        case U'б':
            buffer[0] = 'b';
            return 1;
        case U'В':
            buffer[0] = 'V';
            return 1;
        case U'в':
            buffer[0] = 'v';
            return 1;
        case U'Г':
            buffer[0] = 'G';
            return 1;
        case U'г':
            buffer[0] = 'g';
            return 1;
        case U'Д':
            buffer[0] = 'D';
            return 1;
        case U'д':
            buffer[0] = 'd';
            return 1;
        case U'З':
            buffer[0] = 'Z';
            return 1;
        case U'з':
            buffer[0] = 'z';
            return 1;
        case U'И':
        case U'Й':
            buffer[0] = 'I';
            return 1;
        case U'и':
        case U'й':
            buffer[0] = 'i';
            return 1;
        case U'К':
            buffer[0] = 'K';
            return 1;
        case U'к':
            buffer[0] = 'k';
            return 1;
        case U'Л':
            buffer[0] = 'L';
            return 1;
        case U'л':
            buffer[0] = 'l';
            return 1;
        case U'М':
            buffer[0] = 'M';
            return 1;
        case U'м':
            buffer[0] = 'm';
            return 1;
        case U'Н':
            buffer[0] = 'N';
            return 1;
        case U'н':
            buffer[0] = 'n';
            return 1;
        case U'О':
            buffer[0] = 'O';
            return 1;
        case U'о':
            buffer[0] = 'o';
            return 1;
        case U'П':
            buffer[0] = 'P';
            return 1;
        case U'п':
            buffer[0] = 'p';
            return 1;
        case U'Р':
            buffer[0] = 'R';
            return 1;
        case U'р':
            buffer[0] = 'r';
            return 1;
        case U'С':
            buffer[0] = 'S';
            return 1;
        case U'с':
            buffer[0] = 's';
            return 1;
        case U'Т':
            buffer[0] = 'T';
            return 1;
        case U'т':
            buffer[0] = 't';
            return 1;
        case U'У':
            buffer[0] = 'U';
            return 1;
        case U'у':
            buffer[0] = 'u';
            return 1;
        case U'Ф':
            buffer[0] = 'F';
            return 1;
        case U'ф':
            buffer[0] = 'f';
            return 1;
        case U'Ы':
            buffer[0] = 'Y';
            return 1;
        case U'ы':
            buffer[0] = 'y';
            return 1;
        case U'Е':
        case U'Ё':
        case U'Э':
            buffer[0] = 'E';
            return 1;
        case U'е':
        case U'ё':
        case U'э':
            buffer[0] = 'e';
            return 1;

        case U'Ж':
            buffer[0] = 'Z';
            buffer[1] = 'h';
            return 2;
        case U'ж':
            buffer[0] = 'z';
            buffer[1] = 'h';
            return 2;
        case U'Х':
            buffer[0] = 'K';
            buffer[1] = 'h';
            return 2;
        case U'х':
            buffer[0] = 'k';
            buffer[1] = 'h';
            return 2;
        case U'Ц':
            buffer[0] = 'T';
            buffer[1] = 's';
            return 2;
        case U'ц':
            buffer[0] = 't';
            buffer[1] = 's';
            return 2;
        case U'Ч':
            buffer[0] = 'C';
            buffer[1] = 'h';
            return 2;
        case U'ч':
            buffer[0] = 'c';
            buffer[1] = 'h';
            return 2;
        case U'Ш':
            buffer[0] = 'S';
            buffer[1] = 'h';
            return 2;
        case U'ш':
            buffer[0] = 's';
            buffer[1] = 'h';
            return 2;
        case U'Ъ':
            buffer[0] = 'I';
            buffer[1] = 'e';
            return 2;
        case U'ъ':
            buffer[0] = 'i';
            buffer[1] = 'e';
            return 2;
        case U'Ю':
            buffer[0] = 'I';
            buffer[1] = 'u';
            return 2;
        case U'ю':
            buffer[0] = 'i';
            buffer[1] = 'u';
            return 2;
        case U'Я':
            buffer[0] = 'I';
            buffer[1] = 'a';
            return 2;
        case U'я':
            buffer[0] = 'i';
            buffer[1] = 'a';
            return 2;

        case U'Щ':
            buffer[0] = 'S';
            buffer[1] = 'h';
            buffer[2] = 'c';
            buffer[3] = 'h';
            return 4;
        case U'щ':
            buffer[0] = 's';
            buffer[1] = 'h';
            buffer[2] = 'c';
            buffer[3] = 'h';
            return 4;

        case U'Ь':
        case U'ь':
            return 0;

        default:
            return utf8_to_string(buffer, symbol);
    }
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


size_t utf8_size(const char32_t symbol)
{
    if ((symbol & 0xFFFFFF80) == 0)
    {
        return 1;
    }

    if ((symbol & 0xFFFFF800) == 0)
    {
        return 2;
    }

    if ((symbol & 0xFFFF0000) == 0)
    {
        return 3;
    }

    if ((symbol & 0xFFE00000) == 0)
    {
        return 4;
    }

    return 0;
}

size_t utf8_symbol_size(const char symbol)
{
    if ((symbol & 0x80 /* 0b10000000 */) == 0x00 /* 0b00000000 */)
    {
        return 1;
    }

    if ((symbol & 0xE0 /* 0b11100000 */) == 0xC0 /* 0b11000000 */)
    {
        return 2;
    }

    if ((symbol & 0xF0 /* 0b11110000 */) == 0xE0 /* 0b11100000 */)
    {
        return 3;
    }

    if ((symbol & 0xF8 /* 0b11111000 */) == 0xF0 /* 0b11110000 */)
    {
        return 4;
    }

    return 1;
}

size_t utf8_to_first_byte(const char *const str, const size_t index)
{
    if (str == NULL)
    {
        return 0;
    }

    return (str[index] & 0xC0 /* 0b11000000 */) == 0x80 /* 0b10000000 */
               ? utf8_to_first_byte(str, index - 1)
               : index;
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
        buffer[i] = '\x80' /* 0b10000000 */;
        buffer[i] |= (char)((symbol & mask) >> (6 * (octets - i - 1)));
        mask >>= 6;
    }

    switch (octets)
    {
        case 2:
            buffer[0] |= 0xC0 /* 0b11000000 */;
            break;

        case 3:
            buffer[0] |= 0xE0 /* 0b11100000 */;
            break;

        case 4:
            buffer[0] |= 0xF0 /* 0b11110000 */;
            break;
    }

    buffer[octets] = '\0';
    return octets;
}

size_t utf8_from_cp866(const char *const src, char *const dest)
{
    return utf8_from_codepage(src, dest, &char32_from_cp866);
}

size_t utf8_from_cp1251(const char *const src, char *const dest)
{
    return utf8_from_codepage(src, dest, &char32_from_cp1251);
}

size_t utf8_to_cp866(const char *const src, char *const dest)
{
    return utf8_to_codepage(src, dest, &char_to_cp866);
}

size_t utf8_to_cp1251(const char *const src, char *const dest)
{
    return utf8_to_codepage(src, dest, &char_to_cp1251);
}


size_t utf8_transliteration(const char *const src, char *const dest)
{
    if (src == NULL || dest == NULL)
    {
        return 0;
    }

    size_t size = 0;
    for (size_t i = 0; src[i] != '\0'; i += utf8_symbol_size(src[i]))
    {
        size += char_transliteration(&dest[size], utf8_convert(&src[i]));
    }

    dest[size] = '\0';
    return size;
}

char32_t utf8_to_upper(const char32_t symbol)
{
    if (symbol >= 'a' && symbol <= 'z')
    {
        return symbol + (char32_t)('A' - 'a');
    }

    if (symbol >= U'а' && symbol <= U'я')
    {
        return symbol + (U'А' - U'а');
    }

    return symbol;
}

uint8_t utf8_to_number(const char32_t symbol)
{
    if (utf8_is_digit(symbol))
    {
        return (uint8_t)(symbol - '0');
    }

    if (symbol >= 'A' && symbol <= 'F')
    {
        return (uint8_t)(symbol - 'A' + 10);
    }

    if (symbol >= 'a' && symbol <= 'f')
    {
        return (uint8_t)(symbol - 'a' + 10);
    }

    return 0;
}

bool utf8_is_russian(const char32_t symbol)
{
    return symbol == U'Ё' || symbol == U'ё' || (symbol >= U'А' && symbol <= U'Я') ||
           (symbol >= U'а' && symbol <= U'п') || (symbol >= U'р' && symbol <= U'я');
}

bool utf8_is_letter(const char32_t symbol)
{
    return utf8_is_russian(symbol) || symbol == '_' || (symbol >= 'A' && symbol <= 'Z') ||
           (symbol >= 'a' && symbol <= 'z');
}

bool utf8_is_digit(const char32_t symbol)
{
    return symbol >= '0' && symbol <= '9';
}

bool utf8_is_hexa_digit(const char32_t symbol)
{
    return utf8_is_digit(symbol) || (symbol >= 'A' && symbol <= 'F') || (symbol >= 'a' && symbol <= 'f');
}

bool utf8_is_power(const char32_t symbol)
{
    return symbol == 'e' || symbol == 'E' || symbol == U'е' || symbol == U'Е';
}
