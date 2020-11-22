/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <Model.h>
#include <Controller.h>
#include <TerminalView.h>
#include <iostream>

using namespace std;

/*** terminal ***/

void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}


int main(int argc, char *argv[])
{
    std::shared_ptr<Model> model(new Model);
    std::shared_ptr<TerminalView> view(new TerminalView(model));
    std::shared_ptr<Controller> controller(new Controller(model, view));

    if (argc >= 2)
    {
        model->editorOpen(argv[1]);
    }

    view->editorSetStatusMessage(
        "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

    while (1)
    {
        view->editorRefreshScreen();
        controller->editorProcessKeypress();
    }

    return 0;
}
