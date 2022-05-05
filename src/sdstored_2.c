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

#define MAX_BUFFER_SIZE 1024
#define CLIENT_TO_SERVER_FIFO "../tmp/client_to_server_fifo"


int fd, bytes_read;
int number_of_transformations = 0;

char transformations_location[MAX_BUFFER_SIZE];

Transformation list_of_transformation[128];


int readln(int fd, char* line, int size){
    int next_pos=0;
    while(next_pos<size && read(fd, line + next_pos,1)>0){
    	if(line[next_pos++]=='\n')
    		break;
    }
    return next_pos;
}

void read_config(char *path){
	fd=open(path, O_RDONLY);
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
		number_of_transformations++;
	}
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

void print_request(Request r){
	printf("pid:%d type:%d argc:%d\n", r.pid, r.type, r.argc);
	for(int i=0;i<r.argc;i++){
		printf("%s\n", r.ops[i]);
	}
}

void print_transformations(Transformation t){
	printf("name:%s number:%d\n", t.name, t.max);
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
	}

	close(fd);
	return 0;
}