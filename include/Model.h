#ifndef MODEL_H
#define MODEL_H

#include <time.h>

#define JED_TAB_STOP 8
#define JED_VERSION "0.0.1"

class Model {

public:
    typedef struct erow {
        int size;
        int rsize;
        char *chars;
        char *render;
    } erow;

    Model();
    ~Model();

    int getCx(){return cx;}
    void setCx(int _cx){cx = _cx;}

    int getCy(){return cy;}
    void setCy(int _cy){cy = _cy;}

    int getRx(){return rx;}
    void setRx(int _rx){rx = _rx;}

    int getNumrows(){return numrows;}
    void setNumrows(int n){numrows = n;}

    erow *getRow(){return row;}

    int getRowoff(){return rowoff;}
    void setRowoff(int r){rowoff = r;}

    int getColoff(){return coloff;}
    void setColoff(int c){coloff = c;}

    void updateRow(erow *row);
    void appendRow(char *s, size_t len);

    void openFile(char *filename);

    char *getFilename(){return filename;}
    char *getStatusmsg(){return statusmsg;}
    time_t getStatusmsgTime(){return statusmsg_time;}

private:

    int cx, cy;
    int rx;

    /* offsets for vertical/horizontal scroll */
    int rowoff;
    int coloff;

    int numrows;

    /* Array of erow structs */
    erow *row;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time;

};

#endif /* MODEL_H */