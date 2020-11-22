#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <Controller.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory.h>

Controller::Controller(std::shared_ptr<Model>& model, std::shared_ptr<TerminalView>& view) : model(model), view(view)
{
}

Controller::~Controller()
{
}

void Controller::editorProcessKeypress()
{
    static int quit_times = KILO_QUIT_TIMES;

    int c = editorReadKey();

    switch (c)
    {
    case '\r':
        model->editorInsertNewline();
        break;

    case CTRL_KEY('q'):
        if (model->dirty && quit_times > 0)
        {
            view->editorSetStatusMessage("WARNING!!! File has unsaved changes. "
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
        editorSave();
        break;

    case HOME_KEY:
        model->cx = 0;
        break;

    case END_KEY:
        if (model->cy < model->numrows)
            model->cx = model->row[model->cy].size;
        break;

    case CTRL_KEY('f'):
        editorFind();
        break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
        if (c == DEL_KEY)
            editorMoveCursor(ARROW_RIGHT);
        model->editorDelChar();
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
            if (model->cy > model->numrows)
                model->cy = model->numrows;
        }

        int times = model->screenrows;
        while (times--)
            editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    }
    break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        editorMoveCursor(c);
        break;

    case CTRL_KEY('l'):
    case '\x1b':
        break;

    default:
        model->editorInsertChar(c);
        break;
    }

    quit_times = KILO_QUIT_TIMES;
}

int Controller::editorReadKey()
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

void Controller::editorMoveCursor(int key)
{
    Model::erow *row = (model->cy >= model->numrows) ? NULL : &model->row[model->cy];

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
            model->cx = model->row[model->cy].size;
        }
        break;
    case ARROW_RIGHT:
        if (row && model->cx < row->size)
        {
            model->cx++;
        }
        else if (row && model->cx == row->size)
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
        if (model->cy < model->numrows)
        {
            model->cy++;
        }
        break;
    }

    row = (model->cy >= model->numrows) ? NULL : &model->row[model->cy];
    int rowlen = row ? row->size : 0;
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
        view->editorSetStatusMessage(prompt, buf);
        view->editorRefreshScreen();

        int c = editorReadKey();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE)
        {
            if (buflen != 0)
                buf[--buflen] = '\0';
        }
        else if (c == '\x1b')
        {
            view->editorSetStatusMessage("");
            if (callback)
                callback(buf, c);
            free(buf);
            return NULL;
        }
        else if (c == '\r')
        {
            if (buflen != 0)
            {
                view->editorSetStatusMessage("");
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

void Controller::editorSave()
{
    if (model->filename == NULL)
    {
        model->filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
        if (model->filename == NULL)
        {
            view->editorSetStatusMessage("Save aborted");
            return;
        }
        model->editorSelectSyntaxHighlight();
    }

    int len;
    char *buf = editorRowsToString(&len);

    int fd = open(model->filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1)
    {
        if (ftruncate(fd, len) != -1)
        {
            if (write(fd, buf, len) == len)
            {
                close(fd);
                free(buf);
                model->dirty = 0;
                view->editorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    view->editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

char *Controller::editorRowsToString(int *buflen)
{
    int totlen = 0;
    int j;
    for (j = 0; j < model->numrows; j++)
        totlen += model->row[j].size + 1;
    *buflen = totlen;

    char *buf = (char *)malloc(totlen);
    char *p = buf;
    for (j = 0; j < model->numrows; j++)
    {
        memcpy(p, model->row[j].chars, model->row[j].size);
        p += model->row[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

void Controller::editorFindCallback(char *query, int key)
{
    static int last_match = -1;
    static int direction = 1;

    static int saved_hl_line;
    static char *saved_hl = NULL;

    if (saved_hl)
    {
        memcpy(model->row[saved_hl_line].hl, saved_hl, model->row[saved_hl_line].rsize);
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
    for (i = 0; i < model->numrows; i++)
    {
        current += direction;
        if (current == -1)
            current = model->numrows - 1;
        else if (current == model->numrows)
            current = 0;

        Model::erow *row = &model->row[current];
        char *match = strstr(row->render, query);
        if (match)
        {
            last_match = current;
            model->cy = current;
            model->cx = model->editorRowRxToCx(row, match - row->render);
            model->rowoff = model->numrows;

            saved_hl_line = current;
            saved_hl = (char *)malloc(row->rsize);
            memcpy(saved_hl, row->hl, row->rsize);
            memset(&row->hl[match - row->render], Model::HL_MATCH, strlen(query));
            break;
        }
    }
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