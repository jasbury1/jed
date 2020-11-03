#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <View.h>
#include <Model.h>

class Controller {
public:
    Controller(Model *model, View *view);
    ~Controller();

    void processInput();

private:
    Model *model;
    View *view;
};

#endif /* CONTROLLER_H */