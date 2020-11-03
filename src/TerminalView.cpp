#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "TerminalView.h"
#include "Model.h"

using namespace std;

void TerminalView::draw() {
    return;
}

TerminalView::TerminalView(Model *model) : model(model){
    enableRawMode();
}

TerminalView::~TerminalView() {
    disableRawMode();
}

int TerminalView::getWindowSize(int *rows, int *cols){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* Move cursor to bottom right if window size failed */
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }
        /* Get window size from cursor at bottom right */
        return getCursorPosition(rows, cols);
    }
    else {
        /* Gather window size info if window size succeeded */
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
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
    
}

