#include <iostream>
#include <string>

#include <TerminalView.h>
#include <Model.h>
#include <Controller.h>


using namespace std;

int main(int argc, char *argv[]) {
    Model *model = new Model();
    View *view = new TerminalView(model);
    Controller *controller = new Controller(model, view);

    if(argc >= 2) {
        /* TODO */
    } else {
        /* TODO Error and exit */
        return -1;
    }

    while(true) {
        view->draw();
        controller->processInput();
    }

}