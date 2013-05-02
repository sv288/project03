void* request_handler(void* socket);
void handle_getattr(int sock);

void my_getattr(const char* path, struct stat* stbuf);
void my_readdir();
void my_open();
void my_read();
void my_write();
void my_create();
void my_mkdir();
void my_releasedir();
void my_opendir();
void my_truncate();
