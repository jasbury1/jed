#include "Model.h"
#include "CSyntax.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <ctype.h>
#include <stdarg.h>
#include <fstream>
#include <string>
#include <iostream>


Model::Model() : cx(0), cy(0), rx(0), rowoff(0), coloff(0), dirty(0), statusmsg_time(0), syntax{nullptr}
{
    statusmsg[0] = '\0';

    //TODO: Replace this later!!
    syntax = std::make_unique<CSyntax>();

    if (getWindowSize(&screenrows, &screencols) == -1)
    {
        /* TODO */
    }
    screenrows -= 2;
}

Model::~Model()
{
}

void Model::openFile(const std::string &inputFile)
{
    filename = inputFile;

    selectSyntaxHighlight();

    std::ifstream infile(inputFile);
    if (infile.is_open())
    {
        std::string line;
        while (getline(infile, line))
        {
            auto linelen = line.length();
            while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            {
                linelen--;
            }
            insertRow(numRows(), line, 0, linelen);
        }

        infile.close();
        dirty = 0;
    }
    else
    {
        //TODO: We could not open the file
    }
}

void Model::selectSyntaxHighlight()
{
    if (filename.empty())
        return;
    auto pos = filename.rfind(".");
    if(pos == std::string::npos) {
        return;
    }
    std::string ext = filename.substr(pos);

    // Check to see if the extension has changed
    if(ext != extension) {
        extension = ext;

        // Redo all syntax highlighting
        for(int i = 0; i < rowList.size(); ++i) {
            syntax->updateSyntaxHighlight(rowList, i);
        }
    }
}

int Model::getWindowSize(int *rows, int *cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        return getCursorPosition(rows, cols);
    }
    else
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

int Model::getCursorPosition(int *rows, int *cols)
{
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1;

    while (i < sizeof(buf) - 1)
    {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[')
        return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
        return -1;

    return 0;
}

void Model::setStatusMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(statusmsg, sizeof(statusmsg), fmt, ap);
    va_end(ap);
    statusmsg_time = time(NULL);
}

void Model::updateRowRender(Model::erow& newRow)
{
    newRow.render = "";
    for(auto& c : newRow.contents) {
        // Replace tabs with spaces
        if(c == '\t') {
            for(int j = 0; j < KILO_TAB_STOP; ++j) {
                newRow.render += " ";
            }
        }
        // Copy over all regular characters
        else {
            newRow.render += c;
        }
    }
    syntax->updateSyntaxHighlight(rowList, newRow.idx);
}

void Model::insertRow(int at, const std::string str, int startIndex, std::size_t len)
{
    if (at < 0 || at > numRows())
        return;

    //TODO: Bounds check for len and startIndex

    // Increase the internal index number for each row
    for(int i = at; i < numRows(); ++i) {
        rowList[i].idx++;
    }
    Model::erow newRow;
    auto rowIt = rowList.insert(rowList.begin() + at, newRow);
    rowIt->idx = at;
    rowIt->contents = str.substr(startIndex, len);
    rowIt->commentOpen = false;
    rowIt->render = "";
    rowIt->highlight = {};
    updateRowRender(*rowIt);

    dirty++;
}

void Model::deleteRow(int at) {
    if (at < 0 || at >= numRows())
        return;
    
    auto rowIt = rowList.erase(rowList.begin() + at);
    for(; rowIt < rowList.end(); ++rowIt) {
        rowIt->idx--;
    }
    dirty++; 
}

void Model::insertChar(int c)
{
    if (cy == numRows())
    {
        insertRow(numRows(), "", 0, 0);
    }
    rowInsertChar(c);
    cx++;
}

void Model::rowInsertChar(int c) {
    auto curRow = rowList.begin() + cy;
    int at = cx;

    // TODO: Is this check even necessary??
    if (at < 0 || at > curRow->contents.size())
        at = curRow->contents.size();

    curRow->contents.insert(at, 1, c);

    updateRowRender(*curRow);
    dirty++; 
}

void Model::insertNewline() {
    //TODO: Bounds checking
    if (cx == 0)
    {
        insertRow(cy, "", 0, 0);
    }
    else
    {
        insertRow(cy + 1, this->curRow().contents, cx, rowContentLength() - cx);
        this->curRowContents().resize(cx);
        updateRowRender(this->curRow());
    }
    cy++;
    cx = 0;        
}

void Model::deleteChar()
{
    //TODO: Better bounds checking for rowList index
    if (cy == numRows())
        return;
    if (cx == 0 && cy == 0)
        return;

    // Deleting a character inside the row
    if (cx > 0)
    {
        if(cx > rowContentLength()) {
            return;
        }
        this->curRowContents().erase(cx - 1, 1);
        updateRowRender(rowList[cy]);
        dirty++;
        cx--;
    }
    // Deleting the row's start to shift the row up
    else
    {
        cx = rowList[cy - 1].contents.size();
        rowList[cy - 1].contents += rowList[cy].contents;
        updateRowRender(rowList[cy - 1]);
        dirty++;
        deleteRow(cy);
        cy--;
    } 
}

int Model::rowCxToRx(const Model::erow& row, int cx)
{
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++)
    {
        if (row.contents[j] == '\t')
            rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
        rx++;
    }
    return rx;
}

int Model::rowRxToCx(const Model::erow& row, int rx)
{
    int cur_rx = 0;
    int cx;
    for (cx = 0; cx < row.contents.size(); cx++)
    {
        if (row.contents[cx] == '\t')
            cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
        cur_rx++;

        if (cur_rx > rx)
            return cx;
    }
    return cx;
}