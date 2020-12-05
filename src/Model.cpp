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

Model::Model() : cx(0), cy(0), rx(0), rowoff(0), coloff(0), numrows(0), row(NULL), dirty(0), statusmsg_time(0), syntax(NULL)
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

int Model::rowCxToRx(Model::erow *row, int cx)
{
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++)
    {
        if (row->contents[j] == '\t')
            rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
        rx++;
    }
    return rx;
}

int Model::rowRxToCx(Model::erow *row, int rx)
{
    int cur_rx = 0;
    int cx;
    for (cx = 0; cx < row->size; cx++)
    {
        if (row->contents[cx] == '\t')
            cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
        cur_rx++;

        if (cur_rx > rx)
            return cx;
    }
    return cx;
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

//TODO: Add default parameters here for start and len?
void Model::insertRow(int at, const std::string &str, int startIndex, std::size_t len)
{
    if (at < 0 || at > numrows)
        return;

    //TODO: Bounds check for len and startIndex

    row = (Model::erow *)realloc(row, sizeof(Model::erow) * (numrows + 1));
    // Shift all rows forward
    memmove(&row[at + 1], &row[at], sizeof(Model::erow) * (numrows - at));
    // Increase the internal index number for each row
    for (int j = at + 1; j <= numrows; j++)
        row[j].idx++;

    row[at].idx = at;

    row[at].size = len;
    row[at].contents = str.substr(startIndex, len);

    row[at].rsize = 0;
    row[at].render = NULL;
    row[at].hl = NULL;
    row[at].hl_open_comment = 0;
    updateRowRender(&row[at]);

    numrows++;
    dirty++;
}

//TODO: This needs to be removed entirely. Make it a destructor
void Model::freeRow(Model::erow *curRow)
{
    free(curRow->render);
    free(curRow->hl);
}

void Model::deleteRow(int at) {
    if (at < 0 || at >= numrows)
        return;
    freeRow(&row[at]);
    memmove(&row[at], &row[at + 1], sizeof(Model::erow) * (numrows - at - 1));
    for (int j = at; j < numrows - 1; j++)
        row[j].idx--;
    numrows--;
    dirty++; 
}

void Model::insertChar(int c)
{
    if (cy == numrows)
    {
        insertRow(numrows, "", 0, 0);
    }
    rowInsertChar(c);
    cx++;
}

void Model::rowInsertChar(int c) {
    Model::erow *curRow = &row[cy];
    int at = cx;

    // TODO: Is this check even necessary??
    if (at < 0 || at > curRow->size)
        at = curRow->size;

    curRow->contents.insert(at, 1, c);
    curRow->size++;

    updateRowRender(curRow);
    dirty++; 
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
    int in_comment = (curRow->idx > 0 && row[curRow->idx - 1].hl_open_comment);

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
    if (changed && curRow->idx + 1 < numrows)
        updateSyntax(&row[curRow->idx + 1]);
}



int Model::isSeparator(int c)
{
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void Model::insertNewline() {
    if (cx == 0)
    {
        insertRow(cy, "", 0, 0);
    }
    else
    {
        Model::erow *curRow = &row[cy];
        insertRow(cy + 1, curRow->contents, cx, curRow->size - cx);
        curRow = &row[cy];
        curRow->size = cx;
        curRow->contents.resize(curRow->size);
        updateRowRender(curRow);
    }
    cy++;
    cx = 0;        
}

void Model::deleteChar()
{
    if (cy == numrows)
        return;
    if (cx == 0 && cy == 0)
        return;

    Model::erow *curRow = &row[cy];
    if (cx > 0)
    {
        if(cx > curRow->size) {
            return;
        }
        curRow->contents.erase(cx - 1, 1);
        curRow->size--;
        updateRowRender(curRow);
        dirty++;
        cx--;
    }
    else
    {
        cx = row[cy - 1].size;
        (&row[cy - 1])->contents += curRow->contents;
        (&row[cy - 1])->size += curRow->size;
        updateRowRender(&row[cy - 1]);
        dirty++;
        deleteRow(cy);
        cy--;
    } 
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
            insertRow(numrows, line, 0, linelen);
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
                for (filerow = 0; filerow < numrows; filerow++)
                {
                    updateSyntax(&row[filerow]);
                }

                return;
            }
            i++;
        }
    }
}


