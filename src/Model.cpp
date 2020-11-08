#include <Model.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <stdarg.h>

Model::Model() : cx(0), cy(0), rx(0), rowoff(0), coloff(0), numrows(0),
        row(NULL), filename(NULL) {
    statusmsg[0] = '\0';
    statusmsg_time = 0;
}

Model::~Model() {

}

void Model::openFile(char *filename) {
   //free(filename);
   filename = strdup(filename);

   FILE *fp = fopen(filename, "r");
   if(!fp) {
       /* TODO die */
   }

   char *line = NULL;
   size_t linecap = 0;
   ssize_t linelen = 13;
   
   linelen = getline(&line, &linecap, fp);
   /* Read the entire file line by line */
   while((linelen = getline(&line, &linecap, fp)) != -1) {
      /* Strip trailing newlines or carriage returns */
      while (linelen > 0 && (line[linelen - 1] == '\n' ||
               line[linelen - 1] == '\r')) {
         linelen--;
      }
      appendRow(line, linelen);
   }
   free(line);
   fclose(fp);
}


void Model::updateRow(Model::erow *row) {
   int tabs = 0;
   int j;
   for(j = 0; j < row->size; ++j) {
      if(row->chars[j] == '\t') {
         tabs++;
      }
   }

   free(row->render);
   row->render = (char *)malloc(row->size + tabs * (JED_TAB_STOP - 1) + 1);

   int idx = 0;
   for(j = 0; j < row->size; j++) {
      if (row->chars[j] == '\t'){
         row->render[idx++] = ' ';
         while(idx % JED_TAB_STOP != 0) {
            row->render[idx++] = ' ';
         }
      } else {
         row->render[idx++] = row->chars[j];
      }
   }
   row->render[idx] = '\0';
   row->rsize = idx;
}

void Model::appendRow(char *s, size_t len) {
   row = (Model::erow *)realloc(row, sizeof(erow) * (numrows + 1));

   int at = numrows;
   row[at].size = len;
   row[at].chars = (char *)malloc(len + 1);
   memcpy(row[at].chars, s, len);
   row[at].chars[len] = '\0';

   row[at].rsize = 0;
   row[at].render = NULL;
   updateRow(&row[at]);

   numrows++;
}

void Model::setStatusMessage(const char *fmt, ...) {
   va_list ap;
   va_start(ap, fmt);
   vsnprintf(statusmsg, sizeof(statusmsg), fmt, ap);
   va_end(ap);
   statusmsg_time = time(NULL);
}