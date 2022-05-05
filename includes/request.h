#ifndef __REQUEST__
#define __REQUEST__

typedef struct request {
    int pid;
    int type; //0 for status, 1 for proc-file
    int argc;
    char ops[16][16];
} Request;

#endif