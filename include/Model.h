#ifndef MODEL_H
#define MODEL_H

#include <time.h>
#include <termios.h>

#define CTRL_KEY(k) ((k)&0x1f)

#define KILO_VERSION "0.0.1"
#define KILO_TAB_STOP 8
#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)


class Model
{
public:
    enum editorHighlight
    {
        HL_NORMAL = 0,
        HL_COMMENT,
        HL_MLCOMMENT,
        HL_KEYWORD1,
        HL_KEYWORD2,
        HL_STRING,
        HL_NUMBER,
        HL_MATCH
    };

    typedef struct erow
    {
        int idx;
        int size;
        int rsize;
        char *chars;
        char *render;
        unsigned char *hl;
        int hl_open_comment;
    } erow;

    struct editorSyntax
    {
        char *filetype;
        char **filematch;
        char **keywords;
        char *singleline_comment_start;
        char *multiline_comment_start;
        char *multiline_comment_end;
        int flags;
    };

    static char *C_HL_extensions[4];
    static char *C_HL_keywords[24];
    static struct editorSyntax HLDB[1];

    int cx, cy;
    int rx;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int numrows;
    erow *row;
    int dirty;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time;
    struct editorSyntax *syntax;

    Model();
    ~Model();

    int getWindowSize(int *rows, int *cols);
    int getCursorPosition(int *rows, int *cols);
    int editorRowCxToRx(erow *row, int cx);
    int editorRowRxToCx(erow *row, int rx);
    void editorSetStatusMessage(const char *fmt, ...);
    void editorUpdateRow(erow *row);
    void editorInsertRow(int at, char *s, size_t len);
    void editorFreeRow(erow *row);
    void editorDelRow(int at);
    void editorRowInsertChar(erow *row, int at, int c);
    void editorRowAppendString(erow *row, char *s, size_t len);
    void editorRowDelChar(erow *row, int at);
    void editorUpdateSyntax(erow *row);
    int is_separator(int c);
    void editorInsertNewline();
    void editorInsertChar(int c);
    void editorDelChar();
    void editorOpen(char *open_file);
    void editorSelectSyntaxHighlight();

private:
};

#endif //MODEL_H
