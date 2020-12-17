/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <string>

#include "Model.h"
#include "Controller.h"
#include "TerminalView.h"

/*** terminal ***/

/*
void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}
*/


int main(int argc, char *argv[])
{
    std::shared_ptr<Model> model(new Model);
    std::shared_ptr<TerminalView> view(new TerminalView(model));
    std::shared_ptr<Controller> controller(new Controller(model, view));

    if (argc >= 2)
    {
        std::string filename = argv[1];
        model->openFile(filename);
    }

    model->setStatusMsg("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

    while (true)
    {
        view->draw();
        controller->processInput();
    }

    return 0;
}
