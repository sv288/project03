void* request_handler(void* socket);
void handle_getattr(int sock);

static int my_getattr(const char* path, struct stat* stbuf);
static int my_open(const char* path, struct fuse_file_info* fileInfoStruct);

/*
static struct fuse_operations my_oper = {
	.getattr	= my_getattr,
	.readdir	= my_readdir,
	.open		= my_open,
	.read		= my_read,
	.close		= my_close,
	.write		= my_write,
	.create		= my_create,
	.mkdir		= my_mkdir,
	.releasedir	= my_releasedir,
	.opendir	= my_opendir,
	.truncate	= my_truncate
};
*/
