#ifndef TERMINALVIEW_H
#define TERMINALVIEW_H

#include <Model.h>
#include <memory>

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

    TerminalView(std::shared_ptr<Model>& model);
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

private:
    std::shared_ptr<Model>& model;
};

#endif //TERMINALVIEW_H