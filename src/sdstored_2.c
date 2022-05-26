#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "../includes/request.h"
#include "../includes/reply.h"
#include "../includes/transformation.h"
#include "../includes/process.h"

#define MAX_BUFFER_SIZE 1024
#define CLIENT_TO_SERVER_FIFO "../tmp/client_to_server_fifo"


int fd, bytes_read;
int number_of_transformations = 0;
int processes_in_queue=0;

char transformations_location[MAX_BUFFER_SIZE];

Transformation list_of_transformation[128];

Process process_queue[30];


int readln(int fd, char* line, int size){
    int next_pos=0;
    while(next_pos<size && read(fd, line + next_pos,1)>0){
    	if(line[next_pos++]=='\n')
    		break;
    }
    return next_pos;
}

void read_config(char *path){
	if((fd = open(path, O_RDONLY)) == -1){
		perror("error opening source file");
	}
	char buffer[MAX_BUFFER_SIZE];

	while((readln(fd, buffer, MAX_BUFFER_SIZE)) > 0){
		char *token=strtok(buffer, " ");
		for(int i=0;i<2;i++){
			if(i==0){
				strcpy(list_of_transformation[number_of_transformations].name, token);
			}
			if(i==1){
				list_of_transformation[number_of_transformations].max=atoi(token);
			}
			token = strtok(NULL, " ");
		}
		list_of_transformation[number_of_transformations].running=0;
		number_of_transformations++;
	}
	close(fd);
}


void send_feedback(char *feedback, int client_pid){
	Reply reply;
    reply.message_arguments = 1;

    strcpy(reply.message, feedback);
    int server_to_client_fifo;
    char server_to_client_fifo_name[128];
    sprintf(server_to_client_fifo_name, "../tmp/%d", client_pid);
	
	server_to_client_fifo = open(server_to_client_fifo_name, O_WRONLY);
    
    write(server_to_client_fifo, &reply, sizeof(Reply));
    close(server_to_client_fifo);
}

int validate_operation(char *operation){
	int bool=0;
	for(int i=0;i<number_of_transformations && bool==0;i++){
		if(strcmp(operation, list_of_transformation[i].name)==0){
			bool=1;
		}
	}
	return bool;
}

int validate_ops(Request r){
	int bool;
	for(int i=2;i<r.argc;i++){
		bool = validate_operation(r.ops[i]);
	}
	return bool;
}

void enqueue(Process p){
	process_queue[processes_in_queue]=p;
	processes_in_queue++;
}

void dequeue(){
	for(int i=0;i<processes_in_queue-1;i++){
		process_queue[i]=process_queue[i+1];
	}
	processes_in_queue--;
}

void print_request(Request r){
	printf("pid:%d type:%d argc:%d\n", r.pid, r.type, r.argc);
	for(int i=0;i<r.argc;i++){
		printf("%s\n", r.ops[i]);
	}
}

void print_transformations(Transformation t){
	printf("name:%s number:%d\n", t.name, t.max);
}

void print_process(Process p){
	printf("pid:%d state:%d input:%s output:%s\n", p.client_pid, p.state, p.input_file, p.output_file);
	for(int i=0;i<p.no_of_transformations;i++){
		printf("%s\n", p.transformations[i]);
	}
	printf("***\n");
}

void print_list_processes(){
	for(int i=0;i<processes_in_queue;i++){
		print_process(process_queue[i]);
	}
}


int main(int argc, char *argv[])
{

	/*
	strcpy(transformations_location, argv[2]);
	*/
	read_config(argv[1]);
	
	if(mkfifo(CLIENT_TO_SERVER_FIFO, 0666)==-1)
		perror("Error making fifo!");

	else
		printf("Fifo opened for reading\n");

	fd=open(CLIENT_TO_SERVER_FIFO, O_RDONLY, 0666);
	Request request;

	bytes_read = read(fd, &request, sizeof(Request));
	//print_request(request);

	if(request.type==1 && request.argc>2){
		send_feedback("Pending...\n", request.pid);
		if(validate_ops(request)==0){
			send_feedback("Invalid transformations!", request.pid);
		}
		else{
			Process process;
			strcpy(process.input_file, request.ops[0]);
			strcpy(process.output_file, request.ops[1]);
			for(int i=2;i<request.argc;i++){
				strcpy(process.transformations[i-2], request.ops[i]);
			}
			process.client_pid=request.pid;
			process.no_of_transformations=request.argc-2;
			process.state=0;
			enqueue(process);
			print_list_processes();
		}
	}

	close(fd);
	return 0;
}