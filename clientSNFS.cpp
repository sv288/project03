#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>

#include <fuse.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/stat.h>
#include "client.h"

//compilation is g++ clientSNFS.cpp -o clientSNFS
//clientSNFS.exe <args>
using namespace std;
int debug = 1;
int sock;

/* 
 * DS
 * FINISHED
 * NOT TESTED
 */
static int my_getattr(const char* path, struct stat* stbuf)
{
	int type = 1;
	int pathsize = strlen(path) + 1;
	char data[sizeof(int)*2 + pathsize];
	
	/* Marshalling Data */
	memset(data, 0, sizeof(data));	
	type = htonl(type);
	memcpy(&data, &type, sizeof(int));
	memcpy(&data + sizeof(int)*2, path, pathsize);
	pathsize = htonl(pathsize);
	memcpy(&data + sizeof(int), &pathsize, sizeof(int));
	
	/* Sending Data to Server */
	send(sock, data, sizeof(data), 0);
	
	/* Receiving/Unmarshalling Response */
	int result;
	recv(sock, &result, sizeof(int), 0);
	result = ntohl(result);
	nlink_t nlink;
	recv(sock, &nlink, sizeof(nlink_t), 0);
	nlink = htonl(nlink);
	mode_t mode;
	recv(sock, &mode, sizeof(mode_t), 0);
	mode = htonl(mode);
	off_t size;
	recv(sock, &size, sizeof(off_t), 0);
	size = htonl(size);
	
	/* Setting and Returning Values */
	stbuf->st_nlink = nlink;
	stbuf->st_mode = mode;
	stbuf->st_size = size;	
	return result;
}

static int my_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fileInfoStruct)
{
	int type = 2;
	int pathsize = strlen(path) + 1;
	
	char data[sizeof(int)*2 + pathsize];

	/* Marshalling Data */
	memset(data, 0, sizeof(data));	
	type = htonl(type);
	memcpy(&data, &type, sizeof(int));
	pathsize = htonl(pathsize);
	memcpy(&data + sizeof(int), &pathsize, sizeof(int));
	memcpy(&data + sizeof(int)*2, path, pathsize);
	
	/* Sending Data to Server */
	send(sock, data, sizeof(data), 0);

	
	/* Receiving/Unmarshalling Response */
	int result;
	recv(sock, &result, sizeof(int), 0);
	result = ntohl(result);
	
	/* Returning Value */
	return result;
}

/* 
 * DS
 * FINISHED
 * NOT TESTED
 */
static int my_open(const char* path, struct fuse_file_info* fileInfoStruct)
{
	int type = 3;
	int pathsize = strlen(path) + 1;
	int structsize = sizeof(struct fuse_file_info);
	char data[sizeof(int)*2 + pathsize + structsize];
	
	/* Marshalling Data */
	memset(data, 0, sizeof(data));	
	type = htonl(type);
	memcpy(&data, &type, sizeof(int));
	memcpy(&data + sizeof(int)*2, fileInfoStruct, structsize);
	memcpy(&data + sizeof(int)*2 + structsize, path, pathsize);
	pathsize = htonl(pathsize);
	memcpy(&data + sizeof(int), &pathsize, sizeof(int));
	
	/* Sending Data to Server */
	send(sock, data, sizeof(data), 0);
	
	/* Receiving/Unmarshalling Response */
	int result;
	recv(sock, &result, sizeof(int), 0);
	result = ntohl(result);
	
	/* Returning Value */
	return result;
}

static int my_read(const char *path, char* buf, size_t size, off_t offset, struct fuse_file_info* fileInfoStruct)
{
	int type = 4;
	/* PAUL CODE */
}

static int my_write(const char* path, const char* towrite, size_t size, off_t offset, struct fuse_file_info* fileInfoStruct)
{
	int type = 5;
	/* PAUL CODE */
}

static int my_create(const char* path, mode_t mode, struct fuse_file_info* fileInfoStruct)
{
	int type = 6;
	//sonu
	int pathsize = strlen(path) + 1;
	
	/* Marshalling Data */
	char data[sizeof(int)*2 + pathsize + sizeof(mode_t) + sizeof(struct fuse_file_info)];
	memset(data, 0, sizeof(data));
	type = htonl(type);
	memcpy(&data, &type, sizeof(int));
	mode = htonl(mode);
	memcpy(&data + sizeof(int)*2, &mode, sizeof(mode_t));
	memcpy(&data + sizeof(int)*2 + sizeof(mode_t), path, pathsize);
	pathsize = htonl(pathsize);
	memcpy(&data + sizeof(int), pathsize, sizeof(int));
	memcpy(&data + sizeof(int)*2 + sizeof(mode_t) + pathsize, fileInfoStruct, sizeof(struct fuse_file_info));
		
	/* Sending Data to Server */
	send(sock, data, sizeof(data), 0);
	
	/* Receiving/Unmarshalling Response */
	int result;
	recv(sock, &result, sizeof(int), 0);
	result = ntohl(result);
	
	/* Returning Value */
	return result;
	
	
	//end sonu
	
	/* PAUL CODE */
}

static int my_mkdir(const char* path, mode_t mode)
{
	int type = 7;
	int pathsize = strlen(path) + 1;
	
	/* Marshalling Data */
	char data[sizeof(int)*2 + pathsize + sizeof(mode_t)];
	memset(data, 0, sizeof(data));
	type = htonl(type);
	memcpy(&data, &type, sizeof(int));
	mode = htonl(mode);
	memcpy(&data + sizeof(int)*2, &mode, sizeof(mode_t));
	memcpy(&data + sizeof(int)*2 + sizeof(mode_t), path, pathsize);
	pathsize = htonl(pathsize);
	memcpy(&data + sizeof(int), pathsize, sizeof(int));
	
	/* Sending Data to Server */
	send(sock, data, sizeof(data), 0);
	
	/* Receiving/Unmarshalling Response */
	int result;
	recv(sock, &result, sizeof(int), 0);
	result = ntohl(result);
	
	/* Returning Value */
	return result;
}

static int my_releasedir(const char* path, struct fuse_file_info* fileInfoStruct)
{
	int type = 8;
	int pathsize = strlen(path) + 1;
	
	/* Check that directory exists */
	if (my_opendir(path, null, null, null, null) != 0)
		return -1;
	
	char data[sizeof(int)*2 + pathsize];

	/* Marshalling Data */
	memset(data, 0, sizeof(data));	
	type = htonl(type);
	memcpy(&data, &type, sizeof(int));
	pathsize = htonl(pathsize);
	memcpy(&data + sizeof(int), &pathsize, sizeof(int));
	memcpy(&data + sizeof(int)*2, path, pathsize);
	
	/* Sending Data to Server */
	send(sock, data, sizeof(data), 0);

	/* Receiving/Unmarshalling Response */
	int result;
	recv(sock, &result, sizeof(int), 0);
	result = ntohl(result);
	
	/* Returning Value */
	return result;
}

static int my_opendir(const char* path, struct fuse_file_info* fileInfoStruct)
{
	int type = 9;
	int pathsize = strlen(path) + 1;
	
	/* Check that directory exists */
	if (my_opendir(path, null, null, null, null) != 0)
		return -1;
	
	char data[sizeof(int)*2 + pathsize];

	/* Marshalling Data */
	memset(data, 0, sizeof(data));	
	type = htonl(type);
	memcpy(&data, &type, sizeof(int));
	pathsize = htonl(pathsize);
	memcpy(&data + sizeof(int), &pathsize, sizeof(int));
	memcpy(&data + sizeof(int)*2, path, pathsize);
	
	/* Sending Data to Server */
	send(sock, data, sizeof(data), 0);

	/* Receiving/Unmarshalling Response */
	int result;
	recv(sock, &result, sizeof(int), 0);
	result = ntohl(result);
	
	/* Returning Value */
	return result;
}

/* 
 * DS
 * FINISHED
 * NOT TESTED
 */
static int my_truncate(const char* path, off_t offset)
{
	int type = 10;
	int pathsize = strlen(path) + 1;
	
	/* Marshalling Data */
	char data[sizeof(int)*2 + pathsize + sizeof(off_t)];
	memset(data, 0, sizeof(data));
	type = htonl(type);
	memcpy(&data, &type, sizeof(int));
	offset = htonl(offset);
	memcpy(&data + sizeof(int)*2, &offset, sizeof(off_t));
	memcpy(&data + sizeof(int)*2 + sizeof(off_t), path, pathsize);
	pathsize = htonl(pathsize);
	memcpy(&data + sizeof(int), pathsize, sizeof(int));
	
	/* Sending Data to Server */
	send(sock, data, sizeof(data), 0);
	
	/* Receiving/Unmarshalling Response */
	int result;
	recv(sock, &result, sizeof(int), 0);
	result = ntohl(result);
	
	/* Returning Value */
	return result;
}

static int my_close(const char* path)
{
	int type = 11;
	int pathsize = strlen(path) + 1;
	
	/* Marshalling Data */
	char data[sizeof(int)*2 + pathsize)];
	memset(data, 0, sizeof(data));
	type = htonl(type);
	memcpy(&data, &type, sizeof(int));
	memcpy(&data + sizeof(int)*2, path, pathsize);
	pathsize = htonl(pathsize);
	memcpy(&data + sizeof(int), pathsize, sizeof(int));
	
	/* Sending Data to Server */
	send(sock, data, sizeof(data), 0);
	
	/* Receiving/Unmarshalling Response */
	int result;
	recv(sock, &result, sizeof(int), 0);
	result = ntohl(result);
	
	/* Returning Value */
	return result;
}


static struct my_operations : fuse_operations {
	my_operations() {
		getattr		= my_getattr;
		//readdir		= my_readdir;
		open		= my_open;
		//read		= my_read;
		//write		= my_write;
		//create		= my_create;
		mkdir		= my_mkdir;
		//releasedir	= my_releasedir;
		//opendir		= my_opendir;
		truncate	= my_truncate;
		close		=my_close;
	}
} my_oper;


int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		cout << "Usage: clientSNFS <port> <ip address or hostname> <mount directory>\n";
		return 0;
	}

	int port = atoi(argv[1]);
	std::string hostAddr = argv[2];
	std::string directory = argv[3];
	
	if(debug) 
	{
		std::cout << "\nPort number is: " << port << "\n";
		std::cout << "Server Address is: " << hostAddr << "\n";
		std::cout << "Mount Directory is: " << directory << "\n\n";
	}
	
	if (port < 1024 || port > 65536)
	{
		std::cout << "Port number must be between 0 and 65536\n";
		return 0;
	}
        int    sock;
        struct sockaddr_in pin;
        struct hostent *hp;

        if(debug)
                cout << "Attempting to connect to: " << hostAddr << endl;

        /* go find out about the desired host machine */
        if ((hp = gethostbyname(argv[2])) == 0) {
                perror("gethostbyname");
                return 0;
        }

        /* fill in the socket structure with host information */
        memset(&pin, 0, sizeof(pin));
        pin.sin_family = AF_INET;
        pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
        pin.sin_port = htons(port);

        /* grab an Internet domain socket */
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                perror("socket");
                return 0;
        }

        /* connect to PORT on HOST */
        if (connect(sock,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
                perror("connect");
                return 0;
        }

		cout<<"Successfully connected to: " << hostAddr << endl;
             
	char *myargv[] = { argv[0], argv[3], NULL };
	return fuse_main(2, myargv, &my_oper);
}


