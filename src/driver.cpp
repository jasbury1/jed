#include <iostream>
#include <string>

#include <TerminalView.h>
#include <Model.h>
#include <Controller.h>


using namespace std;

int main(int argc, char *argv[]) {
    Model *model = new Model();
    TerminalView *view = new TerminalView(model);
    Controller *controller = new Controller(model, view);

    if(argc >= 2) {
        model->openFile(argv[1]);
    } else {
        /* TODO Error and exit */
        return -1;
    }

    while(true) {
        view->draw();
        controller->processInput();
    }

}