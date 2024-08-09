

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mymalloc.h"


static char myblock[4096] = {'\0'};
const size_t METASIZE = sizeof(size_t) + 1;

int isMeta(void * ptr){//returns 0 if ptr is not pointing to front of metadata, 1 if it is and its not free, 2 if it is free and meta
	char * cPtr = (char *)ptr;
	size_t * sPtr = (size_t *)ptr;
	if(cPtr[sizeof(size_t)] == '1' && (*sPtr) != 0){ //not free
		return 1;
	}else if(cPtr[sizeof(size_t)] == '2' && (*sPtr) != 0){ //freed meta
		return 2;
	}else{//not meta
		return 0;
	}
}

void * makeMeta(void * ptr, size_t blockSize){ //creates the metadata for a blockSize block at ptr, returns ptr to malloc'd block
	char * metaPtr = (char *)ptr;
	//printf("\tmaking meta at idx %lu of blockSize %lu\n", (metaPtr - myblock), blockSize);
	size_t * sizePtr = (size_t *)ptr;
	*sizePtr = blockSize;
	metaPtr[sizeof(size_t)] = '1';
	return &metaPtr[METASIZE];
}

void * findFree(void * ptr, size_t blockSize){//returns a ptr to the beginning of metadata for freed block, to the beginning of unused space, or returns -1 if no space
	char * charPtr = (char *)ptr;
	if(&myblock[4096] - charPtr < blockSize){
		return (void *)-1;
	}
	if(isMeta(ptr) == 1 ||(isMeta(ptr)  == 2 && *((size_t *)ptr) < blockSize) ){ //if its not free or not big enough...
		size_t size = *((size_t *)ptr);
		return findFree(&charPtr[METASIZE + size], blockSize);
	}else{
		return ptr;
	}
	
}
size_t foldForward(char * ptr, size_t  a, int i){//consolidates free blocks, with 'a' being the acccumulator for the size of the new block, and i is used to count how many iterations have occurred. returns new size
	if(isMeta(ptr) != 2){
		return a;
	}else{
		size_t * sPtr = (size_t *)ptr;
		if(i > 0){
			return foldForward(&ptr[METASIZE + (*sPtr)], a + (*sPtr) + METASIZE, ++i);
		}else{
			return foldForward(&ptr[METASIZE + (*sPtr)], a + (*sPtr), ++i);
		}
	}
}


void * mymalloc(size_t size, char * fileName, int lineNum){//allocates memory from *myblock. returns -1 if it can't and a ptr to the freed memory if it can
	if(size > 4096){
		printf("Memory overflow in %s at line #%d\n", fileName, lineNum);
		return (void *)-1;
	}
	if(myblock[0] =='\0'){
		return makeMeta(myblock, size);
	}
	char * freePtr = (char *)findFree(myblock, size);
	size_t * sizePtr = (size_t *)freePtr;
	if((long int)freePtr == -1){
		//not enough room
		printf("Memory overflow in %s at line #%d\n", fileName, lineNum);
		return (void *)-1;
	}
	if(isMeta(freePtr) == 2){
		if((*sizePtr) - size <= METASIZE){
			freePtr[sizeof(size_t)] = '1';
			return &freePtr[METASIZE];
		}else{
			char * freed = makeMeta(&freePtr[size + METASIZE], (*sizePtr) - (size + METASIZE));
			*(freed - 1) = '2';
			(*sizePtr) = size;
			freePtr[sizeof(size_t)] = '1';
			return &freePtr[METASIZE];
		}
	}else{
		return makeMeta(freePtr, size);
	}
	
}

void myfree(void * ptr, char * fileName, int fileNum){//frees the block in *myblock ptr is pointing to, prints error message if it can't
	if((char *)ptr - myblock < METASIZE){
		return;//error
	}
	char * cPtr = (char *)(ptr - METASIZE);
	size_t * sPtr = (size_t *)cPtr;
	if(isMeta(cPtr) == 1){
		cPtr[sizeof(size_t)] = '2';
		size_t newSize = foldForward(cPtr, 0, 0);
		(*sPtr) = newSize;
		return;
	}else if(isMeta(cPtr) == 2){
		printf("Freed already freed pointer in %s at line #%d\n", fileName, fileNum);
		return; //already freed, or not meta, error
	}else{
		printf("Attempted to free something not malloc'd in %s at line #%d, idx %lu\n", fileName, fileNum, (cPtr - myblock));
		return;
	}
}


