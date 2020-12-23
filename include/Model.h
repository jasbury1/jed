#ifndef MODEL_H
#define MODEL_H

#include <ctime>
#include <termios.h>
#include <string>
#include <vector>

class SyntaxEngine;

constexpr auto editorVersion = "0.0.1";
constexpr auto startupMsg = "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find";
constexpr auto tabStop = 8;
constexpr auto ctrlKey(int k) { return k & 0x1f; }

class Model
{
public:
    typedef struct erow
    {
        std::size_t idx;
        std::string contents;
        std::string render;
        std::vector<unsigned char> highlight;
        bool commentOpen;
    } erow;

    // X and Y values for contents
    int cx, cy;
    // X index coordinate into render field
    int rx;
    int rowoff;
    int coloff;
    int dirty;

    Model();
    ~Model();
    void openFile(const std::string &inputFile);

    void setStatusMsg(const std::string &msg, bool isError = false);
    const std::string &getStatusMsg() const { return statusMsg; }
    void setFilename(const std::string &f) { filename = f; }
    const std::string &getFilename() const { return filename; }

    const std::string &getExtension() const { return extension; }

    void setStatusError(const std::string &errorMsg);

    std::time_t getStatusTime() const { return statusTime; }
    bool isStatusError() const { return statusError; }

    void selectSyntaxHighlight();
    int rowCxToRx(const Model::erow &row, int cx);
    int rowRxToCx(const Model::erow &row, int rx);
    void insertChar(int c);
    void deleteChar();
    void insertNewline();

    // Getters for various length values
    int numRows() const { return rowList.size(); }
    int rowContentLength(int row) const { return rowList[row].contents.size(); }
    int rowContentLength() const { return rowList[cy].contents.size(); }
    int rowRenderLength(int row) const { return rowList[row].render.size(); }
    int rowRenderLength() const { return rowList[cy].render.size(); }

    // Getters for the contents and render strings of a given row
    std::string &rowContents(std::size_t row) { return rowList[row].contents; }
    const std::string &rowContents(std::size_t row) const { return rowList[row].contents; }
    std::string &curRowContents() { return rowList[cy].contents; }
    const std::string &curRowContents() const { return rowList[cy].contents; }

    std::string &rowRender(std::size_t row) { return rowList[row].render; }
    const std::string &rowRender(std::size_t row) const { return rowList[row].render; }
    std::string &curRowRender() { return rowList[cy].render; }
    const std::string &curRowRender() const { return rowList[cy].render; }

    std::vector<unsigned char> &rowHighlight(std::size_t row) { return rowList[row].highlight; }
    const std::vector<unsigned char> &rowHighlight(std::size_t row) const { return rowList[row].highlight; }
    std::vector<unsigned char> &curRowHighlight() { return rowList[cy].highlight; }
    const std::vector<unsigned char> &curRowHighlight() const { return rowList[cy].highlight; }

    // Getters for specific rows
    const erow &getRow(std::size_t row) const { return rowList[row]; }
    const erow &curRow() const { return rowList[cy]; }
    erow &getRow(std::size_t row) { return rowList[row]; }
    erow &curRow() { return rowList[cy]; }

    void updateRowSyntax(std::size_t row);

private:
    std::string filename = "";
    std::string extension = "";
    std::string statusMsg = "";
    std::time_t statusTime;
    bool statusError = false;
    std::unique_ptr<SyntaxEngine> syntax;
    std::vector<erow> rowList;

    void insertRow(int at, const std::string &str, int startIndex, std::size_t len);
    void deleteRow(int at);
    void updateRowRender(erow &newRow);
    void rowInsertChar(int c);
    int getCursorPosition(int *rows, int *cols);
};

#endif //MODEL_H
