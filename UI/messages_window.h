#pragma once

#include <FL/Fl_Text_Editor.H>
#include <string>

class Face_Window;

class Messages_Window : public Fl_Text_Editor
{
public:
    Messages_Window(Face_Window *face, int x, int y, int w, int h);

    void put_message(const std::string &message);
    void put_char(int ch);
    void put_char_straight(char c);
    void put_string_straight(char *c);

    void put_file(const std::string &fname);
    void clear();

private:
    enum State
    {
        IDLE,
        WRITING
    };

    static void modify_cb(int         pos,
                          int         nins,
                          int         ndel,
                          int         nrestyled,
                          const char *c_del,
                          void *      userinf);

    Fl_Text_Buffer buff;
    Face_Window *  win;

    State state;
};
