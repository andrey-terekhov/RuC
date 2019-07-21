#pragma once

#include <FL/Fl_Text_Editor.H>
#include <string>

class Code_Editor : public Fl_Text_Editor
{
public:
    Code_Editor(int x, int y, int w, int h);

    void load_file(const std::string &filename);
    void save_file(const std::string &filename);

    void set_watchpoint();
    bool was_changed();

private:
    static void modify_cb(int         pos,
                          int         nins,
                          int         ndel,
                          int         nrestyled,
                          const char *c_del,
                          void *      userinf);

    bool           changed;
    Fl_Text_Buffer buff;
};
