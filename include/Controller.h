#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <TerminalView.h>
#include <Model.h>

#define KILO_QUIT_TIMES 3

class Controller
{
public:
    Controller(std::shared_ptr<Model> model, std::shared_ptr<TerminalView> view);
    ~Controller();

    void processKeypress();

private:
    enum editorKey
    {
        BACKSPACE = 127,
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

    std::shared_ptr<Model> model;
    std::shared_ptr<TerminalView> view;

    int readKey();
    void moveCursor(int key);
    void scroll();
    char *editorPrompt(char *prompt, void (*callback)(char *, int));
    void editorSave();
    void editorFind();
    void editorFindCallback(char *query, int key);
    char *editorRowsToString(int *buflen);
};

#endif // CONTROLLER_H