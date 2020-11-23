#include <Model.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <ctype.h>


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

Model::Model() : cx(0), cy(0), rx(0), rowoff(0), coloff(0), numrows(0), row(NULL), dirty(0), filename(NULL), statusmsg_time(0), syntax(NULL)
{
    statusmsg[0] = '\0';

    if (getWindowSize(&screenrows, &screencols) == -1){
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


int Model::editorRowCxToRx(Model::erow *row, int cx)
{
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++)
    {
        if (row->chars[j] == '\t')
            rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
        rx++;
    }
    return rx;
}

int Model::editorRowRxToCx(Model::erow *row, int rx)
{
    int cur_rx = 0;
    int cx;
    for (cx = 0; cx < row->size; cx++)
    {
        if (row->chars[cx] == '\t')
            cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
        cur_rx++;

        if (cur_rx > rx)
            return cx;
    }
    return cx;
}

void Model::editorUpdateRow(Model::erow *newRow)
{
    int tabs = 0;
    int j;
    for (j = 0; j < newRow->size; j++)
        if (newRow->chars[j] == '\t')
            tabs++;

    free(newRow->render);
    newRow->render = (char *)malloc(newRow->size + tabs * (KILO_TAB_STOP - 1) + 1);

    int idx = 0;
    for (j = 0; j < newRow->size; j++)
    {
        if (newRow->chars[j] == '\t')
        {
            newRow->render[idx++] = ' ';
            while (idx % KILO_TAB_STOP != 0)
                newRow->render[idx++] = ' ';
        }
        else
        {
            newRow->render[idx++] = newRow->chars[j];
        }
    }
    newRow->render[idx] = '\0';
    newRow->rsize = idx;

    editorUpdateSyntax(newRow);
}

void Model::editorInsertRow(int at, char *s, size_t len)
{
    if (at < 0 || at > numrows)
        return;

    row = (Model::erow *)realloc(row, sizeof(Model::erow) * (numrows + 1));
    memmove(&row[at + 1], &row[at], sizeof(Model::erow) * (numrows - at));
    for (int j = at + 1; j <= numrows; j++)
        row[j].idx++;

    row[at].idx = at;

    row[at].size = len;
    row[at].chars = (char *)malloc(len + 1);
    memcpy(row[at].chars, s, len);
    row[at].chars[len] = '\0';

    row[at].rsize = 0;
    row[at].render = NULL;
    row[at].hl = NULL;
    row[at].hl_open_comment = 0;
    editorUpdateRow(&row[at]);

    numrows++;
    dirty++;
}

void Model::editorFreeRow(Model::erow *row)
{
    free(row->render);
    free(row->chars);
    free(row->hl);
}

void Model::editorDelRow(int at)
{
    if (at < 0 || at >= numrows)
        return;
    editorFreeRow(&row[at]);
    memmove(&row[at], &row[at + 1], sizeof(Model::erow) * (numrows - at - 1));
    for (int j = at; j < numrows - 1; j++)
        row[j].idx--;
    numrows--;
    dirty++;
}

void Model::editorRowInsertChar(Model::erow *row, int at, int c)
{
    if (at < 0 || at > row->size)
        at = row->size;
    row->chars = (char *)realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    dirty++;
}

void Model::editorRowAppendString(Model::erow *row, char *s, size_t len)
{
    row->chars = (char *)realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    dirty++;
}

void Model::editorRowDelChar(Model::erow *row, int at)
{
    if (at < 0 || at >= row->size)
        return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editorUpdateRow(row);
    dirty++;
}

void Model::editorUpdateSyntax(Model::erow *row)
{
    row->hl = (unsigned char *)realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);

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
    int in_comment = (row->idx > 0 && row[row->idx - 1].hl_open_comment);

    int i = 0;
    while (i < row->rsize)
    {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

        if (scs_len && !in_string && !in_comment)
        {
            if (!strncmp(&row->render[i], scs, scs_len))
            {
                memset(&row->hl[i], HL_COMMENT, row->rsize - i);
                break;
            }
        }

        if (mcs_len && mce_len && !in_string)
        {
            if (in_comment)
            {
                row->hl[i] = HL_MLCOMMENT;
                if (!strncmp(&row->render[i], mce, mce_len))
                {
                    memset(&row->hl[i], HL_MLCOMMENT, mce_len);
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
            else if (!strncmp(&row->render[i], mcs, mcs_len))
            {
                memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
                i += mcs_len;
                in_comment = 1;
                continue;
            }
        }

        if (syntax->flags & HL_HIGHLIGHT_STRINGS)
        {
            if (in_string)
            {
                row->hl[i] = HL_STRING;
                if (c == '\\' && i + 1 < row->rsize)
                {
                    row->hl[i + 1] = HL_STRING;
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
                    row->hl[i] = HL_STRING;
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
                row->hl[i] = HL_NUMBER;
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

                if (!strncmp(&row->render[i], keywords[j], klen) &&
                    is_separator(row->render[i + klen]))
                {
                    memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
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

        prev_sep = is_separator(c);
        i++;
    }

    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if (changed && row->idx + 1 < numrows)
        editorUpdateSyntax(&row[row->idx + 1]);
}

int Model::is_separator(int c)
{
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void Model::editorInsertChar(int c)
{
    if (cy == numrows)
    {
        editorInsertRow(numrows, "", 0);
    }
    editorRowInsertChar(&row[cy], cx, c);
    cx++;
}

void Model::editorInsertNewline()
{
    if (cx == 0)
    {
        editorInsertRow(cy, "", 0);
    }
    else
    {
        Model::erow *curRow = &row[cy];
        editorInsertRow(cy + 1, &curRow->chars[cx], curRow->size - cx);
        curRow = &row[cy];
        curRow->size = cx;
        curRow->chars[curRow->size] = '\0';
        editorUpdateRow(curRow);
    }
    cy++;
    cx = 0;
}

void Model::editorDelChar()
{
    if (cy == numrows)
        return;
    if (cx == 0 && cy == 0)
        return;

    Model::erow *curRow = &row[cy];
    if (cx > 0)
    {
        editorRowDelChar(curRow, cx - 1);
        cx--;
    }
    else
    {
        cx = row[cy - 1].size;
        editorRowAppendString(&row[cy - 1], curRow->chars, curRow->size);
        editorDelRow(cy);
        cy--;
    }
}

void Model::editorOpen(char *open_file)
{
    free(filename);
    filename = strdup(open_file);

    editorSelectSyntaxHighlight();

    FILE *fp = fopen(open_file, "r");
    if (!fp){
        /* TODO DIE */
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1)
    {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            linelen--;
        editorInsertRow(numrows, line, linelen);
    }
    free(line);
    fclose(fp);
    dirty = 0;
}

void Model::editorSelectSyntaxHighlight()
{
    syntax = NULL;
    if (filename == NULL)
        return;

    char *ext = strrchr(filename, '.');

    for (unsigned int j = 0; j < HLDB_ENTRIES; j++)
    {
        struct Model::editorSyntax *s = &HLDB[j];
        unsigned int i = 0;
        while (s->filematch[i])
        {
            int is_ext = (s->filematch[i][0] == '.');
            if ((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
                (!is_ext && strstr(filename, s->filematch[i])))
            {
                syntax = s;

                int filerow;
                for (filerow = 0; filerow < numrows; filerow++)
                {
                    editorUpdateSyntax(&row[filerow]);
                }

                return;
            }
            i++;
        }
    }
}
