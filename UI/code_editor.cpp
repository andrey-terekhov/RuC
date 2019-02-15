#include "code_editor.h"
#include "face_defaults.h"
#include <string>

Code_Editor::Code_Editor(int x, int y, int w, int h)
    : Fl_Text_Editor(x, y, w, h), changed(true)
{
    buffer(buff);
    buff.add_modify_callback(modify_cb, this);
}

void Code_Editor::load_file(const std::string &filename) {
    buff.loadfile(filename.c_str());
    insert_position(buff.length());
}

void Code_Editor::save_file(const std::string &filename) {
    buff.outputfile(filename.c_str(), 0, buff.length());
}

void Code_Editor::set_watchpoint() {
    changed = false;
}

bool Code_Editor::was_changed() {
    return changed;
}

void Code_Editor::modify_cb(int pos, int nins, int ndel, int nrestyled,
                            const char *c_del, void *code_editor_voided)
{
    static bool from_modify = false;
    if (from_modify)
    {
        from_modify = false;
        return;
    }

    Code_Editor *code_editor =
        reinterpret_cast<Code_Editor *>(code_editor_voided);
    if (nrestyled == 0)
        code_editor->changed = true;

    if (nins == 1 && (code_editor->buff.char_at(pos) == '\n'))
    {
        int spaces = 0, tabs = 0;
        
        int cur = code_editor->buff.line_start(pos);
        while (true)
        {
            char cur_char = code_editor->buff.char_at(cur);
            if (cur_char == ' ') 
                ++spaces; 
            else if (cur_char == '\t')
                ++tabs;
            else
                break;
            ++cur;
        }

        std::string t_str(tabs, '\t'), s_str(spaces, ' ');
        code_editor->buff.insert(pos + 1, (t_str + s_str).c_str());
        code_editor->insert_position(pos + spaces + tabs);
    }
}
