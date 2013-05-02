#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <dirent.h>

#include "server.h"

using namespace std;


int debug = 1;	 //if 1 it'll show output msgs surrounded by if(debug), for troubleshooting
//void* handleClient(void* sockfd);
//declare your function here

/*
struct arguments {
	int arg1;
	char* arg2;
};*/

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cout << "Usage: serverSNFS <port> <mount directory>\n";
		return 0;
	}
	

	
	int port = atoi(argv[1]);
	std::string directory = argv[2];
	std::cout << "\nPort number is: " << port << "\n";
	std::cout << "Mount Directory is: " << directory << "\n\n";
	if (port < 1024 || port > 65536)
	{
		std::cout << "Port number must be between 1024 and 65536\n";
		return 0;
	}
	
	
	
	  int sd, sd_current;
        unsigned int addrlen;
        struct sockaddr_in sin;
        struct sockaddr_in pin;
        int counter = 0;//keeps track of the threads created
        pthread_t pthreadClient[10];//array of 10 threads

        if(debug)
               printf("Listening for Connections on port: %d\n", port);

        //get socket
        if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
                perror("not able to open a socket");
                exit(1);
        }

        //socket structure
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);

        //bind socket to the given port
        if(bind(sd, (struct sockaddr*) &sin, sizeof(sin)) == -1){
                perror("not able to bind");
                exit(1);
        }

        while(1){
                if(listen(sd,5) == -1){ //not able to listen
                        perror("not able to listen");
                        exit(1);
                }

                //wait for clients to connect
                addrlen = sizeof(pin);
                if((sd_current = accept(sd, (struct sockaddr*) &pin, &addrlen)) == -1){
                        perror("not able to accept");
                        return 0;
                }
                //print out client name and port
                if(debug)
                        printf("Accepting client: %s, port: %d\n", inet_ntoa(pin.sin_addr), ntohs(pin.sin_port));

                if(debug){
                        printf("client sock: %d\n", sd_current);
                }
                
                /*
                struct arguments my_args;
                my_args.arg1 = sd_current;
                my_args.arg2 = 
                */
                
                //create thread for every connection
                // pthread_create(&pthreadClient[counter], NULL, handleClient, (void *)&sd_current);
                 pthread_create(&pthreadClient[counter], NULL, request_handler, (void *)&sd_current); //put function name
                 pthread_detach(pthreadClient[counter]);
                 counter++;
        }
	
		return 0;
}


void* request_handler(void* socket)
{
	char* buf;
	int* temp =(int*) socket;
	int sock = *temp;
	
	    
	int i;
	recv(sock, &i, sizeof(int), 0);
	i = ntohl(i);

	switch(i) 
	{
		case  1: handle_getattr(sock);
		/*
		case  2: handle_readdir(sock, buf);
		case  3: handle_open(sock);
		case  4: handle_read(sock, buf);
		case  5: handle_write(sock);
		case  6: handle_create(sock);
		case  7: handle_mkdir(sock);
		case  8: handle_releasedir(sock);
		case  9: handle_opendir(sock);
		case 10: handle_truncate(sock);
		case 11: handle_close(sock);
		*/
	}
}

void handle_getattr(int sock)
{
	int result;
	struct stat stbuf;
	char[sizeof(int) + sizeof(nlink_t) + sizeof(mode_t) + sizeof(off_t)] toSend;
	struct stat stbuf;

	int pathsize;
	recv(sock, &pathsize, sizeof(int), 0);
	pathsize = ntohl(pathsize);

	char* path;
	recv(sock, path, pathsize, 0);

	result = lstat(path, stbuf);
	result = htonl(result);
	
	nlink_t nlink = stbuf->st_nlink;
	nlink = htonl(nlink);
	mode_t mode = stbuf->st_mode;
	mode = htonl(mode);
	off_t size = stbuf->st_size;
	size = htonl(size);
	
	memcpy(toSend, &result, sizeof(int));
	memcpy(toSend + sizeof(int), &nlink, sizeof(nlink_t));
	memcpy(toSend + sizeof(int) + sizeof(nlink_t), &mode, sizeof(mode_t));
	memcpy(toSend + sizeof(int) + sizeof(nlink_t) + sizeof(mode_t), &off_t, sizeof(off_t));

	send(sock, toSend, sizeof(toSend), 0);
	
	return;
}

void handle_open(int sock)
{
	char* temp;
	int res;
	char[sizeof(int)] result;

	int pathsize;
	recv(sock, &pathsize, sizeof(int), 0);
	pathsize = ntohl(pathsize);

	struct fuse_file_info* fileInfoStruct;
	recv(sock, temp, sizeof(struct fuse_file_info), 0);
	memcpy(fileInfoStruct, temp, sizeof(struct fuse_file_info));

	char* path;
	recv(sock, path, pathsize, 0);

	res = open(path, temp->flags);
	res = htonl(res);

	memcpy(result, &res, sizeof(int));
	send(sock, result, sizeof(int), 0);

	return;
}