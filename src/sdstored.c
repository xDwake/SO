#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include "../includes/sdstore.h"

int main(int argc, char* argv[]){
    mkfifo("client_server_fifo",0664);
    mkfifo("server_client_fifo",0664);
    
    Request req;

    char buffer = calloc(4096,sizeof(char));
    int fileconf = open(argv[1],O_RDONLY);
    char* limits = "\n";
    read(fileconf,&buffer,sizeof(buffer));
    char* oplimits[7][20];
    
    for (char *token = strtok(buffer,limits); token != NULL; token = strtok(NULL, limits)){
        for(int i = 0;i < 7;i++){
            for(int j = 0;token[j] != '\n';j++){
            op[i][j] = token[j];
            }
        }
    }

    int client_server_fifo = open("client_server_fifo",O_RDONLY);
    int server_client_fifo = open("server_client_fifo",O_WRONLY);

    read(client_server_fifo,&req,sizeof(Request));


    

}

