#include "face_window.h"
#include "face_util.h"
#include "messages_window.h"

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <FL/Fl.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>

const int bottom_height = 200, top_height = 20;

const std::string Face_Window::def_fname = "\\";

Face_Window::Face_Window(int width, int height)
    : Fl_Window(width, height), menu_bar(0, 0, width, menubar_height),
      code_editor(0, menubar_height, width,
                  (height - menubar_height) * (1 - messages_window_rel_height)),
      messages_window(this, 0, menubar_height + code_editor.h(),
                      messages_window_rel_width * w(),
                      messages_window_rel_height * (h() - menubar_height)),
      io_editor(this, messages_window.w(), menubar_height + code_editor.h(),
                w() - messages_window.w(), messages_window.h()),
      compiled(false)
{
    menu_bar.add("Файл/Открыть", "^o", cb_File_Open, this);
    menu_bar.add("Файл/Сохранить", "^s", cb_File_Save, this);
    menu_bar.add("Файл/Сохранить как", "^S", cb_File_Save_As, this);
    menu_bar.add("Собрать", FL_F + 7, cb_Compile, this);
    menu_bar.add("Запустить", FL_F + 5, cb_Run, this);
    menu_bar.add("Собрать и запустить", FL_F + 6, cb_Compile_and_Run,
                 this);
    menu_bar.add("Прервать", FL_CTRL + 'a', cb_Abort, this);

    // menu_bar.add("Помощь", 0, cb_Help, this);
    menu_bar.add("Выход", FL_Escape, cb_Quit, this);

    resizable(this);

    code_editor.linenumber_width(40);

    // transcoding_warning_action  TODO
    // io_editor.deactivate();
    end();
}

Face_Window::~Face_Window()
{
    kill(compiler, SIGKILL);
    kill(vm, SIGKILL);
}

void Face_Window::mess(const char *message)
{
    messages_window.put_message(message);
}

Messages_Window *Face_Window::get_messages_window() { return &messages_window; }

std::string escaped_spaces(const std::string &fname)
{
    return "\"" + fname + "\"";
}

std::string extension(const std::string &fname)
{
    int pos = 0;
    if ((pos = fname.find_last_of('.')) != std::string::npos)
        return fname.substr(pos + 1, (int)fname.size() - pos - 1);
    else
        return "";
}

std::string get_directory(const std::string &fname)
{
    int pos = 0;
    if ((pos = fname.find_last_of("/")) != std::string::npos)
        return fname.substr(0, pos + 1);
    else
        return "";
}

void Face_Window::cb_File_Open(Fl_Widget *w, void *face_window_voided)
{
    Face_Window *face_window =
        reinterpret_cast<Face_Window *>(face_window_voided);

    face_window->compiled = false;

    Fl_Native_File_Chooser *f_chooser =
        new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_FILE);
    f_chooser->title("Выберите файл");
    //    f_chooser->filter("*.c");

    char cur_dir[256] = "~";
    f_chooser->directory(cur_dir);

    int result = f_chooser->show();
    if (result == 0)
        face_window->cur_fname = f_chooser->filename();
    else if (result == -1)
    {
        face_window->mess("File Input error: ");
        face_window->mess(f_chooser->errmsg());
    }

    if (access(face_window->cur_fname.c_str(), 0) == -1)
    {
        face_window->mess("Файл недоступен.");
    }

    face_window->cur_directory = get_directory(face_window->cur_fname);

    face_window->code_editor.load_file(face_window->cur_fname);
}

void Face_Window::cb_File_Save_As(Fl_Widget *w, void *face_window_voided)
{
    Face_Window *face_window =
        reinterpret_cast<Face_Window *>(face_window_voided);

    State_Holder<State> sh(face_window->state);
    face_window->state = SAVING;

    Fl_Native_File_Chooser *f_chooser =
        new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    f_chooser->title("Введите имя файла и выберите место для сохранения");
    if (face_window->cur_fname != def_fname)
        f_chooser->preset_file(face_window->cur_fname.c_str());
    // else
    // {
    // char *c_cur_directory = new char[1024];
    // GetCurrentDirectory(1024, c_cur_directory);
    // mess("curdir\n");
    // mess(c_cur_directory);
    // f_chooser->directory(c_cur_directory);
    // }

    int result = f_chooser->show();
    if (result == 0)
        face_window->cur_fname = f_chooser->filename();
    else if (result == -1)
    {
        face_window->mess("File/Save as error: ");
        face_window->mess(f_chooser->errmsg());
    }

    face_window->code_editor.save_file(face_window->cur_fname);

    /*no_comp_errors = false;
      Fl_Window *f_win = new Fl_Window(300, 90, "ÐÐ²ÐµÐ´Ð¸ÑÐµ
      Ð¸Ð¼Ñ
      ÑÐ°Ð¹Ð»Ð°"); Fl_Button *b_browse = new Fl_Button(10, 50, 100,
      20, "ÐÑÐ±ÑÐ°ÑÑ"); Fl_Return_Button *b_save_as = new
      Fl_Return_Button(120, 50, 100, 20,
      "Ð¡Ð¾ÑÑÐ°Ð½Ð¸ÑÑ");

      Fl_Input *f_inp = new Fl_Input(10, 20, 280, 20);
      f_inp->take_focus();

      if (extension(cur_fname) == "c")
      f_inp->insert(cur_fname.substr(0, cur_fname.size() - 2).c_str());
      else
      f_inp->insert(cur_fname == def_fname ? cur_directory.c_str() :
      cur_fname.c_str());


      b_save_as->callback(cb_b_save_as, (void*)f_inp);
      b_save_as->shortcut(FL_KP_Enter);

      b_browse->callback(cb_b_browse_save, (void*)f_inp);
      b_browse->shortcut(FL_CTRL + 'b');
      f_win->end();
      f_win->show();*/
}

void Face_Window::cb_File_Save(Fl_Widget *w, void *face_window_voided)
{
    Face_Window *face_window =
        reinterpret_cast<Face_Window *>(face_window_voided);

    State_Holder<State> sh(face_window->state);
    face_window->state = SAVING;

    if (face_window->cur_fname == def_fname)
        cb_File_Save_As(w, face_window_voided);
    else
        face_window->code_editor.save_file(face_window->cur_fname);
}

void Face_Window::cb_Help(Fl_Widget *w, void *face_window_voided) {}

void Face_Window::cb_Quit(Fl_Widget *w, void *face_window_voided)
{
    if (fl_choice("Вы действительно хотите выйти?", "Отмена", "Выход", 0) == 1)
        exit(0);
}

struct Info_cb_retranslate
{
    FILE *fd_stream;
    Messages_Window *messages_window;
    bool no_errors;
    bool is_dead;
    bool finished;
};

bool is_error(const std::string &str)
{
    static const std::string error_pref("ошибка: ");
    static const int esize = error_pref.size();
    return str.size() >= esize && str.substr(0, esize) == error_pref;
}

bool is_dead(const std::string &str)
{
    return str.size() >= 4 && str.substr(0, 4) == "dead";
}

void cb_retranslate(FL_SOCKET fd, void *cb_info_voided)
{
    Info_cb_retranslate *info = (Info_cb_retranslate *)cb_info_voided;
    char s[1024];
    if (fgets(s, 1023, info->fd_stream))
    {
        if (is_dead(std::string(s)))
        {
            info->is_dead = true;
            info->finished = true;
            return;
        }
        info->messages_window->put_message(s);
        if (is_error(std::string(s)))
            info->no_errors = false;
    }
    else
        info->finished = true;
}

// void retranslate_rest(FL_SOCKET fd, void *cb_info_voided)
// {
// Info_cb_retranslate *info = (Info_cb_retranslate *)cb_info_voided;
// char s[1024];
// while (fgets(s, 1023, info->retranslator_in) != NULL)
// {
// info->messages_window->put_message(s);
// }
// }

// void retranslator_sigterm(int)
// {
// int c;
// while ((c = getchar()) != EOF)
// {
// Fl::lock();
// mess->put_char_straight(char(c));
// Fl::unlock();
// }
// exit(0);
// }

void Face_Window::run_compiler()
{
    int p[2];
    if (pipe(p) == -1)
    {
        mess(("Could not create the pipe: " + std::string(strerror(errno)))
                 .c_str());
        return;
    }
    const int retranslator_in = p[0], compiler_out = p[1];
    int compiler_pid = fork();
    if (compiler_pid == -1)
    {
        mess(("Compiler process could not be created. fork(): " +
              std::string(strerror(errno)))
                 .c_str());
        return;
    }
    else if (compiler_pid == 0)
    { // compiler process
        close(retranslator_in);
        dup2(compiler_out, STDOUT_FILENO);
        close(compiler_out);
        const char *compiler_names[] = {"./ruc", "./bin/ruc"};
        for (const char *name : compiler_names)
        {
            const char *argv[] = {name, cur_fname.c_str(), NULL};
            execv(name, (char **)(argv));
        }
        printf("dead\n");
        fflush(stdout);
        exit(0);
    }
    close(compiler_out);

    compiler = compiler_pid;

    Info_cb_retranslate info;
    info.messages_window = &messages_window;
    info.fd_stream = fdopen(retranslator_in, "r");
    info.finished = false;
    info.no_errors = true;
    info.is_dead = false;
    Fl::add_fd(retranslator_in, cb_retranslate, (void *)&info);

    while (!info.finished)
        Fl::wait(0.05);

    int wstatus, ret;
    ret = waitpid(compiler_pid, &wstatus, WNOHANG);
    if (ret == -1)
    {
        messages_window.put_message("waitpid for compiler failed: " +
                                    std::string(strerror(errno)));
        return;
    }
    if (info.is_dead)
        mess("Не найден компилятор РуСи. Пожалуйста, удостоверьтесь, что\n"
             "собранный компилятор находится в одной папке с запускаемым\n"
             "интерфейсом.");
    compiled = info.no_errors && !info.is_dead;
    Fl::remove_fd(retranslator_in);
    close(retranslator_in);

    if (!compiled)
        printf("erros found\n");
    // int retranslator_pid = fork();
    // if (retranslator_pid == -1)
    // {
    // mess("Retranslator process could not be created");
    // return;
    // }
    // else if (retranslator_pid == 0)
    // { // retranslator process
    // signal(SIGTERM, retranslator_sigterm);
    // dup2(retranslator_in, STDIN_FILENO);
    // close(retranslator_in);
    // int c;
    // while ((c = getchar()) != EOF)
    // {
    // Fl::lock();
    // ::mess->put_char_straight(char(c));
    // Fl::unlock();
    // }
    // }

    // kill(retranslator_pid, SIGTERM);

    // retranslate_rest(retranslator_in, (void*)&info);
}

void Face_Window::cb_Compile(Fl_Widget *w, void *face_window_voided)
{
    Face_Window *face_window =
        reinterpret_cast<Face_Window *>(face_window_voided);

    switch (face_window->state)
    {
    case COMPILING:
        face_window->mess("Код уже компилируется.\n");
        Fl::redraw();
        return;

    case RUNNING:
        face_window->mess("Прежде, чем компилировать программу, дождитесь "
                          "завершения исполнения предыдущей. Прервать "
                          "исполнение можно с помощью кнопки РуСи/Прервать.\n");
        return;

    case SAVING:
        face_window->mess("Дождитесь завершения соххранения файла.\n");
        break;
    default:
        break;
    }

    cb_File_Save(w, face_window_voided);
    State_Holder<State> sh(face_window->state);
    face_window->state = COMPILING;

    face_window->messages_window.clear();
    face_window->io_editor.activate();
    face_window->io_editor.clear();

    face_window->mess((face_window->cur_fname + "\n").c_str());

    face_window->run_compiler();

    face_window->code_editor.set_watchpoint();

    // ::remove("export.txt");
    // ::remove("tree.txt");
}

/*void cb_Compile(Fl_Widget* w, void* it) {
  _cb_Compile(w, it);
  _cb_Compile(w, it);
  }*/

const std::string no_vm_message = "novm!11`'`/";

bool is_vm_not_found(const std::string &s)
{
    return s.size() <= no_vm_message.size() &&
           s.substr(0, no_vm_message.size()) == no_vm_message;
}

struct Info_VM_out
{
    FILE *f;
    Io_Editor *io_editor;
    bool vm_not_found;
    bool vm_finished;
};

void cb_new_vm_output(FL_SOCKET fd, void *info_voided)
{
    Info_VM_out *info = (Info_VM_out *)info_voided;
    char s[4096];
    int got = read(fd, s, 4095);
    if (got != 0)
    {
        s[got] = '\0';
        if (is_vm_not_found(s))
        {
            info->vm_finished = true;
            info->vm_not_found = true;
            return;
        }
        info->io_editor->write(std::string(s));
    }
    else
        info->vm_finished = true;
}

void Face_Window::run_vm()
{
    int p1[2], p2[2];
    if (pipe(p1) == -1)
    {
        mess(("Could not create the first the pipe: " +
              std::string(strerror(errno)))
                 .c_str());
        return;
    }
    if (pipe(p2) == -1)
    {
        mess(("Could not create the second pipe: " +
              std::string(strerror(errno)))
                 .c_str());
        close(p1[0]);
        close(p1[1]);
        return;
    }

    const int vm_in = p1[0], user_in = p1[1];
    const int vm_out = p2[1], user_out = p2[0];
    // user_in stands for input from user, user_out -- output for user
    int vm_pid = fork();
    if (vm_pid == -1)
    {
        mess(("VM process could not be created. fork(): " +
              std::string(strerror(errno)))
                 .c_str());
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        return;
    }
    else if (vm_pid == 0)
    { // VM process
        close(user_in);
        close(user_out);
        dup2(vm_in, STDIN_FILENO);
        close(vm_in);
        dup2(vm_out, STDOUT_FILENO);
        close(vm_out);

        const char *vm_names[] = {"./rucvm", "./bin/rucvm"};
        for (const char *name : vm_names)
        {
            const char *argv[] = {name, NULL};
            execv(name, (char **)(argv));
        }
        printf("%s\n", no_vm_message.c_str());
        fflush(stdout);
        exit(0);
    }

    close(vm_in);
    close(vm_out);

    vm = vm_pid;

    FILE *stream_user_in = fdopen(user_in, "w");
    io_editor.bind_stream(stream_user_in);

    Info_VM_out info;
    info.f = fdopen(user_out, "r");
    info.io_editor = &io_editor;
    info.vm_not_found = false;
    info.vm_finished = false;
    Fl::add_fd(user_out, cb_new_vm_output, (void *)&info);

    while (!info.vm_finished)
        Fl::wait();

    int wstatus, ret;
    do
    {
        ret = waitpid(vm_pid, &wstatus, WNOHANG);
        Fl::wait(0.02);
        if (ret == -1)
        {
            messages_window.put_message("waitpid for vm failed: " +
                                        std::string(strerror(errno)));
            io_editor.unbind_stream();
            close(user_in);
            close(user_out);
            Fl::remove_fd(user_out);
            return;
        }
    } while (ret == 0);

    if (info.vm_not_found)
        mess("Не найдена виртуальная машина РуСи. Пожалуйста, "
             "удостоверьтесь, "
             "что\n"
             "собранная виртуальная машина находится в одной папке с "
             "запускаемым\n"
             "интерфейсом.");
    if (WIFEXITED(wstatus))
        io_editor.write("\nПрограмма завершилась\n");
    else if (WIFSIGNALED(wstatus))
        io_editor.write("\nПрограмма была прервана\n");

    io_editor.unbind_stream();
    close(user_in);
    close(user_out);
    Fl::remove_fd(user_out);
}

void Face_Window::cb_Run(Fl_Widget *w, void *face_window_voided)
{
    Face_Window *face_window =
        reinterpret_cast<Face_Window *>(face_window_voided);

    switch (face_window->state)
    {
    case RUNNING:
    {
        face_window->mess("Программа уже запущена. Для нового запуска "
                          "дождитесь ее завершения или прервите ее.\n");
        return;
    }

    case COMPILING:
    {
        face_window->mess("Для запуска дождитесь завершения компиляции.\n");
        return;
    }

    case SAVING:
    {
        face_window->mess("Для запуска дождитесь завершения сохранения файла.");
        return;
    }

    default:
        break;
    }

    if (!face_window->compiled)
    {
        face_window->mess("Для того, чтобы программа была запущена, она должна "
                          "быть успешно скомпилирована.\n");
        return;
    }

    if (face_window->code_editor.was_changed())
        if (fl_choice("Программа была изменена с момента последней компиляции. "
                      "Скомпилировать заново?",
                      "Запустить старую", "Скомпилировать", 0) == 1)
            cb_Compile(w, face_window_voided);

    State_Holder<State> sh(face_window->state);
    face_window->state = RUNNING;

    face_window->io_editor.clear();
    face_window->run_vm();

    // remove("out.txt");
}

void Face_Window::cb_Compile_and_Run(Fl_Widget *w, void *face_window_voided)
{
    cb_Compile(w, face_window_voided);
    cb_Run(w, face_window_voided);
}

void Face_Window::cb_Abort(Fl_Widget *w, void *face_window_voided)
{
    Face_Window *face_window = (Face_Window *)face_window_voided;
    kill(face_window->vm, SIGTERM);
}
