#ifndef __REPLY__
#define __REPLY__

typedef struct reply {
    int type;               //0 for normal, 1 for error, 2 for finish message, 3 for status message
    char message[4096];
    int message_arguments;
    int to_unlink;          //1 to unlink, 2 unlink status
} Reply;

#endif