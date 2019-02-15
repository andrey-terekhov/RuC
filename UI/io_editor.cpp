#include "io_editor.h"
#include "face_defaults.h"
#include "face_util.h"
#include "face_window.h"

Io_Editor::Io_Editor(Face_Window *win, int x, int y, int w, int h)
    : Fl_Text_Editor(x, y, w, h), messages_window(win->get_messages_window()), state(IDLE)
{
    buffer(buff);
    buff.append("Ввод/вывод\n");
    insert_position(buff.length());
    buff.add_modify_callback(modify_cb, this);
}

void Io_Editor::set_active()
{
    io_block = 0;

    State_Holder<State> sh(state);
    state = MODIFYING;

    activate();
    show_cursor();
    take_focus();
}

void Io_Editor::bind_stream(FILE *new_stream) { stream = new_stream; }

void Io_Editor::unbind_stream()
{
    stream = NULL;
    io_block = buff.length();
}

void Io_Editor::write(const std::string &str)
{
    State_Holder<State> sh(state);
    state = MODIFYING;

    buff.append(str.c_str());
    insert_position(buff.length());

    io_block = buff.length();

    scroll(buff.count_lines(0, buff.length()), 0);
    Fl::check();
}

void Io_Editor::write(char ch)
{
    State_Holder<State> sh(state);
    state = MODIFYING;

    const char str[] = {ch, '\0'};
    buff.append(str);
    io_block = buff.length();

    scroll(buff.count_lines(0, buff.length()), 0);
    Fl::check();
}

void Io_Editor::write_file(const std::string &fname)
{
    State_Holder<State> sh(state);
    state = MODIFYING;

    if (buff.appendfile(fname.c_str()))
    {
        messages_window->put_message("running file err: ");
        messages_window->put_message(strerror(1));
        messages_window->put_message("\n");
    }

    insert_position(buff.length());
    io_block = buff.length();

    Fl::check();
}

void Io_Editor::clear()
{
    State_Holder<State> sh(state);
    state = MODIFYING;

    buff.remove(0, std::string(buff.text()).size());
    Fl::check();
}

void Io_Editor::modify_cb(int pos, int nins, int ndel, int nrestyled,
                          const char *c_del, void *io_editor_voided)
{
    Io_Editor *io_editor = reinterpret_cast<Io_Editor *>(io_editor_voided);
    // printf("mod nins = %i ndel = %i nres = %i state = %s\n", nins, ndel,
           // nrestyled,
           // io_editor->state == Io_Editor::MODIFYING ? "mod" : "norm");

    if (io_editor->state == Io_Editor::MODIFYING)
        return;

    State_Holder<State> sh(io_editor->state);
    io_editor->state = MODIFYING;

    /*char msg[256];
      sprintf(msg, "io changed pos=%i nins=%i ndel=%i nres=%i\n", pos, nins,
      ndel, nrestyled); mess(msg);
      */

    /*if (!running) {
      if (nins > 0) {
      from_modyfy_cb = true;
      buff.remove(pos, pos + nins);
      insert_position(pos);
      }

      if (ndel > 0) {
      from_modyfy_cb = true;
      buff.insert(pos, c_del);
      insert_position(pos + 2 * ndel);
      }

      return;
      }*/

    if (nins > 0)
    {
        if (!io_editor->stream)
        {
            // printf("I should have removed that!\n");
            io_editor->buff.remove(pos, pos + nins);
            return;
        }

        if (nins > 2)
        {
            io_editor->messages_window->put_message(
                "Копирование в окно ввода/вывода пока не поддерживается\n");
            io_editor->buff.remove(pos, pos + nins);
            io_editor->insert_position(pos);
            return;
        }

        if ((pos < io_editor->io_block))
        {
            io_editor->buff.remove(pos, pos + nins);
            io_editor->insert_position(pos);
            return;
        }

        char ch = io_editor->buff.char_at(pos);
        if (ch == '\n')
        {
            std::string added =
                io_editor->buff.text_range(io_editor->io_block, pos + 1);
            if (io_editor->stream)
            {
                fprintf(io_editor->stream, "%s\n", added.c_str());
                fflush(io_editor->stream);
            }
            io_editor->io_block = pos + 1;
        }

        return;
    }
    if (nrestyled > 0)
        return;

    // This last part executes only if characters were deleted
    int last_endl_pos = 0;
    if (pos < io_editor->io_block)
    {
        if (pos != io_editor->buff.length())
        {
            io_editor->insert_position(pos + ndel);
            io_editor->buff.insert(pos, c_del);
        }
        else
        {
            io_editor->buff.insert(pos, c_del);
            io_editor->insert_position(io_editor->buff.length());
        }
        return;
    }

    int pos_endl = 0;

    std::string del(c_del);
    pos_endl = del.find_last_of('\n');
    if (pos_endl != -1)
        io_editor->buff.insert(pos, del.substr(0, pos_endl + 1).c_str());
}
