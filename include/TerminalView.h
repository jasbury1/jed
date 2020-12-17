#ifndef TERMINALVIEW_H
#define TERMINALVIEW_H

#include <memory>
#include <termios.h>

#include "View.h"

class Model;

class TerminalView : public View
{
public:
    TerminalView(std::shared_ptr<const Model> model);
    ~TerminalView();

    int getScreenRows() const {return screenrows;}
    int getScreenCols() const {return screencols;}


private:
    int screenrows;
    int screencols;

    // View is given access to the model for displaying its contents but not to modify it
    std::shared_ptr<const Model> model;
    struct termios origTermios;

    void drawView();
    void drawRows(std::string& displayStr);
    void drawStatusBar(std::string& displayStr);
    void drawMessageBar(std::string& displayStr);
    int syntaxToColor(int hl);
    void disableRawMode();
    void enableRawMode();
    int updateWindowSize();
};

int getCursorPosition(int *rows, int *cols);

#endif //TERMINALVIEW_H