#include "face_window.h"

int main(int argc, char *argv[])
{
    Fl::set_font(FL_TIMES_BOLD, "");
    Fl::lock();
    Face_Window win(Fl::w(), Fl::h());
    win.show(argc, argv);

    return Fl::run();
}
