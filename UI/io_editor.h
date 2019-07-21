#pragma once

#include <FL/Fl_Text_Editor.H>
#include <string>
#include "messages_window.h"

class Face_Window;

class Io_Editor : public Fl_Text_Editor
{
public:
    Io_Editor(Face_Window *win, int x, int y, int w, int h);

    void set_active();
    void bind_stream(FILE *new_stream);
    void unbind_stream();

    void write(const std::string &str);
    void write(char ch);
    void write_file(const std::string &fname);

    void clear();

private:
    enum State
    {
        IDLE,
        MODIFYING
    };

    static void modify_cb(int         pos,
                          int         nins,
                          int         ndel,
                          int         nrestyled,
                          const char *c_del,
                          void *      io_editor_voided);

    int io_block = 0; // before this position nothing can be changed in io
    Fl_Text_Buffer   buff;
    State            state;
    Messages_Window *messages_window;
    FILE *           stream;
};
