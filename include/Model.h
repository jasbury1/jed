#ifndef MODEL_H
#define MODEL_H

#include <time.h>
#include <termios.h>
#include <string>
#include <vector>

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
        std::string contents;
        std::string render;
        std::vector<unsigned char> highlight;
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
    int rowCxToRx(const Model::erow& row, int cx);
    int rowRxToCx(const Model::erow& row, int rx);
    void insertChar(int c);
    void deleteChar();
    void insertNewline();

    // Getters for various length values
    int numRows() const {return rowList.size();}
    int rowContentLength(int row) const {return rowList[row].contents.size();}
    int rowContentLength() const {return rowList[cy].contents.size();}
    int rowRenderLength(int row) const {return rowList[row].render.size();}
    int rowRenderLength() const {return rowList[cy].render.size();}

    // Getters for the contents and render strings of a given row
    std::string& rowContents(int row){return rowList[row].contents;}
    const std::string& rowContents(int row) const {return rowList[row].contents;}
    std::string& curRowContents(){return rowList[cy].contents;}
    const std::string& curRowContents() const {return rowList[cy].contents;}

    std::string& rowRender(int row){return rowList[row].render;}
    const std::string& rowRender(int row) const {return rowList[row].render;}
    std::string& curRowRender(){return rowList[cy].render;}
    const std::string& curRowRender() const {return rowList[cy].render;}

    std::vector<unsigned char>& rowHighlight(int row){return rowList[row].highlight;}
    const std::vector<unsigned char>& rowHighlight(int row) const {return rowList[row].highlight;}
    std::vector<unsigned char>& curRowHighlight(){return rowList[cy].highlight;}
    const std::vector<unsigned char>& curRowHighlight() const {return rowList[cy].highlight;}

    // Getters for specific rows
    const erow& getRow(int row) const {return rowList[row];}
    const erow& curRow() const {return rowList[cy];}
    erow& getRow(int row) {return rowList[row];}
    erow& curRow() {return rowList[cy];}

private:
    std::vector<erow> rowList;

    void insertRow(int at, const std::string str, int startIndex, std::size_t len);
    void deleteRow(int at);
    void updateRowRender(erow& newRow);
    void rowInsertChar(int c);
    int isSeparator(int c);
    void updateSyntax(erow& curRow);
    int getWindowSize(int *rows, int *cols);
    int getCursorPosition(int *rows, int *cols);

};

#endif //MODEL_H
