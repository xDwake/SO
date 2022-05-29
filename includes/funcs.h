#ifndef __FUNCS__
#define __FUNCS__

int readln(int fd, char* line, int size);

void read_config(char *path);

void send_feedback(char *feedback, int client_pid, Reply reply);

int validate_operation(char *operation);

int validate_ops(Request r);

int check_available_ops (Process p);

void enqueue(Process p);

void dequeue(Process p);

void finish(Process p);

void exec_transformation(Process p);

void exec_transformation_pipeline(Process p);

void processing();

#endif