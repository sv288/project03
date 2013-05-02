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


using namespace std;


int debug = 1;	 //if 1 it'll show output msgs surrounded by if(debug), for troubleshooting
void* handleClient(void* sockfd);
//declare your function here

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
                //create thread for every connection
                 pthread_create(&pthreadClient[counter], NULL, handleClient, (void *)&sd_current);
                // pthread_create(&pthreadClient[counter], NULL, YOURFUNCTIONNAME, (void *)&sd_current); //put function name
                 pthread_detach(pthreadClient[counter]);
                 counter++;
        }
	
		return 0;
}
	//  replace this function with yours
	void* handleClient(void* sockfd){
        void* buffer[3000]; //packet's data is stored in this buffer
        int read_bytes = 0;     //to store size of data
        int* temp =(int*) sockfd;
        int sock = *temp;
        
        if(debug)
                printf("Handling socket: %d\n", sock);
        
        while(1){  //read data on the socket
				
				int packetSize;
			    packetSize = read(sock, &packetSize,sizeof(int));    
			    
			    if(packetSize == 0)
					continue;
					
                packetSize = read(sock, buffer, packetSize);
                 
			
		}
		close(sock);
	}
	
	
	

