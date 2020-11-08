#ifndef TERMINALVIEW_H
#define TERMINALVIEW_H

#include <termios.h>

#include <View.h>
#include <Model.h>

#define ABUF_INIT {NULL, 0}

class TerminalView : public View {
public:
    typedef struct abuf {
        char *b;
        int len;
    } abuf;

    void draw();
    void drawRows(struct abuf *ab);
    void drawStatusBar(struct abuf *ab);
    void drawMessageBar(struct abuf *ab);

    TerminalView(Model *model);
    ~TerminalView();

    int getScreenrows(){return screenrows;}
    void setScreenrows(int r){screenrows = r;}

    int getScreencols(){return screencols;}
    void setScreencols(int c){screencols = c;}

    void abAppend(abuf *ab, const char *s, int len);
    void abFree(abuf *ab);


private:
    Model *model;
    struct termios origTermios;

    int screenrows;
    int screencols;

    void enableRawMode();
    void disableRawMode();
};


#endif /* TERMINALVIEW_H */