CC = g++

CFLAG = -Wall -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs`

target : serverSNFS clientSNFS

clientSNFS: clientSNFS.cpp
	$(CC) $(CFLAG) -o $@ $<

serverSNFS: serverSNFS.cpp
	$(CC) $(CFLAG) -o $@ $< -pthread

clean:
	rm -f *.o
	rm serverSNFS clientSNFS
