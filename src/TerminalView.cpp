#include <TerminalView.h>
#include <Model.h>
#include <unistd.h>
#include <string>

TerminalView::TerminalView(std::shared_ptr<const Model> model) : model(model){
    enableRawMode();
}

TerminalView::~TerminalView(){
     disableRawMode();
}

void TerminalView::drawView()
{
    std::string displayStr = "";

    displayStr.append("\x1b[?25l", 6);
    displayStr.append("\x1b[H", 3);

    editorDrawRows(displayStr);
    editorDrawStatusBar(displayStr);
    editorDrawMessageBar(displayStr);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (model->cy - model->rowoff) + 1,
             (model->rx - model->coloff) + 1);
    displayStr.append(buf, strlen(buf));

    displayStr.append("\x1b[?25h", 6);

    write(STDOUT_FILENO, displayStr.c_str(), displayStr.length());
}

void TerminalView::editorDrawRows(std::string& displayStr)
{
    int y;
    for (y = 0; y < model->screenrows; y++)
    {
        int filerow = y + model->rowoff;
        if (filerow >= model->numRows())
        {
            if (model->numRows() == 0 && y == model->screenrows / 3)
            {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                                          "Kilo editor -- version %s", KILO_VERSION);
                if (welcomelen > model->screencols)
                    welcomelen = model->screencols;
                int padding = (model->screencols - welcomelen) / 2;
                if (padding)
                {
                    displayStr.append("~", 1);
                    padding--;
                }
                while (padding--)
                    displayStr.append(" ", 1);
                displayStr.append(welcome, welcomelen);
            }
            else
            {
                displayStr.append("~", 1);
            }
        }
        else
        {
            int len = model->rowList[filerow].rsize - model->coloff;
            if (len < 0)
                len = 0;
            if (len > model->screencols)
                len = model->screencols;
            char *c = &model->rowList[filerow].render[model->coloff];
            unsigned char *hl = &model->rowList[filerow].hl[model->coloff];
            int current_color = -1;
            int j;
            for (j = 0; j < len; j++)
            {
                if (iscntrl(c[j]))
                {
                    char sym = (c[j] <= 26) ? '@' + c[j] : '?';
                    displayStr.append("\x1b[7m", 4);
                    displayStr.append(&sym, 1);
                    displayStr.append("\x1b[m", 3);
                    if (current_color != -1)
                    {
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
                        displayStr.append(buf, clen);
                    }
                }
                else if (hl[j] == Model::HL_NORMAL)
                {
                    if (current_color != -1)
                    {
                        displayStr.append("\x1b[39m", 5);
                        current_color = -1;
                    }
                    displayStr.append(&c[j], 1);
                }
                else
                {
                    int color = editorSyntaxToColor(hl[j]);
                    if (color != current_color)
                    {
                        current_color = color;
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        displayStr.append(buf, clen);
                    }
                    displayStr.append(&c[j], 1);
                }
            }
            displayStr.append("\x1b[39m", 5);
        }

        displayStr.append("\x1b[K", 3);
        displayStr.append("\r\n", 2);
    }
}

void TerminalView::editorDrawStatusBar(std::string& displayStr)
{
    displayStr.append("\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                       model->filename.empty() ? model->filename.c_str() : "[No Name]", model->numRows(),
                       model->dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
                        model->syntax ? model->syntax->filetype : "no ft", model->cy + 1, model->numRows());
    if (len > model->screencols)
        len = model->screencols;
    displayStr.append(status, len);
    while (len < model->screencols)
    {
        if (model->screencols - len == rlen)
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

void TerminalView::editorDrawMessageBar(std::string& displayStr)
{
    displayStr.append("\x1b[K", 3);
    int msglen = strlen(model->statusmsg);
    if (msglen > model->screencols)
        msglen = model->screencols;
    if (msglen && time(NULL) - model->statusmsg_time < 5)
        displayStr.append(model->statusmsg, msglen);
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
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios) == -1){
        // TODO: DIE
    }
}

void TerminalView::enableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &origTermios) == -1){
        // TODO DIE
    }

    struct termios raw = origTermios;
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


