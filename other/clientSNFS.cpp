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
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>

//compilation is g++ clientSNFS.cpp -o clientSNFS
//clientSNFS.exe <args>
using namespace std;
int debug = 1;

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		cout << "Usage: clientSNFS <port> <ip address or hostname> <mount directory>\n";
		return 0;
	}
	//std::string portstr = argv[1];
	//int port = atoi(portstr.substr(1, portstr.length()-1).c_str());
	int port = atoi(argv[1]);
	std::string hostAddr = argv[2];
	//std::string hostAddr = hostAddrTemp.substr(1, hostAddrTemp.length()-1).c_str();
	std::string directory = argv[3];
	//std::string directory = directoryTemp.substr(1, directoryTemp.length()-1).c_str();
	std::cout << "\nPort number is: " << port << "\n";
	std::cout << "Server Address is: " << hostAddr << "\n";
	std::cout << "Mount Directory is: " << directory << "\n\n";
	if (port < 1024 || port > 65536)
	{
		std::cout << "Port number must be between 0 and 65536\n";
		return 0;
	}
	
	
        int     sd;
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
        if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                perror("socket");
                return 0;
        }

        /* connect to PORT on HOST */
        if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
                perror("connect");
                return 0;
        }

        //if(debug)
               cout<<"Successfully connected to: " << hostAddr << endl;
	
	
	
	
	
	
	
	
	
	
	
	
	
	close(sd);
	
	return 0;
}
