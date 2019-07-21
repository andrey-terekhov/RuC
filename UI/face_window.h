#pragma once

#include "code_editor.h"
#include "face_defaults.h"
#include "io_editor.h"
#include "messages_window.h"

#include <cstdio>
#include <set>
#include <string>

#include <FL/Fl.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Window.H>

class Face_Window : public Fl_Window
{
public:
    Face_Window(int width, int height);
    ~Face_Window();

    Messages_Window *get_messages_window();

private:
    static void cb_File_Open(Fl_Widget *w, void *face_window_voided);
    static void cb_File_Save_As(Fl_Widget *w, void *face_window_voided);
    static void cb_File_Save(Fl_Widget *w, void *face_window_voided);
    static void cb_Help(Fl_Widget *w, void *face_window_voided);
    static void cb_Quit(Fl_Widget *w, void *face_window_voided);
    static void cb_Compile(Fl_Widget *w, void *face_window_voided);
    static void cb_Run(Fl_Widget *w, void *face_window_voided);
    static void cb_Compile_and_Run(Fl_Widget *w, void *face_window_voided);
    static void cb_Abort(Fl_Widget *w, void *face_window_voided);

    void set_sizes();
    void mess(const char *message);
    void run_compiler();
    void run_vm();

    Fl_Menu_Bar     menu_bar;
    Code_Editor     code_editor;
    Messages_Window messages_window;
    Io_Editor       io_editor;

    const static std::string def_fname;
    std::string              cur_fname = def_fname;
    std::string              cur_directory = "~";

    pid_t compiler;
    pid_t vm;

    enum State
    {
        IDLE,
        COMPILING,
        RUNNING,
        SAVING
    } state;

    bool compiled;
};
