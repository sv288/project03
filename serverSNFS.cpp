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

DIR *currentDirectory; //This should be declared here because it is going to vary during runtime 
DIR *previousDirectory; //A fallback directory to be used in the event that the user releases the directory he is currently in
DIR *initialDirectory; //To prevent the user from releasing the mount directory
int debug = 1;	 //if 1 it'll show output msgs surrounded by if(debug), for troubleshooting

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
	
	/*
	 *
	 * We should open the directory here. And I need to anyway in order to set up current, previous, and initial directories.
	 * 
	 */
	DIR *de
	de = opendir(directory);
	if (de == null)
		mkdir(directory, 0755);
	
	de = opendir(directory);
	initialDirectory = de;
	currentDirectory = de;
	
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
		case  2: handle_readdir(sock);
		case  3: handle_open(sock);
		//case  4: handle_read(sock);
		//case  5: handle_write(sock);
		//case  6: handle_create(sock);
		case  7: handle_mkdir(sock);
		case  8: handle_releasedir(sock);
		case  9: handle_opendir(sock);
		case 10: handle_truncate(sock);
		case 11: handle_close(sock);
	}
}

/* 
 * DS
 * FINISHED
 * NOT TESTED
 */
void handle_getattr(int sock)
{
	int result;
	struct stat stbuf;
	char[sizeof(int) + sizeof(nlink_t) + sizeof(mode_t) + sizeof(off_t)] toSend;
	struct stat stbuf;

	/* Receive/Unmarshall Pathsize */
	int pathsize;
	recv(sock, &pathsize, sizeof(int), 0);
	pathsize = ntohl(pathsize);

	/* Receive Path */
	char* path;
	recv(sock, path, pathsize, 0);

	/* Making system Call */
	result = lstat(path, stbuf);
	
	/* Marshalling Response to Client */
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

	/* Sending Response to Client */
	send(sock, toSend, sizeof(toSend), 0);
	
	return;
}

void handle_readdir(int sock)
{
	int res = 0;
	char[sizeof(int)] result;

	/* Receive/Unmarshall Pathsize */
	int pathsize;
	recv(sock, &pathsize, sizeof(int), 0);
	pathsize = ntohl(pathsize);

	/* Receive/Unmarshall mode */
	mode_t mode;
	recv(sock, @mode, sizeof(mode_t), 0);
	mode = ntohl(mode);

	/* Receive Path */
	char* path;
	recv(sock, path, pathsize, 0);

	/* Make System Call */
	DIR *de;
	de = opendir(path);

	if(de == null)
		res = -1;
	
	/* Marshalling Response to Client */
	res = htonl(res);
	memcpy(result, &res, sizeof(int));
	
	/* Sending Response to Client */
	send(sock, result, sizeof(int), 0);

	return;
}


/* 
 * DS
 * FINISHED
 * NOT TESTED
 */
void handle_open(int sock)
{
	char* temp;
	int res;
	char[sizeof(int)] result;

	/* Receive/Unmarshall Pathsize */
	int pathsize;
	recv(sock, &pathsize, sizeof(int), 0);
	pathsize = ntohl(pathsize);

	/* Receive/Unmarshall fileInfoStruct */
	struct fuse_file_info* fileInfoStruct;
	recv(sock, temp, sizeof(struct fuse_file_info), 0);
	memcpy(fileInfoStruct, temp, sizeof(struct fuse_file_info));

	/* Receive Path */
	char* path;
	recv(sock, path, pathsize, 0);

	/* Making System Call */
	res = open(path, fileInfoStruct->flags);

	/* Marshalling Response to Client */
	res = htonl(res);
	memcpy(result, &res, sizeof(int));
	
	/* Sending Response to Client */
	send(sock, result, sizeof(int), 0);

	return;
}

void handle_read(int sock)
{
	/* PAUL CODE HERE */
}

void handle_write(int sock)
{
	/* PAUL CODE HERE */
}

void handle_create(int sock)
{
	//sonu
	int res;
	char[sizeof(int)] result;

	/* Receive/Unmarshall Pathsize */
	int pathsize;
	recv(sock, &pathsize, sizeof(int), 0);
	pathsize = ntohl(pathsize);

	/* Receive/Unmarshall mode */
	mode_t mode;
	recv(sock, @mode, sizeof(mode_t), 0);
	mode = ntohl(mode);

	/* Receive Path */
	char* path;
	recv(sock, path, pathsize, 0);
	
	char structBuffer[struct fuse_file_info)];
	struct fuse_file_info* ffinfo;
	
	recv(sock, structBuffer, struct fuse_file_info), 0)
	memcpy(ffinfo, structBuffer, sizeof(struct fuse_file_info));
	
	/* Making System Call */
	res = create(path, ffinfo->flags);

	/* Marshalling Response to Client */
	res = htonl(res);
	memcpy(result, &res, sizeof(int));
	
	/* Sending Response to Client */
	send(sock, result, sizeof(int), 0);
	
	
	//end sonu
	/* PAUL CODE HERE */
}

void handle_mkdir(int sock)
{
	int res;
	char[sizeof(int)] result;

	/* Receive/Unmarshall Pathsize */
	int pathsize;
	recv(sock, &pathsize, sizeof(int), 0);
	pathsize = ntohl(pathsize);

	/* Receive/Unmarshall mode */
	mode_t mode;
	recv(sock, @mode, sizeof(mode_t), 0);
	mode = ntohl(mode);

	/* Receive Path */
	char* path;
	recv(sock, path, pathsize, 0);
	
	/* Making System Call */
	res = mkdir(path, mode);

	/* Marshalling Response to Client */
	res = htonl(res);
	memcpy(result, &res, sizeof(int));
	
	/* Sending Response to Client */
	send(sock, result, sizeof(int), 0);

	/* Make System Call */
	DIR *de;
	de = opendir(path);
	previousDirectory = currentDirectory;
	currentDirectory = de;
	
}

void handle_releasedir(int sock)
{
	int res = 0;
	char[sizeof(int)] result;

	/* Receive/Unmarshall Pathsize */
	int pathsize;
	recv(sock, &pathsize, sizeof(int), 0);
	pathsize = ntohl(pathsize);

	/* Receive/Unmarshall mode */
	mode_t mode;
	recv(sock, @mode, sizeof(mode_t), 0);
	mode = ntohl(mode);

	/* Receive Path */
	char* path;
	recv(sock, path, pathsize, 0);
	
	/*
	 *
	 *
	 * Not sure why a user would need to release a directory unless we are 
	 * Supposed to lock it once the user opens the directory. If so, we need 
	 * A global linked list of directories currently opened and we must ensure 
	 * No other user has that directory opened before we allow this to proceed.
	 *
	 *
	 */
	
	if (initialDirectory == currentDirectory)
		res = -1;
	
	else
		currentDirectory = previousDirectory;
		
	/* Marshalling Response to Client */
	res = htonl(res);
	memcpy(result, &res, sizeof(int));
	
	/* Sending Response to Client */
	send(sock, result, sizeof(int), 0);

}

void handle_opendir(int sock)
{
	int res = 0;
	char[sizeof(int)] result;

	/* Receive/Unmarshall Pathsize */
	int pathsize;
	recv(sock, &pathsize, sizeof(int), 0);
	pathsize = ntohl(pathsize);

	/* Receive/Unmarshall mode */
	mode_t mode;
	recv(sock, @mode, sizeof(mode_t), 0);
	mode = ntohl(mode);

	/* Receive Path */
	char* path;
	recv(sock, path, pathsize, 0);
	
	/* Make System Call */
	DIR *de
	de = opendir(path);
	
	if(de == null)
		res = -1;
	currentDirectory = de;
	
	/* Marshalling Response to Client */
	res = htonl(res);
	memcpy(result, &res, sizeof(int));
	
	/* Sending Response to Client */
	send(sock, result, sizeof(int), 0);

	return;

}

/* 
 * DS
 * FINISHED
 * NOT TESTED
 */
void handle_truncate(int sock)
{
	int res;
	char[sizeof(int)] result;

	/* Receive/Unmarshall Pathsize */
	int pathsize;
	recv(sock, &pathsize, sizeof(int), 0);
	pathsize = ntohl(pathsize);

	/* Receive/Unmarshall Offset */
	off_t offset;
	recv(sock, @offset, sizeof(off_t), 0);
	offset = ntohl(offset);

	/* Receive Path */
	char* path;
	recv(sock, path, pathsize, 0);
	
	/* Making System Call */
	res = truncate(path, offset);

	/* Marshalling Response to Client */
	res = htonl(res);
	memcpy(result, &res, sizeof(int));
	
	/* Sending Response to Client */
	send(sock, result, sizeof(int), 0);

	return;
}

void handle_close (int sock)
{
	int res;
	char[sizeof(int)] result;


	/* Receive/Unmarshall Pathsize */
	int pathsize;
	recv(sock, &pathsize, sizeof(int), 0);
	pathsize = ntohl(pathsize);

	/* Receive Path */
	char* path;
	recv(sock, path, pathsize, 0);
	
	/* Making System Call */
	res = close(path);

	/* Marshalling Response to Client */
	res = htonl(res);
	memcpy(result, &res, sizeof(int));
	
	/* Sending Response to Client */
	send(sock, result, sizeof(int), 0);

}
