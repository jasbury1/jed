#ifndef TERMINALVIEW_H
#define TERMINALVIEW_H

#include <Model.h>
#include <View.h>
#include <memory>

class TerminalView : public View
{
public:
    TerminalView(std::shared_ptr<const Model> model);
    ~TerminalView();

private:
    // View is given access to the model for displaying its contents but not to modify it
    std::shared_ptr<const Model> model;
    struct termios origTermios;

    void drawView();
    void editorDrawRows(std::string& displayStr);
    void editorDrawStatusBar(std::string& displayStr);
    void editorDrawMessageBar(std::string& displayStr);
    int editorSyntaxToColor(int hl);
    void disableRawMode();
    void enableRawMode();
};

#endif //TERMINALVIEW_H