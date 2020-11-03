#include <model.h>

Model::Model() : cx(0), cy(0), rx(0), rowoff(0), coloff(0), numrows(0),
        row(NULL), filename(NULL) {
    statusmsg[0] = '\0';
    statusmsg_time = 0;
}

Model::~Model() {

}