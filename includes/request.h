#ifndef __REQUEST__
#define __REQUEST__

typedef struct request {
    int request_number;
    int pid;
    int type; //0 for status, 1 for proc-file
    int argc;
    char ops[160][160];
} Request;

#endif