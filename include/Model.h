#ifndef MODEL_H
#define MODEL_H

#include <time.h>

class Model {

public:
    Model();
    ~Model();


private:
    typedef struct erow {
        int size;
        int rsize;
        char *chars;
        char *render;
    } erow;

    int cx, cy;
    int rx;

    /* offsets for vertical/horizontal scroll */
    int rowoff;
    int coloff;

    int screenrows;
    int screencols;
    int numrows;

    /* Array of erow structs */
    erow *row;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time;

};

#endif /* MODEL_H */