#include <TerminalView.h>
#include <stdio.h>
#include <string.h>
#include <Model.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>

TerminalView::TerminalView(Model *model) : model(model){
    enableRawMode();
}

TerminalView::~TerminalView(){
     disableRawMode();
}


void TerminalView::editorScroll()
{
    model->rx = 0;
    if (model->cy < model->numrows)
    {
        model->rx = model->editorRowCxToRx(&model->row[model->cy], model->cx);
    }

    if (model->cy < model->rowoff)
    {
        model->rowoff = model->cy;
    }
    if (model->cy >= model->rowoff + model->screenrows)
    {
        model->rowoff = model->cy - model->screenrows + 1;
    }
    if (model->rx < model->coloff)
    {
        model->coloff = model->rx;
    }
    if (model->rx >= model->coloff + model->screencols)
    {
        model->coloff = model->rx - model->screencols + 1;
    }
}

void TerminalView::editorDrawRows(struct abuf *ab)
{
    int y;
    for (y = 0; y < model->screenrows; y++)
    {
        int filerow = y + model->rowoff;
        if (filerow >= model->numrows)
        {
            if (model->numrows == 0 && y == model->screenrows / 3)
            {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                                          "Kilo editor -- version %s", KILO_VERSION);
                if (welcomelen > model->screencols)
                    welcomelen = model->screencols;
                int padding = (model->screencols - welcomelen) / 2;
                if (padding)
                {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while (padding--)
                    abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen);
            }
            else
            {
                abAppend(ab, "~", 1);
            }
        }
        else
        {
            int len = model->row[filerow].rsize - model->coloff;
            if (len < 0)
                len = 0;
            if (len > model->screencols)
                len = model->screencols;
            char *c = &model->row[filerow].render[model->coloff];
            unsigned char *hl = &model->row[filerow].hl[model->coloff];
            int current_color = -1;
            int j;
            for (j = 0; j < len; j++)
            {
                if (iscntrl(c[j]))
                {
                    char sym = (c[j] <= 26) ? '@' + c[j] : '?';
                    abAppend(ab, "\x1b[7m", 4);
                    abAppend(ab, &sym, 1);
                    abAppend(ab, "\x1b[m", 3);
                    if (current_color != -1)
                    {
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
                        abAppend(ab, buf, clen);
                    }
                }
                else if (hl[j] == Model::HL_NORMAL)
                {
                    if (current_color != -1)
                    {
                        abAppend(ab, "\x1b[39m", 5);
                        current_color = -1;
                    }
                    abAppend(ab, &c[j], 1);
                }
                else
                {
                    int color = editorSyntaxToColor(hl[j]);
                    if (color != current_color)
                    {
                        current_color = color;
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        abAppend(ab, buf, clen);
                    }
                    abAppend(ab, &c[j], 1);
                }
            }
            abAppend(ab, "\x1b[39m", 5);
        }

        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }
}

void TerminalView::editorDrawStatusBar(struct abuf *ab)
{
    abAppend(ab, "\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                       model->filename ? model->filename : "[No Name]", model->numrows,
                       model->dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
                        model->syntax ? model->syntax->filetype : "no ft", model->cy + 1, model->numrows);
    if (len > model->screencols)
        len = model->screencols;
    abAppend(ab, status, len);
    while (len < model->screencols)
    {
        if (model->screencols - len == rlen)
        {
            abAppend(ab, rstatus, rlen);
            break;
        }
        else
        {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void TerminalView::editorDrawMessageBar(struct abuf *ab)
{
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(model->statusmsg);
    if (msglen > model->screencols)
        msglen = model->screencols;
    if (msglen && time(NULL) - model->statusmsg_time < 5)
        abAppend(ab, model->statusmsg, msglen);
}

void TerminalView::editorRefreshScreen()
{
    editorScroll();

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (model->cy - model->rowoff) + 1,
             (model->rx - model->coloff) + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

void TerminalView::editorSetStatusMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(model->statusmsg, sizeof(model->statusmsg), fmt, ap);
    va_end(ap);
    model->statusmsg_time = time(NULL);
}


void TerminalView::abAppend(struct abuf *ab, const char *s, int len)
{
    char *new_b = (char *)realloc(ab->b, ab->len + len);

    if (new_b == NULL)
        return;
    memcpy(&new_b[ab->len], s, len);
    ab->b = new_b;
    ab->len += len;
}

void TerminalView::abFree(struct abuf *ab)
{
    free(ab->b);
}

int TerminalView::editorSyntaxToColor(int hl)
{
    switch (hl)
    {
    case Model::HL_COMMENT:
    case Model::HL_MLCOMMENT:
        return 36;
    case Model::HL_KEYWORD1:
        return 33;
    case Model::HL_KEYWORD2:
        return 32;
    case Model::HL_STRING:
        return 35;
    case Model::HL_NUMBER:
        return 31;
    case Model::HL_MATCH:
        return 34;
    default:
        return 37;
    }
}


void TerminalView::disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &model->orig_termios) == -1){
        // TODO: DIE
    }
}

void TerminalView::enableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &model->orig_termios) == -1){
        // TODO DIE
    }

    struct termios raw = model->orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){
        // TODO DIE
    }
}


