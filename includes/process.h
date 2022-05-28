#ifndef __PROCESS__
#define __PROCESS__

typedef struct process {
    int process_number;
    int client_pid;
    char input_file[1024];
    char output_file[1024];
    char transformations[16][16];
    int no_of_transformations;
    int state;                  //0 in queue, 1 active, 2 done
} Process;

#endif