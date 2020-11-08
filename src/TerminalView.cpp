#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "TerminalView.h"
#include "Model.h"

using namespace std;

void TerminalView::draw() {
    struct abuf ab = ABUF_INIT;

    /* Set and reset commands h used to turn off cursor with ?25 argument */
    abAppend(&ab, "\x1b[?25l", 6);
    /* Cursor position escape sequence */
    abAppend(&ab, "\x1b[H", 3);
   
    drawRows(&ab);
    drawStatusBar(&ab);
    drawMessageBar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", 
            (model->getCy()- model->getRowoff()) + 1, 
            (model->getRx() - model->getColoff()) + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

void TerminalView::drawRows(struct abuf *ab) {
   int y;
   for(y = 0; y < screenrows; ++y) {
      int filerow = y + model->getRowoff();
      if(filerow >= model->getNumrows()) {
         /* Only display welcome if text buffer is empty (no file specified) */
         if (model->getNumrows() == 0 && y == screenrows / 3) {
            char welcome[80];
            int welcomelen = snprintf(welcome, sizeof(welcome),
                  "James Editor (Jed) -- version %s", JED_VERSION);
            if(welcomelen > screencols ){
               welcomelen = screencols;
            }
            int padding = (screencols - welcomelen) / 2;
            if(padding) {
               abAppend(ab, "~", 1);
               padding--;
            }
            while(padding--) {
               abAppend(ab, " ", 1);
            }
            abAppend(ab, welcome, welcomelen);
         } else {
            abAppend(ab, "~", 1);
         }
      }
      else {
         int len = model->getRow()[filerow].rsize - model->getColoff();
         if(len < 0) {
            len = 0;
         }
         if(len > screencols) {
            len = screencols;
         }
         abAppend(ab, &model->getRow()[filerow].render[model->getColoff()], len);
      }

      /* Erase line escape sequence to clear line-by-line*/
      abAppend(ab, "\x1b[K", 3);
      abAppend(ab, "\r\n", 2);
   }
}

void TerminalView::drawStatusBar(struct abuf *ab) {
   abAppend(ab, "\x1b[7m", 4);
   char status[80];
   char rstatus[80];

   int len = snprintf(status, sizeof(status), "%.20s - %d lines",
         model->getFilename() ? model->getFilename() : "[No Name]", model->getNumrows());
   int rlen = snprintf(rstatus, sizeof(rstatus), "%d%d",
         model->getCy() + 1, model-> getNumrows());
   
   if (len > screencols) {
      len = screencols;
   }

   abAppend(ab, status, len);

   while (len < screencols) {
      if(screencols - len == rlen) {
         abAppend(ab, rstatus, rlen);
         break;
      } else {
         abAppend(ab, " ", 1);
         len++;
      }
   }
   abAppend(ab, "\x1b[m", 3);
   abAppend(ab, "\r\n", 2);
}

void TerminalView::drawMessageBar(struct abuf *ab) {
   abAppend(ab, "\x1[K", 3);
   int msglen = strlen(model->getStatusmsg());
   if(msglen > screencols) {
      msglen = screencols;
   }
   if(msglen && time(NULL) - model->getStatusmsgTime() < 5) {
      abAppend(ab, model->getStatusmsg(), msglen);
   }
}

TerminalView::TerminalView(Model *model) : model(model){
    enableRawMode();
}

TerminalView::~TerminalView() {
    disableRawMode();
}

void TerminalView::enableRawMode() {
    if(tcgetattr(STDIN_FILENO, &origTermios) == -1) {
        /* TODO! */
    }
    struct termios raw = origTermios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

   tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void TerminalView::disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &(this->origTermios)) == -1) {
        //TODO
    }
}

void TerminalView::abAppend(TerminalView::abuf *ab, const char *s, int len) {
   char *new_b = (char *)realloc(ab->b, ab->len + len);

   if(new_b == NULL) {
      return;
   }
   memcpy(&new_b[ab->len], s, len);
   ab->b = new_b;
   ab->len += len;
}

void TerminalView::abFree(abuf *ab) {
   free(ab->b);
}