#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../includes/request.h"
#include "../includes/reply.h"

#define CLIENT_TO_SERVER_FIFO "../tmp/client_to_server_fifo"


char server_to_client_fifo_name[128];

int validate_request(char** argv, int argc){
	char error_message[128];
	if((strcmp(argv[1], "status") == 0) || (strcmp(argv[1], "proc-file") == 0)){
		if((strcmp(argv[1], "status") == 0) && argc > 2){
			int error_message_size = sprintf(error_message, "Invalid status command.\n");
        	write(1, error_message, error_message_size);
        	return 0;
		}
		else if((strcmp(argv[1], "proc-file") == 0) && argc < 5){
			int error_message_size = sprintf(error_message, "Invalid transformation command: not enough arguments.\n");
        	write(1, error_message, error_message_size);
        	return 0;
		}
	}
	else{
		int error_message_size = sprintf(error_message, "Invalid command.\n");
        write(1, error_message, error_message_size);
        return 0;
	}
	return 1;
}

void make_request(int fd, char** argv, int argc) {
    int i;
    Request request;
    request.pid = getpid();
    
    if(strcmp(argv[1], "status")==0)
    	request.type=0;
    else if(strcmp(argv[1], "proc-file")==0)
    	request.type=1;

    	else
    		request.type=2;

    request.argc = argc - 2;
    for (i = 2; i < argc; i++)
    	strcpy(request.ops[i - 2], argv[i]);
    write(fd, &request, sizeof(Request));
}


void wait_reply() {
    Reply reply;
    int server_to_client_fifo = open(server_to_client_fifo_name, O_RDONLY);
    while(1){
        while(read(server_to_client_fifo, &reply, sizeof(Reply)) > 0){
			write(1, reply.message, strlen(reply.message));
			if(reply.to_unlink){
                close(server_to_client_fifo);
                exit(0);
            }
        }
    }
    close(server_to_client_fifo);
}

//just for debugging
void print_request(Request r){
	printf("pid:%d type:%d argc:%d\n", r.pid, r.type, r.argc);
	for(int i=0;i<r.argc;i++){
		printf("%s\n", r.ops[i]);
	}
}

int main(int argc, char *argv[])
{
	char possible_command1[128];
	char possible_command2[128];

	sprintf(server_to_client_fifo_name, "../tmp/%d", (int)getpid());

	if(mkfifo(server_to_client_fifo_name, 0666)==-1)
		perror("Error making fifo!");

	if(argc > 1){
		if(validate_request(argv, argc)==1){
			int client_to_server_fifo = open(CLIENT_TO_SERVER_FIFO, O_WRONLY);
            make_request(client_to_server_fifo, argv, argc);
            close(client_to_server_fifo);
            wait_reply();
		}
	}
	else{
        int possible_command1_size = sprintf(possible_command1, "%s status.\n", argv[0]);
        int possible_command2_size = sprintf(possible_command2, "%s proc-file <priority> input-filename output-filename transformation-id-1 transformation-id-2 ...", argv[0]);
        write(1, possible_command1, possible_command1_size);
        write(1, possible_command2, possible_command2_size);
	}

	return 0;
}
