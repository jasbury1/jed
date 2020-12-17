#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <functional>

#include "Syntax.h"
#include "Controller.h"

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
            model->setStatusMsg("WARNING!!! File has unsaved changes. "
                                         "Press Ctrl-Q " + std::to_string(quit_times) + "more times to quit.");
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
            model->cy = model->rowoff + view->getScreenRows() - 1;
            if (model->cy > model->numRows())
                model->cy = model->numRows();
        }

        int times = view->getScreenRows();
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
    if (model->cy >= model->rowoff + view->getScreenRows())
    {
        model->rowoff = model->cy - view->getScreenRows() + 1;
    }
    if (model->rx < model->coloff)
    {
        model->coloff = model->rx;
    }
    if (model->rx >= model->coloff + view->getScreenCols())
    {
        model->coloff = model->rx - view->getScreenCols() + 1;
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

std::string Controller::editorPrompt(const std::string& prompt, std::function<void(std::string&, int)> callback)
{
    std::string response = "";

    while (true)
    {
        model->setStatusMsg(prompt + response);
        view->draw();

        int c = readKey();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE)
        {
            if (!response.empty()){
                response.pop_back();
            }
        }
        else if (c == '\x1b')
        {
            model->setStatusMsg("");
            callback(response, c);
            return "";
        }
        else if (c == '\r')
        {
            if (!response.empty())
            {
                model->setStatusMsg("");
                callback(response, c);
                return response;
            }
        }
        else if (!iscntrl(c) && c < 128)
        {
            response += c;
        }

        callback(response, c);
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
    using namespace std::placeholders;
    auto cb = std::bind(&Controller::saveCallback, this, _1, _2);


    if (model->getFilename().empty())
    {
        model->setFilename(editorPrompt("(ESC to cancel) Save as: ", cb));
        if (model->getFilename().empty())
        {
            model->setStatusMsg("Save aborted");
            return;
        }
        model->selectSyntaxHighlight();
    }

    int len = 0;

    std::ofstream saveFile(model->getFilename(), std::ios::out | std::ios::trunc);

    if(saveFile.is_open()) {
        for(int i = 0; i < model->numRows(); ++i) {
            saveFile << model->rowContents(i) << "\n";
            len += model->rowContentLength(i) + 1;
        }
        model->dirty = 0;
        model->setStatusMsg(std::to_string(len) + " bytes written to disk");
        saveFile.close();
    }
    else {
        //TODO IO ERROR HERE
        model->setStatusMsg("Can't save! IO error: TODO");
    }
}

void Controller::editorFind()
{
    int saved_cx = model->cx;
    int saved_cy = model->cy;
    int saved_coloff = model->coloff;
    int saved_rowoff = model->rowoff;

    using namespace std::placeholders;
    auto cb = std::bind(&Controller::editorFindCallback, this, _1, _2);
    
    std::string query = editorPrompt("(Use ESC/Arrows/Enter) Search: ", cb);

    if(query.empty())
    {
        model->cx = saved_cx;
        model->cy = saved_cy;
        model->coloff = saved_coloff;
        model->rowoff = saved_rowoff;
    }
}

void Controller::editorFindCallback(std::string& query, int key)
{
    static int lastMatch = -1;
    static int direction = 1;
    static int restoreRow = -1;

    model->updateRowSyntax(restoreRow);
    
    if (key == '\r' || key == '\x1b')
    {
        lastMatch = -1;
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
        lastMatch = -1;
        direction = 1;
    }

    if (lastMatch == -1)
        direction = 1;
    int current = lastMatch;
    int i;
    for (i = 0; i < model->numRows(); i++)
    {
        current += direction;
        if (current == -1)
            current = model->numRows() - 1;
        else if (current == model->numRows())
            current = 0;

        Model::erow& row = model->getRow(current);        
        auto pos = model->rowRender(current).find(query);
        if (pos != std::string::npos)
        {
            lastMatch = current;
            model->cy = current;
            model->cx = model->rowRxToCx(row, pos);
            model->rowoff = model->numRows();
            scroll();

            restoreRow = current;
            std::fill_n(row.highlight.begin() + pos, query.size(), Syntax::HL_MATCH);

            break;
        }
    }
}

void Controller::saveCallback(std::string& query, int i) {
    // TODO: No functionality yet
    return;
}