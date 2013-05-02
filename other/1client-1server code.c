#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_tun.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h> 

#define numBytes 1000

/*	The strategy to construct the VLAN will be for your team to create a proxy program that runs on each machine. 
 *	The proxy will use 2 Ethernet devices to implement the VLAN. One  ethernet device, called eth0, will be 
 *	connected to the Internet. The second device is a virtual device called a tap (for network tap). The tap
 *	operates on layer 2 packets and allows your proxy to send and receive packets on the local machine. 
 */

/* Version Changes
 * 
 * 0.1
 * - changed name to cs352proxy
 * - added arg checking for number of args in main
 * 
 * 0.11
 * - added version notes
 * - added checker for client in main
 * - moved code from main into createTCP
 * - created empty methods: createTap, incomingPackets
 * 
 * 0.12
 * - changed some stuff in client TCP
 * 
 * 0.13
 * - implemented server stuff in main
 * 
 * 0.14
 * - moved server stuff to new method serverTCP
 * 
 * 0.15
 * - implemented tap device intitiation (maybe handling?)
 * 
 * 0.16
 * - created threads to handle tap device and read incoming TCP packet data
 * - not fully functional
 * 
 * 0.17
 * - serverTCP returns sd_current instead of sd
 * - changed pthreads to single pthreads instead of arrays of 1
 * - added pthread_join in main so program doesn't finish
 * 
 * 0.18
 * - pulled allocate tunnel CALL out of handle socket and into main
 * - made tap_fd global
 * - segfaults for client somewhere in method: readline
 * - loops for server printing out EOF
 * 
 * 0.2
 * - skipped .19 just cuz... NEW START!
 * - moved readline to bottom, commented out
 * - replaced readline call in method: handlesocket
 * - handleSocket and handleTap now read from their sockets and print their data
 * - can send and recieve packets both ways
 * 
 * 
 * 0.5
 * - a leap of faith
 * - started encapsulation
 * - buffer sizes changed to 3000
 * - got rid of localInterface parameter from the functions where it was not needed.
 * - haven't checked if it works...
 * 
 * 0.6
 * - Final version!
 * - got rid of errors
 * - made buffers void*
 * - when one host ends the program other goes into infinte loop
 * - got rid of extra comments and stuff
 * */


/* Global Variables */
int		TCPsock; //file descriptor of the client/server socket
int		tap_fd; //file descriptor of tap device


/* allocate_tunnel:
 * open a tun or tap device and returns the file
 * descriptor to read/write back to the caller
 */
int allocate_tunnel(char *dev, int flags) {
	int fd, error;
	struct ifreq ifr;
	char *device_name = "/dev/net/tun";
	if( (fd = open(device_name , O_RDWR)) < 0 ) {
		perror("error opening /dev/net/tun");
		return fd;
	}
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = flags;
	if (*dev) {
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	}
	if( (error = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
		perror("ioctl on tap failed");
		close(fd);
		return error;
	}
	strcpy(dev, ifr.ifr_name);
	return fd;
}

/* Creates a socket, binds it to the given port
 * and waits for a client to connect.
 * On success, return socket's file descriptor 
 * */
int serverTCP(int port)
{
	// implement server
	int sd, sd_current;
	unsigned int addrlen;
	struct sockaddr_in sin;
	struct sockaddr_in pin;

	//get socket
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("not able to open a socket");
		exit(1);
	}

	//socket structure
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = port;

	//bind socket to the given port
	if(bind(sd, (struct sockaddr*) &sin, sizeof(sin)) == -1){
		perror("not able to bind");
		exit(1);
	}

	if(listen(sd,5) == -1){ //not able to listen
		perror("not able to listen");
		exit(1);
	}

	//wait for clients to connect
	addrlen = sizeof(pin);
	if((sd_current = accept(sd, (struct sockaddr*) &pin, &addrlen)) == -1){
		perror("not able to accept");
		exit(1);
	}

	//print out client name and port
	printf("client ip: %s, port: %d\n", inet_ntoa(pin.sin_addr), ntohs(pin.sin_port));

	return sd_current;
}

/* Creates a socket assigned to the AF_INET protocol
 * Given the remote host address, connect to the VM with the socket
 * On success, return socket's file descriptor
 * */
int clientTCP(int port, char* host)
{
	char hostname[100];
	int	sd;
	struct sockaddr_in pin;
	struct hostent *hp;

	strcpy(hostname,host);

	/* go find out about the desired host machine */
	if ((hp = gethostbyname(hostname)) == 0) {
		perror("gethostbyname");
		exit(1);
	}

	/* fill in the socket structure with host information */
	memset(&pin, 0, sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	pin.sin_port = port;

	/* grab an Internet domain socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	/* connect to PORT on HOST */
	if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
		perror("connect");
		exit(1);
	}

	return sd;
}


/* the thread handles incoming packets on the TCP socket and sends to tap device */
void* handleSocket()
{
	void* buffer[3000]; //packet's data is stored in this buffer
	int read_bytes = 0;	//to store size of data 
	
	while(1){ 	//loop forever to read data from the socket
		unsigned short type;	//to store type of the data from socket
		unsigned short length;	//to store length of the data
		
		read(TCPsock, &type, 2);	//get type from the incoming data packet
		type = htons(type);	//convert back to normal value
		
		read(TCPsock, &length, 2);	//get length of the incoming data packet
		length = htons(length);		//convert back to normal value
		
		read_bytes = read(TCPsock, buffer, length);	//read the data in buffer and size in read_bytes
				
		if(type == 0xABCD && length == read_bytes){//if type and length matches, send data to tap device
			if(read_bytes > 0){//check if there's any data to send
			
				
				printf("type: %x\n", type);		//debugging printf
				printf("recieved: %d\n", buffer);		//debugging printf
				
					write(tap_fd, buffer, read_bytes); //send decapsulated data to tap device
				}
			}
			else 	//wrong packet
				printf("type mismatched! \n"); 
	}

	
	close(TCPsock);
	return NULL;
}

/* the thread opens and handles virtual tap device, reads data from tap and sends to socket*/
void* handleTap()
{
	void* buffer[3000]; //packet's data is stored in this buffer
	int read_bytes = 0; //store size of data received
	
	while(1){// loop forever to read data from tap device
		read_bytes = read(tap_fd, buffer, sizeof(buffer)); //get data from tap device
		
		if(read_bytes > 0){ //send data if any received
			printf("sent: %d\n", buffer);		//debugging printf
			
			//"encapsulate" the packet
			unsigned short type = ntohs(0xABCD); //type of header
			unsigned short length = ntohs(read_bytes); //length of data
			
			write(TCPsock, &type, 2); //send type header
			write(TCPsock, &length, 2);//send length
			write(TCPsock, buffer, read_bytes);//send data
		}
	}
	
	close(tap_fd);
	return NULL;
}


int main(int argc, char *argv[])
{
	// Read in command line arguments
	int 	i;
	int 	port;
	int		client;
	char*	host;
	char* 	localInterface;

	//threads to handle socket and tap
	pthread_t pthreadSock;
	pthread_t pthreadTap;

	for(i = 0; i < argc; i++)
		printf("arg %d: %s\n", i, argv[i]);

	// Arg check
	if(argc > 4 || argc < 3){
		puts("Incorrect number of arguments");
		return 0;
	}

	// Find out if client or host and get appropriate args
	if(argc == 3){ 
		port = atoi(argv[1]);
		localInterface = argv[2];
		client = 0;
		//server
		TCPsock = serverTCP(port);
	}
	else{
		host = argv[1];
		port = atoi(argv[2]);
		localInterface = argv[3];
		client = 1;
		//client
		TCPsock = clientTCP(port, host);
	}

	
	//open tap
	if ( (tap_fd = allocate_tunnel(localInterface, IFF_TAP | IFF_NO_PI)) < 0 ) {
		perror("Opening tap interface failed! \n");
		exit(1);
	}
	

	//connect to tap device and create thread to handle it
	pthread_create(&pthreadTap, NULL, handleTap, NULL);
	
	//create thread using the socket's fd for listening/sending packets
	pthread_create(&pthreadSock, NULL, handleSocket, NULL);

	pthread_join(pthreadSock, NULL);
	pthread_join(pthreadTap, NULL);
	
	return 0;
}



