#include <Model.h>
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

char *Model::C_HL_extensions[] = {".c", ".h", ".cpp", NULL};
char *Model::C_HL_keywords[] = {
    "switch", "if", "while", "for", "break", "continue", "return", "else",
    "struct", "union", "typedef", "static", "enum", "class", "case",

    "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
    "void|", NULL};
struct Model::editorSyntax Model::HLDB[] = {
    {"c",
     C_HL_extensions,
     C_HL_keywords,
     "//", "/*", "*/",
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS},
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

Model::Model() : cx(0), cy(0), rx(0), rowoff(0), coloff(0), dirty(0), statusmsg_time(0), syntax(NULL)
{
    statusmsg[0] = '\0';

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
    syntax = NULL;
    if (filename.empty())
        return;
    auto pos = filename.rfind(".");
    if(pos == std::string::npos) {
        return;
    }

    std::string ext = filename.substr(pos);

    for (unsigned int j = 0; j < HLDB_ENTRIES; j++)
    {
        struct Model::editorSyntax *s = &HLDB[j];
        unsigned int i = 0;
        while (s->filematch[i])
        {
            int is_ext = (s->filematch[i][0] == '.');
            if ((is_ext && ext.c_str() && !strcmp(ext.c_str(), s->filematch[i])) ||
                (!is_ext && strstr(filename.c_str(), s->filematch[i])))
            {
                syntax = s;

                int filerow;
                for (filerow = 0; filerow < numRows(); filerow++)
                {
                    updateSyntax(&rowList[filerow]);
                }

                return;
            }
            i++;
        }
    }
}

void Model::updateSyntax(Model::erow *curRow)
{
    curRow->hl = (unsigned char *)realloc(curRow->hl, curRow->rsize);
    memset(curRow->hl, HL_NORMAL, curRow->rsize);

    if (syntax == NULL)
        return;

    char **keywords = syntax->keywords;

    char *scs = syntax->singleline_comment_start;
    char *mcs = syntax->multiline_comment_start;
    char *mce = syntax->multiline_comment_end;

    int scs_len = scs ? strlen(scs) : 0;
    int mcs_len = mcs ? strlen(mcs) : 0;
    int mce_len = mce ? strlen(mce) : 0;

    int prev_sep = 1;
    int in_string = 0;
    int in_comment = (curRow->idx > 0 && rowList[curRow->idx - 1].hl_open_comment);

    int i = 0;
    while (i < curRow->rsize)
    {
        char c = curRow->render[i];
        unsigned char prev_hl = (i > 0) ? curRow->hl[i - 1] : HL_NORMAL;

        if (scs_len && !in_string && !in_comment)
        {
            if (!strncmp(&curRow->render[i], scs, scs_len))
            {
                memset(&curRow->hl[i], HL_COMMENT, curRow->rsize - i);
                break;
            }
        }

        if (mcs_len && mce_len && !in_string)
        {
            if (in_comment)
            {
                curRow->hl[i] = HL_MLCOMMENT;
                if (!strncmp(&curRow->render[i], mce, mce_len))
                {
                    memset(&curRow->hl[i], HL_MLCOMMENT, mce_len);
                    i += mce_len;
                    in_comment = 0;
                    prev_sep = 1;
                    continue;
                }
                else
                {
                    i++;
                    continue;
                }
            }
            else if (!strncmp(&curRow->render[i], mcs, mcs_len))
            {
                memset(&curRow->hl[i], HL_MLCOMMENT, mcs_len);
                i += mcs_len;
                in_comment = 1;
                continue;
            }
        }

        if (syntax->flags & HL_HIGHLIGHT_STRINGS)
        {
            if (in_string)
            {
                curRow->hl[i] = HL_STRING;
                if (c == '\\' && i + 1 < curRow->rsize)
                {
                    curRow->hl[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                if (c == in_string)
                    in_string = 0;
                i++;
                prev_sep = 1;
                continue;
            }
            else
            {
                if (c == '"' || c == '\'')
                {
                    in_string = c;
                    curRow->hl[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        if (syntax->flags & HL_HIGHLIGHT_NUMBERS)
        {
            if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
                (c == '.' && prev_hl == HL_NUMBER))
            {
                curRow->hl[i] = HL_NUMBER;
                i++;
                prev_sep = 0;
                continue;
            }
        }

        if (prev_sep)
        {
            int j;
            for (j = 0; keywords[j]; j++)
            {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen - 1] == '|';
                if (kw2)
                    klen--;

                if (!strncmp(&curRow->render[i], keywords[j], klen) &&
                    isSeparator(curRow->render[i + klen]))
                {
                    memset(&curRow->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
                    i += klen;
                    break;
                }
            }
            if (keywords[j] != NULL)
            {
                prev_sep = 0;
                continue;
            }
        }

        prev_sep = isSeparator(c);
        i++;
    }

    int changed = (curRow->hl_open_comment != in_comment);
    curRow->hl_open_comment = in_comment;
    if (changed && curRow->idx + 1 < numRows())
        updateSyntax(&rowList[curRow->idx + 1]);
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

void Model::updateRowRender(Model::erow *newRow)
{
    int tabs = 0;
    int j;
    for (j = 0; j < newRow->size; j++)
        if (newRow->contents[j] == '\t')
            tabs++;

    free(newRow->render);
    newRow->render = (char *)malloc(newRow->size + tabs * (KILO_TAB_STOP - 1) + 1);

    int idx = 0;
    for (j = 0; j < newRow->size; j++)
    {
        if (newRow->contents[j] == '\t')
        {
            newRow->render[idx++] = ' ';
            while (idx % KILO_TAB_STOP != 0)
                newRow->render[idx++] = ' ';
        }
        else
        {
            newRow->render[idx++] = newRow->contents[j];
        }
    }
    newRow->render[idx] = '\0';
    newRow->rsize = idx;

    updateSyntax(newRow);
}

void Model::insertRow(int at, const std::string str, int startIndex, std::size_t len)
{
    if (at < 0 || at > numRows())
        return;

    //TODO: Bounds check for len and startIndex

    // Increase the internal index number for each row
    for(int i = at + 1; i < numRows(); ++i) {
        rowList[i].idx++;
    }
    Model::erow newRow;
    auto rowIt = rowList.insert(rowList.begin() + at, newRow);
    rowIt->idx = at;
    rowIt->size = len;
    rowIt->contents = str.substr(startIndex, len);
    rowIt->rsize = 0;
    rowIt->render = NULL;
    rowIt->hl = NULL;
    rowIt->hl_open_comment = 0;
    updateRowRender(&*rowIt);

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
    if (at < 0 || at > curRow->size)
        at = curRow->size;

    curRow->contents.insert(at, 1, c);
    curRow->size++;

    updateRowRender(&*curRow);
    dirty++; 
}

void Model::insertNewline() {
    if (cx == 0)
    {
        insertRow(cy, "", 0, 0);
    }
    else
    {
        auto curRow = rowList.begin() + cy;
        auto tempStr = curRow->contents;
        auto tempSize = curRow->size - cx;
        curRow->size = cx;
        curRow->contents.resize(curRow->size);
        updateRowRender(&*curRow);
        insertRow(cy + 1, tempStr, cx, tempSize);
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

    //auto curRow = rowList[cy];

    // Deleting a character inside the row
    if (cx > 0)
    {
        if(cx > rowList[cy].size) {
            return;
        }
        rowList[cy].contents.erase(cx - 1, 1);
        rowList[cy].size--;
        updateRowRender(&rowList[cy]);
        dirty++;
        cx--;
    }
    // Deleting the row's start to shift the row up
    else
    {
        cx = rowList[cy - 1].size;
        rowList[cy - 1].contents += rowList[cy].contents;
        rowList[cy - 1].size += rowList[cy].size;
        updateRowRender(&rowList[cy - 1]);
        dirty++;
        deleteRow(cy);
        cy--;
    } 
}

int Model::isSeparator(int c)
{
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
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
    for (cx = 0; cx < row.size; cx++)
    {
        if (row.contents[cx] == '\t')
            cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
        cur_rx++;

        if (cur_rx > rx)
            return cx;
    }
    return cx;
}