#include <unistd.h>
#include <Controller.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>

Controller::Controller(std::shared_ptr<Model> model, std::shared_ptr<TerminalView> view) : model(model), view(view)
{
}

Controller::~Controller()
{
}

void Controller::processInput()
{
    static int quit_times = KILO_QUIT_TIMES;

    int c = readKey();

    switch (c)
    {
    case '\r':
        model->insertNewline();
        break;

    case CTRL_KEY('q'):
        if (model->dirty && quit_times > 0)
        {
            model->setStatusMessage("WARNING!!! File has unsaved changes. "
                                         "Press Ctrl-Q %d more times to quit.",
                                         quit_times);
            quit_times--;
            return;
        }
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;

    case CTRL_KEY('s'):
        saveFile();
        break;

    case HOME_KEY:
        model->cx = 0;
        break;

    case END_KEY:
        if (model->cy < model->numRows())
            model->cx = model->rowContentLength(model->cy);
        break;

    case CTRL_KEY('f'):
        editorFind();
        break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
        if (c == DEL_KEY)
            moveCursor(ARROW_RIGHT);
        model->deleteChar();
        break;

    case PAGE_UP:
    case PAGE_DOWN:
    {
        if (c == PAGE_UP)
        {
            model->cy = model->rowoff;
        }
        else if (c == PAGE_DOWN)
        {
            model->cy = model->rowoff + model->screenrows - 1;
            if (model->cy > model->numRows())
                model->cy = model->numRows();
        }

        int times = model->screenrows;
        while (times--)
            moveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    }
    break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        moveCursor(c);
        break;

    case CTRL_KEY('l'):
    case '\x1b':
        break;

    default:
        model->insertChar(c);
        break;
    }

    quit_times = KILO_QUIT_TIMES;
    scroll();
}

int Controller::readKey()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (nread == -1 && errno != EAGAIN)
        {
            // TODO
        }
    }

    if (c == '\x1b')
    {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';

        if (seq[0] == '[')
        {
            if (seq[1] >= '0' && seq[1] <= '9')
            {
                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if (seq[2] == '~')
                {
                    switch (seq[1])
                    {
                    case '1':
                        return HOME_KEY;
                    case '3':
                        return DEL_KEY;
                    case '4':
                        return END_KEY;
                    case '5':
                        return PAGE_UP;
                    case '6':
                        return PAGE_DOWN;
                    case '7':
                        return HOME_KEY;
                    case '8':
                        return END_KEY;
                    }
                }
            }
            else
            {
                switch (seq[1])
                {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
                }
            }
        }
        else if (seq[0] == 'O')
        {
            switch (seq[1])
            {
            case 'H':
                return HOME_KEY;
            case 'F':
                return END_KEY;
            }
        }

        return '\x1b';
    }
    else
    {
        return c;
    }
}

void Controller::scroll()
{
    model->rx = 0;
    if (model->cy < model->numRows())
    {
        model->rx = model->rowCxToRx(model->curRow(), model->cx);
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

void Controller::moveCursor(int key)
{
    //TODO: Remove use of erow later
    const Model::erow *row = (model->cy >= model->numRows()) ? NULL : &model->curRow();

    switch (key)
    {
    case ARROW_LEFT:
        if (model->cx != 0)
        {
            model->cx--;
        }
        else if (model->cy > 0)
        {
            model->cy--;
            model->cx = model->rowContentLength();
        }
        break;
    case ARROW_RIGHT:
        if (row && model->cx < model->rowContentLength())
        {
            model->cx++;
        }
        else if (row && model->cx == model->rowContentLength())
        {
            model->cy++;
            model->cx = 0;
        }
        break;
    case ARROW_UP:
        if (model->cy != 0)
        {
            model->cy--;
        }
        break;
    case ARROW_DOWN:
        if (model->cy < model->numRows())
        {
            model->cy++;
        }
        break;
    }

    auto rowlen = 0;
    if(model->cy < model->numRows()){
        rowlen = model->rowContentLength();
    }

    if (model->cx > rowlen)
    {
        model->cx = rowlen;
    }
}

char *Controller::editorPrompt(char *prompt, void (*callback)(char *, int))
{
    size_t bufsize = 128;
    char *buf = (char *)malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

    while (1)
    {
        model->setStatusMessage(prompt, buf);
        view->draw();

        int c = readKey();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE)
        {
            if (buflen != 0)
                buf[--buflen] = '\0';
        }
        else if (c == '\x1b')
        {
            model->setStatusMessage("");
            if (callback)
                callback(buf, c);
            free(buf);
            return NULL;
        }
        else if (c == '\r')
        {
            if (buflen != 0)
            {
                model->setStatusMessage("");
                if (callback)
                    callback(buf, c);
                return buf;
            }
        }
        else if (!iscntrl(c) && c < 128)
        {
            if (buflen == bufsize - 1)
            {
                bufsize *= 2;
                buf = (char *)realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }

        if (callback)
            callback(buf, c);
    }
}

/*
TODO: Eventually we want to do the following:
More advanced editors will write to a new, temporary file, and then rename that file to 
the actual file the user wants to overwrite, and they’ll carefully check for errors through the whole process.
This prevents errors after we erase the file from ruining everything
*/
void Controller::saveFile()
{
    if (model->filename.empty())
    {
        model->filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
        if (model->filename.empty())
        {
            model->setStatusMessage("Save aborted");
            return;
        }
        model->selectSyntaxHighlight();
    }

    int len = 0;

    std::ofstream saveFile(model->filename, std::ios::out | std::ios::trunc);

    if(saveFile.is_open()) {
        for(int i = 0; i < model->numRows(); ++i) {
            saveFile << model->rowContents(i) << "\n";
            len += model->rowContentLength(i) + 1;
        }
        model->dirty = 0;
        model->setStatusMessage("%d bytes written to disk", len);
        saveFile.close();
    }
    else {
        model->setStatusMessage("Can't save! IO error: TODO");
    }
}

void Controller::editorFindCallback(char *query, int key)
{
    /*
    static int last_match = -1;
    static int direction = 1;

    static int saved_hl_line;
    static char *saved_hl = NULL;

    if (saved_hl)
    {
        memcpy(model->rowList[saved_hl_line].hl, saved_hl, model->rowList[saved_hl_line].rsize);
        free(saved_hl);
        saved_hl = NULL;
    }

    if (key == '\r' || key == '\x1b')
    {
        last_match = -1;
        direction = 1;
        return;
    }
    else if (key == ARROW_RIGHT || key == ARROW_DOWN)
    {
        direction = 1;
    }
    else if (key == ARROW_LEFT || key == ARROW_UP)
    {
        direction = -1;
    }
    else
    {
        last_match = -1;
        direction = 1;
    }

    if (last_match == -1)
        direction = 1;
    int current = last_match;
    int i;
    for (i = 0; i < model->numRows(); i++)
    {
        current += direction;
        if (current == -1)
            current = model->numRows() - 1;
        else if (current == model->numRows())
            current = 0;

        Model::erow *row = &model->rowList[current];
        char *match = strstr(row->render, query);
        if (match)
        {
            last_match = current;
            model->cy = current;
            model->cx = model->rowRxToCx(*row, match - row->render);
            model->rowoff = model->numRows();

            saved_hl_line = current;
            saved_hl = (char *)malloc(row->rsize);
            memcpy(saved_hl, row->hl, row->rsize);
            memset(&row->hl[match - row->render], Model::HL_MATCH, strlen(query));
            break;
        }
    }
    */
}

void Controller::editorFind()
{
    int saved_cx = model->cx;
    int saved_cy = model->cy;
    int saved_coloff = model->coloff;
    int saved_rowoff = model->rowoff;
    /*
    char *query = editorPrompt("Search: %s (Use ESC/Arrows/Enter)",
                               editorFindCallback);

    if (query)
    {
        free(query);
    }
    else
    {
        model->cx = saved_cx;
        model->cy = saved_cy;
        model->coloff = saved_coloff;
        model->rowoff = saved_rowoff;
    }
    */
}