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

int flag = 1;
int inc_request = 1;
int fd, bytes_read;
int number_of_transformations = 0;
int processes_in_queue=0;

char transformations_location[MAX_BUFFER_SIZE];

Transformation list_of_transformation[16];

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


void send_feedback(char *feedback, int client_pid, Reply reply){
    reply.message_arguments = 1;

    strcpy(reply.message, feedback);
    int server_to_client_fifo;
    char server_to_client_fifo_name[128];
    sprintf(server_to_client_fifo_name, "../tmp/%d", client_pid);
    if(reply.type==1)
    	reply.to_unlink=1;
    else
    	reply.to_unlink=0;
    //printf("%s\n", reply.message);
    //printf("%s\n", server_to_client_fifo_name);
	
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

int check_available_ops (Process p){
	int flag=1;
	for(int i = 0; i < p.no_of_transformations && flag;i++){
		for(int j = 0; j < number_of_transformations && flag;j++){
			if(strcmp(p.transformations[i], list_of_transformation[j].name)==0){
				if(list_of_transformation[j].running < list_of_transformation[j].max){
					list_of_transformation[j].running++;
				}
				else
					flag=0;
			}
		}
	}
	return flag;	
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

void exec_transformation(Process p){
	int pid;
	int status;
	int ret;
	int input_file_fd;
	int output_file_fd;
	char transformation_path[128];
	if((pid=fork())==0){
		
		input_file_fd=open(p.input_file, O_RDONLY);

		if((input_file_fd = open(p.input_file, O_RDONLY)) == -1){
			perror("error opening source file");
		}

		if((output_file_fd = open(p.output_file, O_CREAT | O_WRONLY, 0666)) == -1){
			perror("error creating output file");
		}

		
		dup2(input_file_fd, 0);
		close(input_file_fd);

		dup2(output_file_fd, 1);
		close(output_file_fd);

		strcpy(transformation_path, transformations_location);
		strcat(transformation_path, p.transformations[0]);

		ret=execlp(transformation_path, transformation_path, 0, 1, NULL);
		_exit(ret);
	}

	else{
		pid_t terminated=wait(&status);
		if(WIFEXITED(status))
			printf("[Father] - Child with process id %d has finished with code %d\n", terminated, WEXITSTATUS(status));
		else
			printf("error\n");
	}
}

void exec_transformation_pipeline(Process p){
	int pid;
	int ret;
	int status[p.no_of_transformations];
	int input_file_fd;
	int output_file_fd;
	char transformation_path[128];
	int pipes[p.no_of_transformations-1][2];
	for(int i=0;i<p.no_of_transformations;i++){
		if(i==0){		//first transformation in line
			if(pipe(pipes[i])<0){
				perror("Error creating pipe");
			}
			switch(fork()){
				case -1:
					perror("Error forking");

				case 0:
					close(pipes[i][0]);

					if((input_file_fd = open(p.input_file, O_RDONLY)) == -1){
						perror("error opening source file");

					}

					dup2(input_file_fd, 0);
					close(input_file_fd);

					dup2(pipes[i][1],1);
					close(pipes[i][1]);

					strcpy(transformation_path, transformations_location);
					strcat(transformation_path, p.transformations[i]);

					ret=execlp(transformation_path, transformation_path, 0, 1, NULL);
					_exit(ret);

				default:
					close(pipes[i][1]);
			}
		}
		else if(i==p.no_of_transformations-1){
			switch(fork()){
				case -1:
					perror("Error forking");

				case 0:
					dup2(pipes[i-1][0],0);
					close(pipes[i-1][0]);

					if((output_file_fd = open(p.output_file, O_CREAT | O_WRONLY, 0666)) == -1){
						perror("error creating output file");
					}

					dup2(output_file_fd, 1);
					close(output_file_fd);

					strcpy(transformation_path, transformations_location);
					strcat(transformation_path, p.transformations[i]);

					ret=execlp(transformation_path, transformation_path, 0, 1, NULL);
					_exit(ret);

				default:
					close(pipes[i-1][0]);

			}
		}
		else{
			if(pipe(pipes[i])<0){
				perror("Error creating pipe");
			}
			switch(fork()){
				case -1:
					perror("Error forking");

				case 0:
					close(pipes[i][0]);
					//writing to pipe i
					dup2(pipes[i][1],1);
					close(pipes[i][1]);
					//reading from pipe i-1
					dup2(pipes[i-1][0],0);
					close(pipes[i-1][0]);


					strcpy(transformation_path, transformations_location);
					strcat(transformation_path, p.transformations[i]);
					
					ret=execlp(transformation_path, transformation_path, 0, 1, NULL);
					_exit(ret);

				default :
					close(pipes[i-1][0]);
					close(pipes[i][1]);
			}
		}
	}
	for(int w=0;w<p.no_of_transformations;w++){
		pid_t terminated=wait(&status[w]);
		if(WIFEXITED(status[w]))
			printf("[Father] - Child with process id %d has finished with code %d\n", terminated, WEXITSTATUS(status[w]));
		else
			printf("error\n");
	}
}

void processing(){
	for(int i=0;i<processes_in_queue;i++){
		if(process_queue[i].state==0){
			if(check_available_ops(process_queue[i])==1){				//tem slots livres para fazer o pedido
				process_queue[i].state=1;
				Reply r;
				r.type=0;
				send_feedback("Processing...\n", process_queue[i].client_pid, r);
				//processar o pedido
				if(process_queue[i].no_of_transformations==1){
					Process p1=process_queue[i];
					dequeue();
					exec_transformation(p1);
				}
				else{
					Process p1=process_queue[i];
					dequeue();
					exec_transformation_pipeline(process_queue[i]);
				}
			}
		}
	}
}

void send_status(char* message){ // done
	int pipe_fd[2];
	if(pipe(pipe_fd) == -1){
		perror("Error creating pipe.\n");
	}

	dup2(pipe_fd[1],1);
	dup2(pipe_fd[0],0);
	close(pipe_fd[1]);
	close(pipe_fd[0]);

	for(int i = 0; i < sizeof(process_queue);i++){
		char processo[128];
		if(process_queue[i].state == 1){
			Process p = process_queue[i]; 
				snprintf(processo,sizeof(processo),"task#%d proc-file %s %s ",p.process_number,p.output_file, p.input_file); // alterar se adicionarmos priorities!!
				write(1,processo,sizeof(processo));
				for(int j = 0; j < p.no_of_transformations;i++){
					write(1,p.transformations[i],sizeof(p.transformations[i]));
				}
				
				write(1,"\n", sizeof(char));
		}
	}

	char subtitle[20] = "(running/max)\n";

	for(int i = 0; i < 7;i++){
		char transformacao [128];
		Transformation t = list_of_transformation[i];
		snprintf(transformacao,sizeof(transformacao),"transf %s: %d/%d %s", t.name,t.running,t.max,subtitle);
		write(1,transformacao,sizeof(transformacao));
		write(1,"\n", sizeof(char));
	}
	
	
	read(pipe_fd[0],message,sizeof(message));

}

void sigtermhandler(int signum){ // altera a flag de modo a terminar o programa
	flag = 0;
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

	strcpy(transformations_location, argv[2]);
	
	read_config(argv[1]);
	
	if(mkfifo(CLIENT_TO_SERVER_FIFO, 0666)==-1)
		perror("Error making fifo!");

	else
		printf("Fifo opened for reading\n");

	Request request;
	Reply r;

	signal(sigtermhandler,SIGTERM);
	
	alarm(1);
	while(flag){
		char status_message[2048];

		fd=open(CLIENT_TO_SERVER_FIFO, O_RDONLY, 0666);
			
		while(bytes_read = read(fd, &request, sizeof(Request))>0 && flag){

			request.request_number = inc_request;
			inc_request++;

			if(request.type==1 && request.argc>2){
				r.type=0;
				send_feedback("Pending...\n", request.pid, r);
				if(validate_ops(request)==0){
					r.type=1;
					send_feedback("Invalid transformations!\n", request.pid, r);
				}
				else{
					Process process;
	
					strcpy(process.input_file, request.ops[0]);
					strcpy(process.output_file, request.ops[1]);
					for(int i=2;i<request.argc;i++){
						strcpy(process.transformations[i-2], request.ops[i]);
					}
					process.process_number= request.request_number;
					process.client_pid=request.pid;
					process.no_of_transformations=request.argc-2;
					process.state=0;
					//char transformation_path[128];
					//strcpy(transformation_path, transformations_location);
					//strcat(transformation_path, process.transformations[0]);
					//teste(process, transformation_path);
					//exec_transformation(process);
					enqueue(process);
					//print_list_processes();
					processing();
				}
			}
			else{
				send_status(status_message);
				send_feedback(status_message,request.pid,r);
			}
	
		}
		if(flag == 0){//se  nao funcionar remover codigo daqui ate linha 474
			while(process_queue){
				if(request.type==1 && request.argc>2){
				r.type=0;
				send_feedback("Pending...\n", request.pid, r);
				if(validate_ops(request)==0){
					r.type=1;
					send_feedback("Invalid transformations!\n", request.pid, r);
				}
				else{
					Process process;
	
					strcpy(process.input_file, request.ops[0]);
					strcpy(process.output_file, request.ops[1]);
					for(int i=2;i<request.argc;i++){
						strcpy(process.transformations[i-2], request.ops[i]);
					}
					process.process_number= request.request_number;
					process.client_pid=request.pid;
					process.no_of_transformations=request.argc-2;
					process.state=0;
					//char transformation_path[128];
					//strcpy(transformation_path, transformations_location);
					//strcat(transformation_path, process.transformations[0]);
					//teste(process, transformation_path);
					//exec_transformation(process);
					enqueue(process);
					//print_list_processes();
					processing();
				}
			}
			else{
				send_status(status_message);
				send_feedback(status_message,request.pid,r);
			}
			}
		}
		close(fd);
	}
	return 0;
}