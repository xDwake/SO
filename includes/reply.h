#ifndef __REPLY__
#define __REPLY__

typedef struct reply {
    int type;
    char message[1024];
    int message_arguments;
    int to_unlink;
} Reply;

#endif