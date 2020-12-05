#ifndef MODEL_H
#define MODEL_H

#include <time.h>
#include <termios.h>
#include <string>

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
        std::string contents;
        //char *chars;
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
    // Index into render field. Needed for positioning based on
    int rx;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int numrows;
    erow *row;
    int dirty;
    //char *filename;
    std::string filename;
    char statusmsg[80];
    time_t statusmsg_time;
    struct editorSyntax *syntax;

    Model();
    ~Model();
    void openFile(const std::string& inputFile);
    void setStatusMessage(const char *fmt, ...);
    void selectSyntaxHighlight();
    int rowCxToRx(Model::erow *row, int cx);
    int rowRxToCx(Model::erow *row, int rx);
    void insertChar(int c);
    void deleteChar();
    void insertNewline();

private:
    void insertRow(int at, const std::string &str, int startIndex, std::size_t len);
    void updateRowRender(erow *newRow);
    void rowInsertChar(int c);
    void deleteRow(int at);
    void freeRow(erow *curRow);
    int isSeparator(int c);
    void updateSyntax(erow *curRow);
    int getWindowSize(int *rows, int *cols);
    int getCursorPosition(int *rows, int *cols);

};

#endif //MODEL_H
