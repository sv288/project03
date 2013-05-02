#define FUSE_USE_VERSION 26

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
#include <sys/ioctl.h>
#include <netdb.h>

//compilation is g++ clientSNFS.cpp -o clientSNFS
//clientSNFS.exe <args>
using namespace std;
int debug = 1;


int sendRequestToServer(int port, char* hostaddr, const char* directory, char* buf, int size) {
	int sd, bytes_received; //socket descr, bytes_received
	struct hostent *hp; //host pointer
	struct sockaddr_in pin; //server address
	
	if(debug) cout << "Attempting to connect to: " << hostaddr << endl;
	/* go find out about the desired host machine */
	if ((hp = gethostbyname(hostaddr)) == 0) {
		perror("gethostbyname");
		return -1;
	}
	/* fill in the socket structure with host information */
	pin.sin_family = AF_INET;
	pin.sin_port = htons(port);
	pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	memset(&pin, 0, sizeof(pin));
	/* grab an Internet domain socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return -1;
	}
	/* connect to PORT on HOST */
	if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
		perror("connect");
		return -1;
	}
	cout<<"Successfully connected to: " << hostaddr << endl;
	
	//Only read is implemented here, so just send the path to server
	send(sd, directory, strlen(directory), 0);
	
	//Receive the content of file
	bytes_received = recv(sd, buf, size, 0);
	buf[bytes_received] = '\0';
	
	close(sd);
	
	return 0;
}

static int my_getattr(const char *path, struct stat *stbuf)
{
	return 0;
}

static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	return 0;
}

static int my_open(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int my_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	/* CALL SENDREQUESTTOSERVER */
	return 0;
}

//http://www.masonchang.com/blog/2012/1/27/building-fuse-with-g.html
static struct my_operations : fuse_operations {
	my_operations() {
		.getattr	= my_getattr;
		.readdir	= my_readdir;
		.open	= my_open;
		.read	= my_read;
		
	}
} my_operations_init;

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		cout << "Usage: clientSNFS <port> <ip address or hostname> <mount directory>\n";
		return -1;
	}
	int port = atoi(argv[1]);
	std::string hostaddr = argv[2];
	std::string directory = argv[3];
	
	std::cout << "\nPort number is: " << port << "\n";
	std::cout << "Server Address is: " << hostaddr << "\n";
	std::cout << "Mount Directory is: " << directory << "\n\n";
	
	if (port < 1024 || port > 65536)
	{
		std::cout << "Port number must be between 0 and 65536\n";
		return -1;
	}
	
	return fuse_main(2, &argv[2], &my_operations_init, NULL);
}
