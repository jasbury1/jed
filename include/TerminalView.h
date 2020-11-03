#ifndef TERMINALVIEW_H
#define TERMINALVIEW_H

#include <termios.h>

#include <View.h>
#include <Model.h>

class TerminalView : public View {
public:
    void draw();

    int getWindowSize(int *rows, int *cols);

    TerminalView(Model *model);
    ~TerminalView();

private:
    Model *model;
    
    struct termios origTermios;

    void enableRawMode();
    void disableRawMode();
};


#endif /* TERMINALVIEW_H */