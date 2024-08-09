
#ifndef _mymalloc
#define _mymalloc
	static char myblock[4096];
	void * mymalloc(size_t size, char * filleName, int fileNum);
	void myfree(void * ptr, char * fileName, int fileNum);
	#define malloc(x) mymalloc(x, __FILE__, __LINE__)
	#define free(x) myfree(x, __FILE__, __LINE__)
#endif
