#ifndef TERMINALVIEW_H
#define TERMINALVIEW_H

#include <Model.h>
#include <memory>

class TerminalView
{
public:
    TerminalView(std::shared_ptr<const Model> model);
    ~TerminalView();
    void drawScreen();

private:
    std::shared_ptr<const Model> model;
    struct termios origTermios;

    void editorDrawRows(std::string& displayStr);
    void editorDrawStatusBar(std::string& displayStr);
    void editorDrawMessageBar(std::string& displayStr);
    int editorSyntaxToColor(int hl);
    void disableRawMode();
    void enableRawMode();
};

#endif //TERMINALVIEW_H