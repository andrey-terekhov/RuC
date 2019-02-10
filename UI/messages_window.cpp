#include "messages_window.h"
#include "face_defaults.h"
#include "face_util.h"
#include "face_window.h"

Messages_Window::Messages_Window(Face_Window *win, int x, int y, int w, int h)
    : Fl_Text_Editor(x, y, w, h), win(win)
{
    buffer(buff);
    buff.append("Сообщения компилятора:\n");
    buff.add_modify_callback(modify_cb, this);
    hide_cursor();
}

void Messages_Window::put_message(const std::string &str)
{
    State_Holder<State> sh(state);
    state = WRITING;

    buff.append(str.c_str());

    scroll(buff.count_lines(0, buff.length()), 0);
    Fl::check();
}

void Messages_Window::put_char(int ch)
{
    State_Holder<State> sh(state);
    state = WRITING;

    if (ch < 128)
    {
        char cc = char(ch);
        put_message(std::string() + cc);
    }
    else
    {
        char first = (ch >> 6) | 0xC0;
        char second = (ch & 0x3F) | 0x80;

        put_message(std::string() + first + second);
    }

    Fl::check();
}

void Messages_Window::put_char_straight(char c)
{
    State_Holder<State> sh(state);
    state = WRITING;

    char cc[] = {c, '\0'};
    buff.append(cc);
    if (c == '\n')
        scroll(buff.count_lines(0, buff.length()), 0);
}

void Messages_Window::put_string_straight(char *c)
{
    State_Holder<State> sh(state);
    state = WRITING;

    buff.append(c);
    scroll(buff.count_lines(0, buff.length()), 0);
}

void Messages_Window::put_file(const std::string &fname)
{
    State_Holder<State> sh(state);
    state = WRITING;

    int err = buff.appendfile(fname.c_str());
    if (err)
    {
        put_message("in Messages_Window::put_file: ");
        put_message(strerror(err));
        put_message("\n");
        put_message("file: ");
        put_message(fname);
        put_message("\n");
        return;
    }

    scroll(buff.count_lines(0, buff.length()), 0);
    Fl::check();
}

void Messages_Window::clear()
{
    State_Holder<State> sh(state);
    state = WRITING;

    buff.remove(0, std::string(buff.text()).size());
    Fl::check();
}

void Messages_Window::modify_cb(int pos, int nins, int ndel, int nrestyled,
                                const char *c_del, void *messages_window_voided)
{
    Messages_Window *messages_window =
        reinterpret_cast<Messages_Window *>(messages_window_voided);
    if (messages_window->state == WRITING)
        return;

    State_Holder<State> sh(messages_window->state);
    messages_window->state = WRITING;

    if (nins > 0)
        messages_window->buff.remove(pos, pos + nins);

    if (ndel > 0)
        messages_window->buff.insert(pos, c_del);
}
