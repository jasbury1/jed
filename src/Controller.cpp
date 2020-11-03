#include <Controller.h>
#include <View.h>
#include <Model.h>

Controller::Controller(Model *model, View *view) : model(model), view(view) {

}

Controller::~Controller() {

}