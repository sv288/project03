/*
  FUSE: Filesystem in Userspace

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` hello.c -o hello
  
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>


//send request to server through socket and get feedback
static int sendRequestToServer(const char* path, char* buf, int size){

 		int sock, bytes_recieved;  
        struct hostent *host;
        struct sockaddr_in server_addr;  

        host = gethostbyname("127.0.0.1");

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket error");
            return -1;
        }

        server_addr.sin_family = AF_INET;     
        server_addr.sin_port = htons(5000);   
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server_addr.sin_zero),8); 

        if (connect(sock, (struct sockaddr *)&server_addr,
                    sizeof(struct sockaddr)) == -1) 
        {
            perror("Connect error");
            return -1;
        }
        
        //Only read is implemented here, so just send the path to server
		send(sock,path,strlen(path), 0);
				
		//Receive the content of file
		bytes_recieved=recv(sock,buf,size,0);
        
        buf[bytes_recieved] = '\0';

		close(sock);

		return 0;
}


//Let's assume that only two files are there stored in the server, each with size of 19

static const char *file01_path = "/file01";

static const int file01_len = 19;

static const char *file02_path = "/file02";

static const int file02_len = 19;

static int hello_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));

	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, file01_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = file01_len;
	} else if (strcmp(path, file02_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = file02_len;
	}
	else
		res = -ENOENT;

	return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, file01_path + 1, NULL, 0);
	filler(buf, file02_path + 1, NULL, 0);
	return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
	if (strcmp(path, file01_path) != 0 && strcmp(path, file02_path) != 0 )
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;

	if( ( strcmp(path, file01_path) != 0 ) && ( strcmp(path, file02_path) != 0 ) )
		return -ENOENT;

    char recvBuf[1024] = {0};

	sendRequestToServer(path,recvBuf,1024);

	len = strlen(recvBuf);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, recvBuf + offset, size);
	} else
		size = 0;

	return size;
}

static struct fuse_operations hello_oper = {
	.getattr	= hello_getattr,
	.readdir	= hello_readdir,
	.open		= hello_open,
	.read		= hello_read,
};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hello_oper, NULL);
}
