#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <TerminalView.h>
#include <Model.h>

#define CTRL_KEY(k) ((k) & 0x1f)

class Controller {
public:
    Controller(Model *model, TerminalView *view);
    ~Controller();

    enum editorKey {
        ARROW_LEFT = 1000,
        ARROW_RIGHT,
        ARROW_UP,
        ARROW_DOWN,
        DEL_KEY,
        HOME_KEY,
        END_KEY,
        PAGE_UP,
        PAGE_DOWN
    };

    void processInput();
    int readKey();
    void setCursorPosition(int key);
    int getCursorPosition();
    int updateWindowSize();
    void scroll();
    int rowCxToRx(Model::erow *row, int cx);

private:
    Model *model;
    TerminalView *view;
};

#endif /* CONTROLLER_H */