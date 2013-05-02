#CC = gcc

#CFLAG = -Wall `pkg-config fuse --cflags --libs`

target : serverSNFS clientSNFS

clientSNFS:	clientSNFS.cpp
	g++ clientSNFS.cpp -o clientSNFS

serverSNFS:	serverSNFS.cpp
	g++ serverSNFS.cpp -o serverSNFS -pthread

clean:
	rm -f *.o
	rm serverSNFS clientSNFS
