#include <unistd.h>
#include <sys/ioctl.h>
#include <string>
#include <iostream>

#include "TerminalView.h"
#include "Model.h"
#include "Syntax.h"

TerminalView::TerminalView(std::shared_ptr<const Model> model) : model(model)
{
    enableRawMode();
}

TerminalView::~TerminalView()
{
    disableRawMode();
}

void TerminalView::drawView()
{
    if (updateWindowSize() == -1)
    {
        /* TODO */
    }
    screenrows -= 2;
    std::string displayStr = "";

    displayStr.append("\x1b[?25l", 6);
    displayStr.append("\x1b[H", 3);

    drawRows(displayStr);
    drawStatusBar(displayStr);
    drawMessageBar(displayStr);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (model->cy - model->rowoff) + 1,
             (model->rx - model->coloff) + 1);
    displayStr.append(buf, strlen(buf));

    displayStr.append("\x1b[?25h", 6);

    write(STDOUT_FILENO, displayStr.c_str(), displayStr.length());
}

void TerminalView::drawRows(std::string &displayStr)
{
    for (int r = 0; r < screenrows; r++)
    {
        auto rowNum = r + model->rowoff;
        if (rowNum >= model->numRows())
        {
            // File is empty. Draw a temporary welcome message
            if (model->numRows() == 0 && r == screenrows / 3)
            {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                                          "Kilo editor -- version %s", editorVersion);
                if (welcomelen > screencols)
                    welcomelen = screencols;
                int padding = (screencols - welcomelen) / 2;
                if (padding)
                {
                    displayStr.append("~", 1);
                    padding--;
                }
                while (padding--)
                    displayStr.append(" ", 1);
                displayStr.append(welcome, welcomelen);
            }
            // Rows that don't yet exist are represented by ~
            else
            {
                displayStr.append("~", 1);
            }
        }
        // Draw rows from the file
        else
        {
            auto len = model->rowRenderLength(rowNum) - model->coloff;
            if (len < 0)
                len = 0;
            if (len > screencols)
                len = screencols;
            int current_color = -1;
            for (int j = 0; j < len; j++)
            {
                if (iscntrl(model->rowRender(rowNum)[model->coloff + j]))
                {
                    displayStr.append("\x1b[7m", 4);
                    if (model->rowRender(rowNum)[model->coloff + j] <= 26)
                    {
                        displayStr += '@' + model->rowRender(rowNum)[model->coloff + j];
                    }
                    else
                    {
                        displayStr += '?';
                    }
                    displayStr.append("\x1b[m", 3);
                    if (current_color != -1)
                    {
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
                        displayStr.append(buf, clen);
                    }
                }
                else if (model->rowHighlight(rowNum)[j + model->coloff] == Syntax::HL_NORMAL)
                {
                    if (current_color != -1)
                    {
                        displayStr.append("\x1b[39m", 5);
                        current_color = -1;
                    }
                    displayStr += model->rowRender(rowNum)[model->coloff + j];
                }
                else
                {
                    int color = syntaxToColor(model->rowHighlight(rowNum)[j + model->coloff]);
                    if (color != current_color)
                    {
                        current_color = color;
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        displayStr.append(buf, clen);
                    }
                    displayStr += model->rowRender(rowNum)[model->coloff + j];
                }
            }
            displayStr.append("\x1b[39m", 5);
        }

        displayStr.append("\x1b[K", 3);
        displayStr.append("\r\n", 2);
    }
}

void TerminalView::drawStatusBar(std::string &displayStr)
{
    displayStr.append("\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                       !model->getFilename().empty() ? model->getFilename().c_str() : "[No Name]", model->numRows(),
                       model->dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
                        model->getSyntax() != nullptr ? model->getSyntax()->fileType().c_str() : "plaintext", model->cy + 1, model->numRows());
    if (len > screencols)
        len = screencols;
    displayStr.append(status, len);
    while (len < screencols)
    {
        if (screencols - len == rlen)
        {
            displayStr.append(rstatus, rlen);
            break;
        }
        else
        {
            displayStr.append(" ", 1);
            len++;
        }
    }
    displayStr.append("\x1b[m", 3);
    displayStr.append("\r\n", 2);
}

void TerminalView::drawMessageBar(std::string &displayStr)
{
    displayStr.append("\x1b[K", 3);
    int msglen = model->getStatusMsg().size();
    if (msglen > screencols)
        msglen = screencols;
    std::string status = model->getStatusMsg();
    status.resize(msglen);
    displayStr += status;
}

int TerminalView::syntaxToColor(int hl)
{
    switch (hl)
    {
    case Syntax::HL_COMMENT:
    case Syntax::HL_MLCOMMENT:
        return 36;
    case Syntax::HL_KEYWORD1:
        return 33;
    case Syntax::HL_KEYWORD2:
        return 32;
    case Syntax::HL_STRING:
        return 35;
    case Syntax::HL_NUMBER:
        return 31;
    case Syntax::HL_MATCH:
        return 34;
    default:
        return 37;
    }
}

int TerminalView::updateWindowSize()
{
    struct winsize ws;

    /* If we couldn't get the window size, move cursor to bottom right and read position */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        return getCursorPosition(&screenrows, &screencols);
    }
    /* We were able to read the window size from ioctl */
    else
    {
        screencols = ws.ws_col;
        screenrows = ws.ws_row;
        return 0;
    }
}

int getCursorPosition(int *rows, int *cols)
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

void TerminalView::disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios) == -1)
    {
        // TODO: DIE
    }
}

void TerminalView::enableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &origTermios) == -1)
    {
        // TODO DIE
    }

    struct termios raw = origTermios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    {
        // TODO DIE
    }
}
