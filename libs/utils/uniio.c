/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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

#include "uniio.h"
#include "workspace.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>

extern intptr_t _get_osfhandle(int fd);
#elif __APPLE__
    #include <fcntl.h>
#else
    #include <unistd.h>

    #define MAX_LINK_SIZE 20
#endif

#define MAX_FORMAT_SIZE 128


static inline bool is_specifier(const char ch)
{
    return (ch >= '0' && ch <= '9') || ch == 'h' || ch == 'l'                            // Sub-specifiers
           || ch == 'j' || ch == 'z' || ch == 't' || ch == 'L' || ch == '*' || ch == 'i' // Integer
           || ch == 'd' || ch == 'u'                                                     // Decimal integer
           || ch == 'o'                                                                  // Octal integer
           || ch == 'x' || ch == 'X'                                                     // Hexadecimal integer
           || ch == 'f' || ch == 'F'                                                     // Floating point number
           || ch == 'e' || ch == 'E' || ch == 'g' || ch == 'G' || ch == 'a' || ch == 'A' || ch == 'c' ||
           ch == 'C'                                                                     // Character
           || ch == 's' || ch == 'S'                                                     // String of characters
           || ch == 'p'                                                                  // Pointer address
           || ch == '['                                                                  // Scanset
           || ch == 'n';                                                                 // Count
}

static int scan_file_arg(universal_io *const io, const char *const format, const size_t size, void *arg)
{
    char buffer[MAX_FORMAT_SIZE];
    strncpy(buffer, format, size);
    sprintf(&buffer[size], "%%zn");

    size_t number = 0;
    int ret = fscanf(io->in_file, buffer, arg, &number);
    io->in_position += number;

    return number != 0 ? ret : 0;
}

static int scan_buffer_arg(universal_io *const io, const char *const format, const size_t size, void *arg)
{
    char buffer[MAX_FORMAT_SIZE];
    strncpy(buffer, format, size);
    sprintf(&buffer[size], "%%zn");

    size_t number = 0;
    int ret = sscanf(&io->in_buffer[io->in_position], buffer, arg, &number);
    io->in_position += number;

    return io->in_position < io->in_size || number != 0 ? ret : 0;
}

static inline int in_func_position(universal_io *const io, const char *const format, va_list args,
                                   int (*scan_arg)(universal_io *const, const char *const, const size_t, void *))
{
    const size_t position = io->in_position;
    int ret = 0;

    size_t last = 0;
    size_t i = 0;
    while (format[i] != '\0')
    {
        if (format[i] == '%')
        {
            bool was_count = false;

            while (is_specifier(format[i + 1]))
            {
                i++;
                was_count = was_count || format[i] == 'n';

                if (format[i] == '[')
                {
                    while (format[i] != '\0' && (format[i - 1] == '\\' || format[i] != ']'))
                    {
                        i++;
                    }
                }
            }

            if (format[i] != '%')
            {
                void *arg = va_arg(args, void *);
                ret += scan_arg(io, &format[last], i + 1, arg);

                if (io->in_position >= io->in_size)
                {
                    return ret;
                }

                if (was_count)
                {
                    *(size_t *)arg = io->in_position - position;
                }

                last = i + 1;
            }
        }

        i++;
    }

    return ret;
}


static int in_func_file(universal_io *const io, const char *const format, va_list args)
{
    return in_func_position(io, format, args, &scan_file_arg);
}

static int in_func_buffer(universal_io *const io, const char *const format, va_list args)
{
    return in_func_position(io, format, args, &scan_buffer_arg);
}

static int in_func_user(universal_io *const io, const char *const format, va_list args)
{
    return io->in_user_func(format, args);
}


static int out_func_file(universal_io *const io, const char *const format, va_list args)
{
    return vfprintf(io->out_file, format, args);
}

static int out_func_buffer(universal_io *const io, const char *const format, va_list args)
{
    va_list local;
    va_copy(local, args);

    int ret = vsnprintf(&io->out_buffer[io->out_position], io->out_size - io->out_position, format, local);

    if (ret != -1 && (size_t)ret + io->out_position < io->out_size)
    {
        io->out_position += (size_t)ret;
        return ret;
    }

    io->out_buffer[io->out_position] = '\0';

    char *new_buffer = realloc(io->out_buffer, 2 * io->out_size * sizeof(char));
    if (new_buffer == NULL)
    {
        return -1;
    }

    io->out_size *= 2;
    io->out_buffer = new_buffer;
    return out_func_buffer(io, format, args);
}

static int out_func_user(universal_io *const io, const char *const format, va_list args)
{
    return io->out_user_func(format, args);
}


static inline size_t io_get_path(FILE *const file, char *const buffer)
{
#ifdef _WIN32
    GetFinalPathNameByHandleA((HANDLE)_get_osfhandle(_fileno(file)), buffer, MAX_PATH, FILE_NAME_NORMALIZED);

    size_t ret = 0;
    while (buffer[ret + 4] != '\0')
    {
        buffer[ret] = buffer[ret + 4];
        ret++;
    }

    buffer[ret] = '\0';
    return ret;
#elif __APPLE__
    return fcntl(fileno(file), F_GETPATH, buffer) != -1 ? strlen(buffer) : 0;
#else
    char link[MAX_LINK_SIZE];
    sprintf(link, "/proc/self/fd/%d", fileno(file));
    int ret = readlink(link, buffer, 256);

    if (ret == -1)
    {
        buffer[0] = '\0';
        return -1;
    }

    buffer[ret] = '\0';
    return ret;
#endif
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


universal_io io_create(void)
{
    universal_io io;

    io.in_file = NULL;
    io.in_buffer = NULL;

    io.in_size = 0;
    io.in_position = 0;

    io.in_user_func = NULL;
    io.in_func = NULL;

    io.out_file = NULL;
    io.out_buffer = NULL;

    io.out_size = 0;
    io.out_position = 0;

    io.out_user_func = NULL;
    io.out_func = NULL;

    return io;
}


int in_set_file(universal_io *const io, const char *const path)
{
    if (path == NULL || in_clear(io))
    {
        return -1;
    }

    io->in_file = fopen(path, "r+b");
    if (io->in_file == NULL)
    {
        return -1;
    }

    io->in_position = 0;

    io->in_func = &in_func_file;

    return 0;
}

int in_set_buffer(universal_io *const io, const char *const buffer)
{
    if (buffer == NULL || in_clear(io))
    {
        return -1;
    }

    io->in_buffer = buffer;

    io->in_size = strlen(io->in_buffer);
    io->in_position = 0;

    io->in_func = &in_func_buffer;

    return 0;
}

int in_set_func(universal_io *const io, const io_user_func func)
{
    if (in_clear(io))
    {
        return -1;
    }

    io->in_user_func = func;
    io->in_func = &in_func_user;

    return 0;
}

int in_set_position(universal_io *const io, const size_t position)
{
    if (in_is_buffer(io))
    {
        if (position <= io->in_size)
        {
            io->in_position = position;
            return 0;
        }

        return -1;
    }

    if (in_is_file(io))
    {
        if ((position == 0 && fseek(io->in_file, 0, SEEK_SET) == 0) ||
            (fseek(io->in_file, (long)(position - 1), SEEK_SET) == 0 && fgetc(io->in_file) != EOF))
        {
            io->in_position = position;
            return 0;
        }

        fseek(io->in_file, (long)io->in_position, SEEK_SET);
        return -1;
    }

    return -1;
}

int in_swap(universal_io *const fst, universal_io *const snd)
{
    if (fst == NULL || snd == NULL)
    {
        return -1;
    }

    FILE *file = fst->in_file;
    fst->in_file = snd->in_file;
    snd->in_file = file;

    const char *buffer = fst->in_buffer;
    fst->in_buffer = snd->in_buffer;
    snd->in_buffer = buffer;

    const size_t size = fst->in_size;
    fst->in_size = snd->in_size;
    snd->in_size = size;

    const size_t position = fst->in_position;
    fst->in_position = snd->in_position;
    snd->in_position = position;

    const io_user_func user_func = fst->in_user_func;
    fst->in_user_func = snd->in_user_func;
    snd->in_user_func = user_func;

    const io_func func = fst->in_func;
    fst->in_func = snd->in_func;
    snd->in_func = func;

    return 0;
}


bool in_is_correct(const universal_io *const io)
{
    return io != NULL && (in_is_file(io) || in_is_buffer(io) || in_is_func(io));
}

bool in_is_file(const universal_io *const io)
{
    return io != NULL && io->in_file != NULL;
}

bool in_is_buffer(const universal_io *const io)
{
    return io != NULL && io->in_buffer != NULL;
}

bool in_is_func(const universal_io *const io)
{
    return io != NULL && io->in_user_func != NULL;
}


io_func in_get_func(const universal_io *const io)
{
    return io != NULL ? io->in_func : NULL;
}

size_t in_get_path(const universal_io *const io, char *const buffer)
{
    return in_is_file(io) ? io_get_path(io->in_file, buffer) : 0;
}

const char *in_get_buffer(const universal_io *const io)
{
    return in_is_buffer(io) ? io->in_buffer : NULL;
}

size_t in_get_position(const universal_io *const io)
{
    return in_is_buffer(io) || in_is_file(io) ? io->in_position : 0;
}


int in_close_file(universal_io *const io)
{
    if (!in_is_file(io))
    {
        return -1;
    }

    int ret = fclose(io->in_file);
    io->in_file = NULL;

    io->in_position = 0;

    return ret;
}

int in_clear(universal_io *const io)
{
    if (io == NULL)
    {
        return -1;
    }

    if (in_is_file(io))
    {
        in_close_file(io);
    }
    else if (in_is_buffer(io))
    {
        io->in_buffer = NULL;

        io->in_size = 0;
        io->in_position = 0;
    }
    else
    {
        io->in_user_func = NULL;
    }

    io->in_func = NULL;
    return 0;
}


int out_set_file(universal_io *const io, const char *const path)
{
    if (path == NULL || out_clear(io))
    {
        return -1;
    }

    io->out_file = fopen(path, "w+b");
    if (io->out_file == NULL)
    {
        return -1;
    }

    io->out_func = &out_func_file;

    return 0;
}

int out_set_buffer(universal_io *const io, const size_t size)
{
    if (out_clear(io))
    {
        return -1;
    }

    io->out_size = size + 1;
    io->out_buffer = malloc(io->out_size * sizeof(char));

    if (io->out_buffer == NULL)
    {
        io->out_size = 0;
        return -1;
    }

    io->out_buffer[0] = '\0';
    io->out_position = 0;

    io->out_func = &out_func_buffer;

    return 0;
}

int out_set_func(universal_io *const io, const io_user_func func)
{
    if (out_clear(io))
    {
        return -1;
    }

    io->out_user_func = func;
    io->out_func = &out_func_user;

    return 0;
}

int out_swap(universal_io *const fst, universal_io *const snd)
{
    if (fst == NULL || snd == NULL)
    {
        return -1;
    }

    FILE *file = fst->out_file;
    fst->out_file = snd->out_file;
    snd->out_file = file;

    char *buffer = fst->out_buffer;
    fst->out_buffer = snd->out_buffer;
    snd->out_buffer = buffer;

    const size_t size = fst->out_size;
    fst->out_size = snd->out_size;
    snd->out_size = size;

    const size_t position = fst->out_position;
    fst->out_position = snd->out_position;
    snd->out_position = position;

    const io_user_func user_func = fst->out_user_func;
    fst->out_user_func = snd->out_user_func;
    snd->out_user_func = user_func;

    const io_func func = fst->out_func;
    fst->out_func = snd->out_func;
    snd->out_func = func;

    return 0;
}


bool out_is_correct(const universal_io *const io)
{
    return io != NULL && (out_is_file(io) || out_is_buffer(io) || out_is_func(io));
}

bool out_is_file(const universal_io *const io)
{
    return io != NULL && io->out_file != NULL;
}

bool out_is_buffer(const universal_io *const io)
{
    return io != NULL && io->out_buffer != NULL;
}

bool out_is_func(const universal_io *const io)
{
    return io != NULL && io->out_user_func != NULL;
}


io_func out_get_func(const universal_io *const io)
{
    return io != NULL ? io->out_func : NULL;
}

size_t out_get_path(const universal_io *const io, char *const buffer)
{
    if (!out_is_file(io))
    {
        return 0;
    }

    return io_get_path(io->out_file, buffer);
}


char *out_extract_buffer(universal_io *const io)
{
    if (!out_is_buffer(io))
    {
        return NULL;
    }

    char *buffer = io->out_buffer;
    io->out_buffer = NULL;

    io->out_size = 0;
    io->out_position = 0;

    io->out_func = NULL;
    return buffer;
}

int out_close_file(universal_io *const io)
{
    if (!out_is_file(io))
    {
        return -1;
    }

    int ret = fclose(io->out_file);
    io->out_file = NULL;

    return ret;
}

int out_clear(universal_io *const io)
{
    if (io == NULL)
    {
        return -1;
    }

    if (out_is_file(io))
    {
        out_close_file(io);
    }
    else if (out_is_buffer(io))
    {
        free(out_extract_buffer(io));
    }
    else
    {
        io->out_user_func = NULL;
    }

    io->out_func = NULL;
    return 0;
}


int io_erase(universal_io *const io)
{
    return in_clear(io) || out_clear(io) ? -1 : 0;
}
