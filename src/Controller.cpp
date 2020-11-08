#include <Controller.h>
#include <TerminalView.h>
#include <Model.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

Controller::Controller(Model *model, TerminalView *view) : model(model), view(view) {
    if(updateWindowSize() == -1) {
        /* TODO */
    }
    view->setScreenrows(view->getScreenrows() - 1);
}

Controller::~Controller() {

}

int Controller::processInput() {
    scroll();

    int c = readKey();

    switch (c) {
        case CTRL_KEY('q'):
            /* Erase display escape sequence */
            write(STDOUT_FILENO, "\x1b[2J", 4);
            /* Cursor position escape sequence */
            write(STDOUT_FILENO, "\x1b[H", 3);
            return -1;
            break;

        case HOME_KEY:
            model->setCx(0);
            break;

        case END_KEY:
            if (model->getCy() < model->getNumrows()) {
                model->setCx(model->getRow()[model->getCy()].size);
            }
            break;

        case PAGE_UP:
            {
                model->setCy(model->getRowoff());
                int times = view->getScreenrows();
                while (times--) {
                    setCursorPosition(ARROW_UP);
                }
            }
            break;

        case PAGE_DOWN:
            {
                model->setCy(model->getRowoff() + view->getScreenrows() - 1);
                if(model->getCy() > model->getNumrows()) {
                    model->setCy(model->getNumrows());
                }
                int times = view->getScreenrows();
                while (times--) {
                    setCursorPosition(ARROW_DOWN);
                }
            }
            break;

        case ARROW_UP:
            setCursorPosition(c);
            break;

        case ARROW_DOWN:
            setCursorPosition(c);
            break;

        case ARROW_LEFT:
            setCursorPosition(c);
            break;
            
        case ARROW_RIGHT:
            setCursorPosition(c);
            break;
    }
    return 0;
}

int Controller::readKey() {
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if(nread == -1 && errno != EAGAIN) {
            /* TODO */
        }
    }

    if (c == '\x1b') {
        char seq[3];

        if(read(STDIN_FILENO, &seq[0], 1) != 1) {
            return '\x1b';
        }
        if(read(STDIN_FILENO, &seq[1], 1) != 1) {
            return '\x1b';
        }

        /* Alias arrow keys to hjkl movement */
        if (seq[0] == '[') {
            if(seq[1] >= '0' && seq[1] <= '9') {
                if(read(STDIN_FILENO, &seq[2], 1) != 1) {
                    return '\x1b';
                }
                if(seq[2] == '~') {
                    switch (seq[1]) {
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
            else {
                switch (seq[1]) {
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
        /* Default to assuming <esc> was pressed */
        return '\x1b';
    } else {
        return c;
    }
}

void Controller::setCursorPosition(int key) {
    Model::erow *row;

    if(model->getCy() >= model->getNumrows()){
        row = NULL;
    } else {
        row = &model->getRow()[model->getCy()];
    }

    switch (key) {
        case ARROW_LEFT:
            if(model->getCx() != 0) {
                model->setCx(model->getCx() - 1);
            } else if (model->getCy() > 0) {
                model->setCy(model->getCy() - 1);
                model->setCx(model->getRow()[model->getCy()].size);
            }
            break;
        case ARROW_RIGHT:
            if(row && model->getCx() < row->size) {
                model->setCx(model->getCx() + 1);
            } else if (row && model->getCx() == row->size) {
                model->setCy(model->getCy() + 1);
                model->setCx(0);
            }
            break;
        case ARROW_UP:
            if(model->getCy() != 0) {
                model->setCy(model->getCy() - 1);
            }
            break;
        case ARROW_DOWN:
            /* move past bottom of screen but not bottom of file */
            if(model->getCy() < model->getNumrows()){
                model->setCy(model->getCy() + 1);
            }
            break;
    }

    /* Correct E.cx if it ends up past the end of a line */
    row = (model->getCy() >= model->getNumrows()) ? NULL : &model->getRow()[model->getCy()];
    int rowlen = row ? row->size : 0;
    if(model->getCx() > rowlen) {
        model->setCx(rowlen);
    }
}

int Controller::getCursorPosition(){
   char buf[32];
   unsigned int i = 0;
   int rows = view->getScreenrows();
   int cols = view->getScreencols();

   /* Device status report (n) with arg 6 for cursor position */
   if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
      return -1;
   }

   /* Read characters until we get to an 'R' character marking the end
    * of the cursor position report */
   while(i < sizeof(buf) - 1) {
      if (read(STDIN_FILENO, &buf[i], 1) != 1) {
         break;
      }
      if(buf[i] == 'R') {
         break;
      }
      i++;
   }
   buf[i] = '\0';

   /* Check the response formatting */
   if (buf[0] != '\x1b' || buf[1] != '[') {
      return -1;
   }

   /* Attempt to skip over \x1b and [ sequence to get row and col values */
   if (sscanf(&buf[2], "%d;%d", &rows, &cols) != 2) {
      return -1;
   }

   return 0;
}

int Controller::updateWindowSize(){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* Move cursor to bottom right if window size failed */
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }
        /* Get window size from cursor at bottom right */
        return getCursorPosition();
    }
    else {
        /* Gather window size info if window size succeeded */
        view->setScreencols(ws.ws_col);
        view->setScreenrows(ws.ws_row);
        return 0;
    }
}

void Controller::scroll() {
   model->setRx(0);
   if(model->getCy() < model->getNumrows()) {
      model->setRx(rowCxToRx(&model->getRow()[model->getCy()], model->getCx()));
   }

   /* Checks if the cursor is above the visible window */
   if(model->getCy() < model->getRowoff()) {
       /* Scroll up to where the cursor is */
       model->setRowoff(model->getCy());
   }
   /* See if the cursor is past the bottom */
   if(model->getCy() >= model->getRowoff() + view->getScreenrows()) {
       model->setRowoff(model->getCy() - view->getScreenrows() + 1);
   }
   if(model->getRx() < model->getColoff()) {
       model->setColoff(model->getRx());
   }
   if(model->getRx() >= model->getColoff() + view->getScreencols()) {
       model->setColoff(model->getRx() - view->getScreencols() + 1);
   }
}

int Controller::rowCxToRx(Model::erow *row, int cx) {
   int rx = 0;
   int j;
   for(j = 0; j < cx; j++) {
      if(row->chars[j] == '\t') {
         rx += (JED_TAB_STOP - 1) - (rx % JED_TAB_STOP);
      }
      rx++;
   }
   return rx;
}