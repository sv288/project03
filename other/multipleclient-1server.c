
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
#include "linkedlist.h"

#include <netinet/ether.h> 
#include <netinet/ip.h>
#include <netinet/tcp.h> 
#include <netinet/udp.h>
#include <net/if_arp.h>

/*      The strategy to construct the VLAN will be for your team to create a proxy program that runs on each machine. 
 *      The proxy will use 2 Ethernet devices to implement the VLAN. One  ethernet device, called eth0, will be 
 *      connected to the Internet. The second device is a virtual device called a tap (for network tap). The tap
 *      operates on layer 2 packets and allows your proxy to send and receive packets on the local machine. 
 */


/* Version Changes
 * 
 * 1.0
 * - added argcheck function
 * - added parsing function
 * - added global variables
 * 
 * 1.1
 * - deleted "#define numbytes"
 * - changed main so that both server and client paths happen
 * - problem: still stops after creating server, needs to be put into a thread
 * 
 * 1.2
 * - added debug global variable
 * - changed functions parseConfig and argCheck to handle debug variable
 * - added debug messages in pretty much every function
 * - moved everything from main to new function: proxyInit
 * 
 * 1.3
 * - replaced the allocate_tunnel function with the new one
 * - new functions serverPath and clientPath
 * - modified proxyInit to make threads for serverPath and clientPath
 * 
 * 1.4
 * - allocate_tunnel WORKS and returns mac address
 * - modified serverpath and clientpath to create threads
 * - client can CONNECT to server
 * - added header file linkedlist.h
 * 
 * 1.5
 * - handlesocket now creates thread that goes to linkstate function
 * - new thread in proxyInit, runs function timeout

 * 
 * 1.6
 * - changed some of the function descriptions
 * - changed exit(1) to return 0 in threaded function in clientTCP
 * - able to connect AND send AND recieve data properly
 * 
 * 1.7
 * - added timeOut function
 * - implemented the linklist
 * - added mutexes for the linked list
 * 
 * 1.8
 * - fixed timeOut function slightly
 * - changed debug messages
 * - added floodLeave and floodQuit functions but not implemented
 * 
 * 1.81
 * - changed some more debug things
 * - fixed link state reading from socket (used to loop through neighbors wrong)
 * 
 * 1.9
 * - added parsing tap name from config.txt
 * - changed handletap() to read arp packets and ip packets from tap,
 * - gonna implement broadcast if it's arp, and write it to member socket if it's ip packet
 * - if memebr is not in list then broadcast it to all memebers(or the closest one found by doing Dijkstra's
 * 
 * 2.0
 * - added broadcast() function, handletap takes care of ARP packets and data packets and writes to the sockfds.
 * - did some other stuff don't remember right now.
 * - LINK STATE FUNCTION WORKSSSSS
 * 
 * 2.1
 * - itty bitty changes, nothing big
 * 
 * 2.2
 * - pinging works between most of the directly connected proxies.
 * - added mypacket function to check if the packet was meant for the proxy or needs to forward/broadcast to others.uploading 2.1 
 * 
 * 2.3
 * - can ping any direcly connected proxies 
 * 
 * 2.4 
 * - can ping any direcly connected proxies most of the time, pinging work between indirectly-
 * - connected proxies for the most part, then whole network breaks.
 * 
 * 2.5
 * - can ping between all proxies : 1<->2<->3<->4, when 1 pings 3, it works without any problem, but when 1 pings 4 
 * - we receive the ping back, but with multiple duplicates too. :/
 * */



#define MAX_DEV_LINE 256 

/* Global Variables */
int             debug = 0; // debug flag
int             tap_fd; //file descriptor of tap device
int             listenPort;
int             linkPeriod;
int             linkTimeout;
int             quitAfter;
char    tap[10];
char    peerAddr[5][50];
int             peerPorts[5];
char    macAddr[18];
char    IPAddr[16];
struct  timeval currentTime;
int qsize = 20;
double packetid[20];	//keep track of last 20 arp packets
int packcounter = 0;

connectedPeer memberListHead = NULL;
pthread_mutex_t MUTEX; //mutex to lock the memberlist of each proxy

/* allocate_tunnel:
 * open a tun or tap device and returns the file
 * descriptor to read/write back to the caller
 * also stores MAC address in local_mac variable
 */
int allocate_tunnel(char *dev, int flags, char* local_mac){
        if(debug)
                puts("Allocating Tunnel...");

        int fd, error;
        struct ifreq ifr;
        char *device_name = "/dev/net/tun";
        char buffer[MAX_DEV_LINE];

        if( (fd = open(device_name , O_RDWR)) < 0 ){
                fprintf(stderr,"error opening /dev/net/tun\n%d:%s\n",errno,strerror(errno));
                return fd;
        }
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = flags;
        if (*dev) {
                strncpy(ifr.ifr_name, dev, IFNAMSIZ);
        }
        if( (error = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ){
                fprintf(stderr,"ioctl on tap failed\n%d: %s\n",errno,strerror(errno));
                close(fd);
                return error;
        }
        strcpy(dev, ifr.ifr_name);
        if(debug)
                printf("dev = %s, ifr_name = %s\n", dev, ifr.ifr_name);

        // Get device MAC address //
        sprintf(buffer,"/sys/class/net/%s/address",dev);
        FILE* f = fopen(buffer,"r");
        fread(buffer,1,MAX_DEV_LINE,f);

        //replaced sscanf with strncpy because sscanf returns garbage
        //sscanf(buffer,"%hhX:%hhX:%hhX:%hhX:%hhX:%hhX",local_mac,local_mac+1,local_mac+2,local_mac+3,local_mac+4,local_mac+5);

        strncpy(local_mac, buffer, 17);
        local_mac[17] = '\0';
        fclose(f);
        if(debug){
                puts("===DONE ALLOCATING TUN===");
			}

        return fd;
}


/* Creates a socket assigned to the AF_INET protocol
 * Given the remote host address, connect to the VM with the socket
 * On success, return socket's file descriptor
 * On failure to connect, return 0
 * */
int clientTCP(int port, char* host){
        char hostname[100];
        int     sd;
        struct sockaddr_in pin;
        struct hostent *hp;

        if(debug)
                printf("Attempting to connect to %s...\n", host);

        strcpy(hostname,host);

        /* go find out about the desired host machine */
        if ((hp = gethostbyname(hostname)) == 0) {
                perror("gethostbyname");
                return 0;
        }

        /* fill in the socket structure with host information */
        memset(&pin, 0, sizeof(pin));
        pin.sin_family = AF_INET;
        pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
        pin.sin_port = port;

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
                printf("Successfully connected to %s!\n", host);

        return sd;
}

/* threaded function that will periodically send linkstate packets to a given socket
 * sends it every linkPeriod seconds
 * */
void* linkstate(void* socket){
        int* temp = socket;
        int socketfd = *temp;

        while(1){
                
                unsigned short type = htons(0xABAC);
                unsigned short length = htons(12 + sizeofList());
                unsigned short numMembers = numPeers();

                //send link state packet
                write(socketfd, &type, 2); //send type header
                write(socketfd, &length, 2);//send length
                write(socketfd, &numMembers, 2); //send number of members

                //first send this proxy's info
                write(socketfd, IPAddr, sizeof(IPAddr));
                write(socketfd, &listenPort, sizeof(listenPort));
                write(socketfd, macAddr, sizeof(macAddr));
                if(debug){
                        puts("SENDING");
                        printf("numneig: %d\n", (int)(sizeofList()/(sizeof(connectedPeerList))));
                        printf("ip: %s\n", IPAddr);
                        printf("PORT: %d\n", listenPort);
                        puts("END SEND");
                }
                pthread_mutex_lock(&MUTEX);
                //now send member's info
                connectedPeer ptr = memberListHead;
                while(ptr != NULL)
                {
                        write(socketfd, ptr->localMAC, sizeof(ptr->localMAC));
                        write(socketfd, ptr->remoteMAC, sizeof(ptr->remoteMAC));
                        write(socketfd, ptr->localIP, sizeof(ptr->localIP));
                        write(socketfd, ptr->remoteIP, sizeof(ptr->remoteIP));
                        write(socketfd, &ptr->localPort, sizeof(ptr->localPort));
                        write(socketfd, &ptr->remotePort, sizeof(ptr->remotePort));
                        write(socketfd, &ptr->avgRTT, sizeof(ptr->avgRTT));
                        write(socketfd, &ptr->uniqueID, sizeof(ptr->uniqueID));
                        ptr = ptr->next;
                }
                pthread_mutex_unlock(&MUTEX);
                //sleep for a bit until sending next link state packet
                if(debug)
                        printf("sending another linkstate in: %d\n", linkPeriod);
                sleep(linkPeriod);
        }
}

//flood quit message to all connected peers and ends program
void* floodQuit(){
		pthread_mutex_lock(&MUTEX);
        connectedPeer ptr = memberListHead;
        short type = htons(0xAB12);
        short length = htons(20);
        double id = 0; //id of the proxy doesn't matter for quit
        while(ptr != NULL){ 
                write(ptr->peerSocket, &type, 2);
                write(ptr->peerSocket, &length, 2);
                write(ptr->peerSocket, IPAddr, sizeof(IPAddr));
                write(ptr->peerSocket, &listenPort, sizeof(listenPort));
                write(ptr->peerSocket, macAddr, sizeof(macAddr));
                write(ptr->peerSocket, &id, 2);
                ptr = ptr->next;
        }
        printf("QUITING.........\n");
        pthread_mutex_unlock(&MUTEX);
        exit(1);// kill all threads and finish the program.
}

//flood leave message to all connected peers
void floodLeave(char ip[], int port, char mac[], double id){
		pthread_mutex_lock(&MUTEX);
        connectedPeer ptr = memberListHead;
        short type = htons(0xAB01);
        short length = htons(20);
        while(ptr != NULL && !strcmp(ptr->remoteMAC,mac)){ 
                write(ptr->peerSocket, &type, 2);
                write(ptr->peerSocket, &length, 2);
                write(ptr->peerSocket, ip, sizeof(ip));
                write(ptr->peerSocket, &listenPort, sizeof(port));
                write(ptr->peerSocket, IPAddr, sizeof(IPAddr));
                write(ptr->peerSocket, &id, 2);
                ptr = ptr->next;
        }
        pthread_mutex_unlock(&MUTEX);
}

void getmacs(void* buffer, char*  sorc, char* dest){
		char sorcMac[18] = "";
        char destMac[18] = "";
        struct ether_header *ether_packet_p;  /* Pointer to an Ethernet header */
		ether_packet_p = (struct ether_header *) buffer; 

		//store source mac address
		sprintf(sorcMac, "%02x:%02x:%02x:%02x:%02x:%02x", ether_packet_p->ether_shost[0], ether_packet_p->ether_shost[1], 
						ether_packet_p->ether_shost[2], ether_packet_p->ether_shost[3], 
						ether_packet_p->ether_shost[4], ether_packet_p->ether_shost[5]);
		//store destination mac address
		sprintf(destMac, "%02x:%02x:%02x:%02x:%02x:%02x",          ether_packet_p->ether_dhost[0], ether_packet_p->ether_dhost[1], 
						ether_packet_p->ether_dhost[2], ether_packet_p->ether_dhost[3], 
						ether_packet_p->ether_dhost[4], ether_packet_p->ether_dhost[5]);
		strcpy(sorc,sorcMac);
		strcpy(dest,destMac);
		//if(debug)
			//printf("copied source mac: %s | dest mac: %s\n", sorc, dest);
}

//check if the packet dest is me, return 1 if its, -1 if it was sent by me, 0 otherwise
int mypacket(void* buffer){
		char sorcMac[18] = "";
        char destMac[18] = "";
        char ffMac [18] = "ff:ff:ff:ff:ff:ff";
		getmacs(buffer,sorcMac,destMac);
				if(!strcmp(macAddr,destMac)){
					return 1;
				}
				else if(!strcmp(macAddr,sorcMac)){
					return -1;
				}
				else if(!strcmp(destMac,ffMac)){
					return 2;
				}
				else{
					return 0;
				}
	return 0;
}



/* broadcasts ARP messages to all neighbors 
 */
void broadcast(void* buffer, short rbytes, double packid){
	if(debug)
        printf("broadcasting!!\n");
        pthread_mutex_lock(&MUTEX);
        connectedPeer temp = memberListHead;
        //send to all members in list, eventually get the correct destination
        while(temp != NULL){
                int sockfd =  temp->peerSocket;
                //printf("temp sock:%d\n",sockfd);           
                if(sockfd <= 0){
					if(debug)
                        printf("not able to get valid sockfd, for: %s\n",temp->remoteMAC);
                        temp = temp->next;
                        continue;
                }
                //"encapsulate" the packet
                unsigned short type = htons(0xAAAB); //type of header
                unsigned short length = htons(rbytes); //length of data
                
                write(sockfd, &type, 2); 		//send type header
                write(sockfd,&packid, sizeof(double));	//send packet id
                write(sockfd, &length, 2);		//send length
                write(sockfd, buffer, rbytes);		//send data
                if(debug)
					printf("sending arp to socket:%d\n",sockfd);
                temp = temp->next;
        }
        pthread_mutex_unlock(&MUTEX);
}



/* the thread handles INCOMING packets on the TCP socket
 * creates a linkstate thread as soon as connection is established
 * loops forever listening to the socket
 * */
void* handleSocket(void* sockfd){
        void* buffer[3000]; //packet's data is stored in this buffer
        int read_bytes = 0;     //to store size of data
        int* temp = sockfd;
        int TCPsock = *temp;
        pthread_t linkstate_t;

        if(debug)
                printf("Handling socket: %d\n", TCPsock);

        //send this proxy's info to newly connected peer
        //create thread to handle linkstate packets
        pthread_create(&linkstate_t, NULL, linkstate, sockfd);
        
        
        while(1){       //loop forever to read data from the socket
                unsigned short type;    //to store type of the data from socket
                unsigned short length;  //to store length of the data
                char sndrip[16] ="";
                int sndrport;
                char sndrmac[18] = "";
                double sndrid;

                read_bytes = read(TCPsock, &type, 2);        //get type from the incoming data packet
                type = ntohs(type);     //convert back to normal value

                if(read_bytes <= 0 || type <0xAAAA)
                        continue;

                //read(TCPsock, &length, 2);      //get length of the incoming data packet
                //length = ntohs(length);         //convert back to normal value
                
                if(type == 0xABCD){//Data packet: if type and length matches, send data to tap device
                if(debug)
						printf("packet type: data\n");

						read(TCPsock, &length, 2);      //get length of the incoming data packet
						length = ntohs(length);         //convert back to normal value
						read_bytes = read(TCPsock, buffer, length);     //read the data in buffer and size in read_bytes
                        if(read_bytes > 0){//check if there's any data to send

                                write(tap_fd, buffer, read_bytes); //send decapsulated data to tap device
                        }
                }
                else if(type == 0xAAAB){
					if(debug)
                        printf("packet type: ARP\n");
                        double packid;
						read(TCPsock, &packid, sizeof(double));	//read packet id
                        read(TCPsock, &length, 2);      //get length of the incoming data packet
						length = ntohs(length);         //convert back to normal value
                        read_bytes = read(TCPsock, buffer, length);
                        int i, flag =0;
                        pthread_mutex_lock(&MUTEX);
                        for(i = 0; i < qsize; i++){
							if(packetid[i] == packid)	//check if we already have this packet
								flag = 1;
								break;
						}
						pthread_mutex_unlock(&MUTEX);
						if(flag){		//do not broadcast the packet
							continue;
						}
						pthread_mutex_lock(&MUTEX);
                        packetid[packcounter] = packid;		//add to list of received packets if it's a new packet
                        if(packcounter == qsize - 1){
							packcounter = 0;
						}
						else{
							packcounter++;
						}
						pthread_mutex_unlock(&MUTEX);
                        int check = mypacket(buffer);
                        if(check == 1){		//for me, write to tap
							write(tap_fd, buffer, length);
						}
						else if(check == 2){	//ARP packet, write to tap and broadcast
							broadcast(buffer,length,packid);
							write(tap_fd, buffer, length);						
						}
						else if(check == 0){	//not for me, not sent by me, forward to members
							broadcast(buffer,length,packid);
						}
						else{
							if(debug)
								printf("sent by me, ignore\n");
						}					
                }
                else if(type == 0xAB01 && length == 20){//Leave packet
                if(debug)
                        printf("packet type: leave\n");
                        read(TCPsock, &length, 2);      //get length of the incoming data packet
						length = ntohs(length);         //convert back to normal value

                        read(TCPsock, sndrip, sizeof(sndrip));          //read sender's ip address

                        read(TCPsock, &sndrport, sizeof(sndrport));      //get sender's port number
                        sndrport = ntohs(sndrport); 

                        read(TCPsock, sndrmac, sizeof(sndrmac));                //read sender's mac address

                        read(TCPsock, &sndrid, sizeof(sndrid));         //read sender's id

                        floodLeave(sndrip,sndrport,sndrmac,sndrid);// let other proxies know about leave
                }
                else if(type == 0xAB12 && length == 20){//Quit packet
                if(debug)
                        printf("packet type: quit\n");
                        read(TCPsock, &length, 2);      //get length of the incoming data packet
						length = ntohs(length);         //convert back to normal value

                        read(TCPsock, sndrip, sizeof(sndrip));          //read sender's ip address

                        read(TCPsock, &sndrport, sizeof(sndrport));      //get sender's port number
                        sndrport = ntohs(sndrport); 

                        read(TCPsock, sndrmac, sizeof(sndrmac));                //read sender's mac address

                        read(TCPsock, &sndrid, sizeof(sndrid));         //read sender's id

                        if(debug){
                                printf("type: %x\n", type);             //debugging printf
                                printf("sender ip: %s\n", sndrip);       //debugging printf
                                printf("sender port: %hu\n", sndrport);
                                printf("sender mac: %s\n", sndrmac);
                                printf("sender id: %g\n", sndrid);
                        }
                        floodQuit();//let other proxies know to quit
                        exit(0);
                }
                else if(type == 0xABAC){//Linkstate
                if(debug)
                        printf("packet type: link state\n");
                        read(TCPsock, &length, 2);      //get length of the incoming data packet
						length = ntohs(length);         //convert back to normal value
                        unsigned short numneigbrs = 0;
                        connectedPeer result = NULL;

                        read(TCPsock, &numneigbrs, 2);  //read # of neighbors
                        read(TCPsock, sndrip, sizeof(sndrip));          //read sender's ip address
                        read(TCPsock, &sndrport, sizeof(sndrport));      //get sender's port number
                        sndrport = ntohs(sndrport); 
                        read(TCPsock, sndrmac, sizeof(sndrmac));                //read sender's mac address

                        int time = gettimeofday(&currentTime, NULL);
                        while(time < 0)
                                time = gettimeofday(&currentTime, NULL);

                        double setTime = currentTime.tv_sec;

                        //make new node and insert
                        connectedPeer addingNode = createNode(macAddr, sndrmac, TCPsock, IPAddr, sndrip, listenPort, sndrport, 1, setTime);
                        pthread_mutex_lock(&MUTEX);
                        if((result = insertPeer(addingNode)) == NULL)//already in memberlist
                                free(addingNode);
                        else
                                memberListHead = result;
                        pthread_mutex_unlock(&MUTEX);
                        /* read every member from their list
                         * insert into our list (takes care of updating and checking previous existance)
                         */

                        char rmtip[16] ="";
                        int rmtport;
                        char rmtmac[18] = "";
                        int artt =0;
                        double uid = 0;
                        while(numneigbrs > 0){
                                read(TCPsock, sndrmac, sizeof(sndrmac));                //read sender's mac address
                                read(TCPsock, rmtmac, sizeof(rmtmac));          //read sender's mac address
                                read(TCPsock, sndrip, sizeof(sndrip));          //read sender's ip address
                                read(TCPsock, rmtip, sizeof(rmtip));            //read sender's ip address
                                read(TCPsock, &sndrport, sizeof(sndrport));      //get sender's port number
                                read(TCPsock, &rmtport, sizeof(rmtport));      //get sender's port number
                                read(TCPsock, &artt, sizeof(artt));     
                                read(TCPsock, &uid, sizeof(uid));
                                
                                if(debug)
                                       printf("NEIGHBOR MAC: %s \n", rmtmac);
                                //search if the current member is on our member list
                                connectedPeer addingNode = createNode(macAddr, rmtmac, 0, IPAddr, rmtip, listenPort, rmtport, 1, uid);
                                if(strcmp(rmtmac, macAddr) == 0 || strlen(rmtmac) < 15){//we're their neighbor
                                        
									if(debug)
										printf("ignore the member\n");
									free(addingNode);
                                }
                                else{//since it's not there, we make a new thread to connect and pass socketfd, in order to add to our list
                                        if(debug)
                                         printf("not us, trying to add NEIGHBOR if not in list: %s\n", rmtmac);
                                        pthread_mutex_lock(&MUTEX);
                                        connectedPeer result;
                                        if((result = insertPeer(addingNode)) == NULL)//already in memberlist
                                                free(addingNode);
                                        else
                                                memberListHead = result;
                                        
                                        pthread_mutex_unlock(&MUTEX);
                                }
                                numneigbrs--;
                        }
                }
                else if(type == 0xAB34 && length == 8){
					if(debug)
                        printf("packet type: rtt probe request\n");
                        read(TCPsock, &length, 2);      //get length of the incoming data packet
						length = ntohs(length);         //convert back to normal value

                        read(TCPsock, &sndrid, sizeof(sndrid));                 //read sender's ip address

                        if(debug){
                                printf("type: %x\n", type);             //debugging printf
                                printf("sender id: %g\n",sndrid);        //debugging printf
                        }
                }
                else if(type == 0xAB35 && length == 8){
					if(debug)
                        printf("packet type: rtt probe response\n");
                        read(TCPsock, &length, 2);      //get length of the incoming data packet
						length = ntohs(length);         //convert back to normal value

                        read(TCPsock, &sndrid, sizeof(sndrid));                 //read sender's id

                        if(debug){
                                printf("type: %x\n", type);             //debugging printf
                                printf("sender id: %g\n",sndrid);        //debugging printf
                        }
                }
                else if(type == 0xAB21){
					if(debug)
                        printf("packet type: proxy public key\n");
                }
                else if(type == 0xABC1){
					if(debug)
                        printf("packet type: signed data\n");
                }
                else if(type == 0xAB22){
					if(debug)
                        printf("packet type: proxy secret key\n");
                }
                else if(type == 0xABC2){
					if(debug)
                        printf("packet type: encrypted data\n");
                }
                else if(type == 0xABAB){
					if(debug)
                        printf("packet type: encrypted link state\n");
                }
                else if(type == 0xABAD){
					if(debug)
                        printf("packet type: signed link state\n");
                }
                else if(type == 0xAB45){
					if(debug)
                        printf("packet type: bandwidth probe request\n");
                }
                else if(type == 0xAB46){
					if(debug)
                        printf("packet type: bandwidth response\n");
                }                               
                else{
					if(debug)
                        printf("unknown type! \n");
                }
        }
        
        free(sockfd);
        close(TCPsock);
        return NULL;
}


/* thread that opens and handles virtual tap device and reads data
 * */
void* handleTap(){
        void* buffer[3000]; //packet's data is stored in this buffer
        int read_bytes = 0; //store size of data received

        struct ether_header *ether_packet_p;  /* Pointer to an Ethernet header */
        struct iphdr *ip_packet_p;            /* Pointer to an IP header  */
        unsigned int ip_source_addr;  /* An IP source address */
        unsigned int ip_dest_addr;    /* An IP destination address */
        char sorcMac[18] = "";
        char destMac[18] = "";
        char sorcIp[16] = "";
        char destIp[16] = "";

       // if(debug)
               // puts("Reading from tap in loop");

        while(1){// loop forever to read data from tap device
                
                read_bytes = read(tap_fd, buffer, sizeof(buffer)); //get data from tap device
                if(debug)
                        puts("reading a packet from tap");
                
                if(read_bytes == 0){
                        continue;
                }
                int time = gettimeofday(&currentTime, NULL);		//cretae a unique packet id
                        while(time < 0)
                                time = gettimeofday(&currentTime, NULL);
                        double packid = currentTime.tv_sec;
                        
                pthread_mutex_lock(&MUTEX);
				packetid[packcounter] = packid;		//add packet id to list so ignore if received back
				       if(packcounter == qsize - 1){
							packcounter = 0;
						}
						else{
							packcounter++;
						}
				pthread_mutex_unlock(&MUTEX);

                ether_packet_p = (struct ether_header *) buffer; 

                //store source mac address
                sprintf(sorcMac, "%02x:%02x:%02x:%02x:%02x:%02x", ether_packet_p->ether_shost[0], ether_packet_p->ether_shost[1], 
                                ether_packet_p->ether_shost[2], ether_packet_p->ether_shost[3], 
                                ether_packet_p->ether_shost[4], ether_packet_p->ether_shost[5]);
                //store destination mac address
                sprintf(destMac, "%02x:%02x:%02x:%02x:%02x:%02x",          ether_packet_p->ether_dhost[0], ether_packet_p->ether_dhost[1], 
                                ether_packet_p->ether_dhost[2], ether_packet_p->ether_dhost[3], 
                                ether_packet_p->ether_dhost[4], ether_packet_p->ether_dhost[5]);

                if(debug){
                        printf("from handle tap, sourceMac: %s | DestMac: %s\n",sorcMac,destMac); 
                }

                switch (ntohs(ether_packet_p->ether_type)) { 

                case ETHERTYPE_ARP:  /* we have an ARP packet. */
						printf("received a packet form tap.\n");
						int sockfd1 =  0;
                        connectedPeer temp1 = memberListHead;
                        
                        int flag1 = 0;
                        //check if the member is in the list, and if it's send it to the member only
						pthread_mutex_lock(&MUTEX);
                        while(temp1 != NULL){
                                if(!strcmp(temp1->remoteMAC,destMac)){
                                        flag1 = 1;
                                        sockfd1 = temp1->peerSocket;
                                        break;
                                }
                                temp1 = temp1->next;
                        }
                        pthread_mutex_unlock(&MUTEX);
                        if(flag1 && sockfd1 > 0){ //destination address is in member list
                                unsigned short type = htons(0xABCD); //type of header
                                unsigned short length = htons(read_bytes); //length of data
                                write(sockfd1, &type, 2); //send type header
                                write(sockfd1, &length, 2);//send length
                                write(sockfd1, buffer, read_bytes);//send data
                                if(debug){
                                        printf("member in list, sending to: %s\n",temp1->remoteMAC);     //debugging printf
                                }
                                break;          //break the switch
                        }
                        broadcast(buffer,read_bytes,packid);	//broadcast to everyone
                        if(debug)
                                printf("sent ARP packet from handle tap\n");                                                                 
                        /* add printing out ARP packet 
                         * See net/if_arp.h for the header definition */
                        break;

                case ETHERTYPE_IP:  /* we have an IP packet */
                        /* set the pointer of the IP packet header to the memory location 
                         * of the buffer base address + the size of the Ethernet header 
                         * should check if the length from cread is sane here */ 
                        ip_packet_p = (struct iphdr *) &(buffer[sizeof(struct ether_header)] );
                        /* get the source and destination IP address */
                        /* remember to change the byte order of the IP addresses to the 
                         * local host's byte order */ 
                        ip_source_addr = (unsigned int) ntohl( ip_packet_p->saddr);
                        ip_dest_addr = (unsigned int) ntohl( ip_packet_p->daddr);

                        // store source ip in char array
                        sprintf(sorcIp, "%d.%d.%d.%d",  (ip_dest_addr & 0xFF000000)>>24, 
                                        (ip_dest_addr & 0x00FF0000)>>16, 
                                        (ip_dest_addr & 0x0000FF00)>>8,
                                        (ip_dest_addr & 0x000000FF));

                        //store dest ip in char array
                        sprintf(destIp, "%d.%d.%d.%d",  (ip_source_addr & 0xFF000000)>>24, 
                                        (ip_source_addr & 0x00FF0000)>>16, 
                                        (ip_source_addr & 0x0000FF00)>>8,
                                        (ip_source_addr & 0x000000FF));
                        if(debug)
                                printf("print from array, source ip:%s | dest ip: %s\n", sorcIp, destIp); 
                        
                        connectedPeer temp = memberListHead;
                        int sockfd =  0;
                        int flag = 0;
                        //send to all members in list, eventually get the correct destination
						pthread_mutex_lock(&MUTEX);
                        while(temp != NULL){
                                if(!strcmp(temp->remoteIP,destIp) || !strcmp(temp->remoteMAC,destMac)){
                                        flag = 1;
                                        sockfd = temp->peerSocket;
                                        break;
                                }
                                temp = temp->next;
                        }
                        pthread_mutex_unlock(&MUTEX);
                        if(flag && sockfd > 0){ //destination address is in member list
                                unsigned short type = htons(0xABCD); //type of header
                                unsigned short length = htons(read_bytes); //length of data
                                write(sockfd, &type, 2); //send type header
                                write(sockfd, &length, 2);//send length
                                write(sockfd, buffer, read_bytes);//send data
                                if(debug){
                                        printf("member in list, sending to: %s\n",temp->remoteMAC);     //debugging printf
                                }
                                break;          //break the switch
                        }
                        broadcast(buffer,read_bytes,packid);
                        break;  //break switch
                default:{
					if(debug)
                        printf("got unknown packet type from tap \n");
					}
                }//switch ends here
        }
     return NULL;
}


/* Threaded function that binds socket to port and listens to it for incoming connection
 * when connection is recieved creates a thread and sends it to handleSocket function
 * */
int serverPath(){
        // implement server
        int sd, sd_current;
        unsigned int addrlen;
        struct sockaddr_in sin;
        struct sockaddr_in pin;
        pthread_t pthreadSock;

        if(debug)
                puts("Listening for Connections...");

        //get socket
        if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
                perror("not able to open a socket");
                exit(1);
        }

        //socket structure
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = listenPort;

        //bind socket to the given port
        if(bind(sd, (struct sockaddr*) &sin, sizeof(sin)) == -1){
                perror("not able to bind");
                exit(1);
        }

        while(1){
                if(listen(sd,5) == -1){ //not able to listen
                        perror("not able to listen");
                        return 0;;
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
                pthread_create(&pthreadSock, NULL, handleSocket, (void *)&sd_current);
        }
}

/* attempts to connect to all threads in the list of peers given by the config file
 * creates a thread to handleSocket for every succesful connection
 * */
int clientPath(){
        int* clientSocket;
        pthread_t pthreadSock;
        int i;
        for(i=0; i < 5; i++){
                if(strlen(peerAddr[i]) > 6){
                        clientSocket = (int*)malloc(sizeof(int));
                        *clientSocket = clientTCP(peerPorts[i], peerAddr[i]);//attempt connection
                        if(*clientSocket){//if succesful
                                if(debug){
                                        printf("client sock: %d\n", *clientSocket);
                                }
                                pthread_create(&pthreadSock, NULL, handleSocket, clientSocket);//new thread handles socket
                        }
                }
        }
        return 0;
}


/* Thread cycles through memberlist checking every member's uniqueID
 * if the difference between the uniqueID and current time is greater than the linkTimeout time
 * the node is removed from the member list
 * */
void* timeOut(){
        while(1){
                connectedPeer ptr = memberListHead;//start at the head
                while(ptr != NULL){//every member in list
                        int time = gettimeofday(&currentTime, NULL);
                        while(time < 0)
                                time = gettimeofday(&currentTime, NULL);
                        double setTime = currentTime.tv_sec;
                        if(setTime - ptr->uniqueID >= linkTimeout){//time to die
                                if(debug)
                                        printf("Host %s has taken too long to respond, removing from member list...", ptr->remoteIP);
                                pthread_mutex_lock(&MUTEX);
                                memberListHead = removePeer(ptr->remoteMAC);
                                pthread_mutex_unlock(&MUTEX);
                        }
                        ptr = ptr->next;
                }
        }
}

/* Attempts to open file given by parameters
 * If succeful, reads file line by line, storing the values
 * If unsuccesful, returns error
 * */
int parseConfig(char *argv[]){
        FILE* fp;
        char buff[BUFSIZ];
        char* tok;
        char* delim = " \n";
        int peerCount = 0;

        if(debug)
                puts("Parsing Config File...");

        //open config file
        fp = fopen(argv[1], "r");

        if(fp == NULL){
                perror(NULL);
                exit(1);
        }

        //while not reached EOF, read every line
        while(fgets(buff, BUFSIZ, fp) != NULL){
                if(buff[0] == '/' && buff[1] == '/')//comment
                        continue;
                if(buff[0] == '\n')//newline
                        continue;

                tok = strtok(buff, delim);

                //tokenizes and gets the value from the line, e.g. port number, 
                if(!strcmp(tok, "listenPort")){
                        tok =strtok(NULL, delim);
                        listenPort = atoi(tok);
                        if (debug)
                                printf("lPort = %s\n", tok);
                }
                if(!strcmp(tok, "linkPeriod")){
                        tok =strtok(NULL, delim);
                        linkPeriod = atoi(tok);
                        if (debug)
                                printf("lPeriod = %s\n", tok);
                }
                if(!strcmp(tok, "linkTimeout")){
                        tok =strtok(NULL, delim);
                        linkTimeout = atoi(tok);
                        if (debug)
                                printf("lTime = %s\n", tok);
                }
                if(!strcmp(tok, "peer")){
                        tok =strtok(NULL, delim);

                        strcpy(peerAddr[peerCount], tok);
                        if(debug)
                                printf("peer %d addr = %s, ", peerCount, tok);

                        tok =strtok(NULL, delim);
                        peerPorts[peerCount] = atoi(tok);
                        if(debug)
                                printf("peer port = %s\n", tok);

                        peerCount++;
                }
                if(!strcmp(tok, "quitAfter")){
                        tok =strtok(NULL, delim);
                        quitAfter = atoi(tok);
                        if (debug)
                                printf("qAfter = %s\n", tok);
                }
                
                if(!strcmp(tok, "tapDevice")){
                        tok =strtok(NULL, delim);
                        strcpy(tap,tok);
                        if (debug)
                                printf("tap = %s\n", tok);
                }
                
                /* not implemented yet
                if(!strcmp(tok, "globalSecretKey")
                if(!strcmp(tok, "peerKey")
                 */
        }
        fclose(fp);
        return 0;
}

/* checks number of arguments and for help and debug flags
 * */
void argCheck(int argc, char* argv[]){
        if(argc < 2 || argc > 3){
                puts("Incorrect number of arguments");
                puts("For help use -h");
                exit(1);
        }
        if(argc == 2 && !strcmp(argv[1], "-h")){
                puts("=====");
                puts("Invocation: ./cs352proxy <config file name>");
                puts("For debug use -d");
                puts("=====");
                exit(1);
        }
        else if(argc == 3){
                if(!strcmp(argv[1], "-h") || !strcmp(argv[2], "-h")){
                        puts("=====");
                        puts("Invocation: ./cs352proxy <config file name>");
                        puts("For debug use -d");
                        puts("=====");
                        exit(1);
                }
                if(!strcmp(argv[1], "-d")){
                        strcpy(argv[1], argv[2]);
                        debug = 1;
                }
                else if(!strcmp(argv[2], "-d"))
                        debug = 1;
                else{                   
                        puts("Incorrect number of arguments");
                        puts("For help use -h");
                        exit(1);
                }

        }

}

//print the memberlist every 10 seconds
void* priList(){
	        while(1)
        {
                if(memberListHead != NULL)
                        printList();
                sleep(10);
        }
}


/* Initializes proxy on startup
 * creates the main threads that spawn subsequent threads
 * - Server thread
 * - Client connect thread
 * - Tap thread
 * - Timeout thread
 * */
int proxyInit(int argc, char* argv[]){
        //threads to handle socket and tap
        pthread_t pthreadServer;
        pthread_t pthreadClient;
        pthread_t pthreadTap;
        pthread_t pthreadTimeout;
        pthread_t pthreadQuit;
        pthread_t printList;
        
		printf("PROXY STARTED!\n");
        //check args
        argCheck(argc, argv);

        //parse the config file
        parseConfig(argv);

        //open tap
        if ( (tap_fd = allocate_tunnel(tap, IFF_TAP | IFF_NO_PI, macAddr)) < 0 ) {
                perror("Opening tap interface failed! \n");
                exit(1);
        }
        if(debug)
                printf("Local MAC: %s\n", macAddr);
        if(debug)
                printf("Tap_fd is: %d\n", tap_fd);

        //initiate the mutex lock on the member list
        pthread_mutex_init(&MUTEX, NULL);

        //creates thread that runs the serverPath function
        pthread_create(&pthreadServer, NULL,(void*) serverPath, NULL);

        //connect to tap device and create thread to handle it
        pthread_create(&pthreadTap, NULL, handleTap, NULL);


        //create thread using the socket's fd for listening/sending packets
        pthread_create(&pthreadClient, NULL,(void*) clientPath, NULL);

        //cycle through member list checking all ID's comparing them to current time
        pthread_create(&pthreadTimeout, NULL, timeOut, NULL);
        
        //therad to print memebrlist every 10 seconds
        pthread_create(&printList, NULL, priList, NULL);
        pthread_detach(printList);

        //pthread_join(pthreadClient, NULL);
        //pthread_join(pthreadServer, NULL);
        //pthread_join(pthreadTap, NULL);

        //wait quitAfter seconds to quit the network
        sleep(quitAfter);

     printf("##############creating floodQuit thread#####################\n");
		 pthread_create(&pthreadQuit, NULL, floodQuit, NULL);
        //Wait for thread to finish
        sleep(3);
        close(tap_fd);
        return 0;
}

//main
int main(int argc, char *argv[]){
        proxyInit(argc, argv);
        return 0;
}



