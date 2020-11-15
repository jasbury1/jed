#ifndef TERMINALVIEW_H
#define TERMINALVIEW_H

#include <Model.h>

#define ABUF_INIT \
    {             \
        NULL, 0   \
    }

class TerminalView
{

public:
    struct abuf
    {
        char *b;
        int len;
    };

    Model *model;

    TerminalView(Model *model);
    ~TerminalView();
    void editorScroll();
    void editorDrawRows(struct abuf *ab);
    void editorDrawStatusBar(struct abuf *ab);
    void editorDrawMessageBar(struct abuf *ab);
    void editorRefreshScreen();
    void editorSetStatusMessage(const char *fmt, ...);
    int editorSyntaxToColor(int hl);
    void disableRawMode();
    void enableRawMode();

    void abAppend(struct abuf *ab, const char *s, int len);
    void abFree(struct abuf *ab);
};

#endif //TERMINALVIEW_H