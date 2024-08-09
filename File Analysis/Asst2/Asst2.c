#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>


//begin struct definitions
typedef struct tokenNode{
	char * token;
	int freq;
	float prob;
	struct tokenNode * next;
} tokenNode;

typedef struct fileNode{
	char * name;
	int tokens;
	int words;
	struct tokenNode * tokenHead;
	struct fileNode * next;
} fileNode;

typedef struct args{
	char * name;
	fileNode ** ptr;
	pthread_mutex_t lock;
} args;

typedef struct threadNode{
	pthread_t id;
	struct threadNode * next;
} threadNode;

typedef struct outNode{
	float score;
	char * fileA;
	char * fileB;
	int tokenCount;
	struct outNode * next;
} outNode;

//end struct definitions

int isDilimiter(char c){//returns 1 if c is a delimiter, 0 otherwise
        if(c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\n' || c == '\r'){
                return 1;
        }else{
                return 0;
        }
}

outNode * initOutNode(float score, char * fileA, char * fileB, int tokenCount){//constructs outnodes
	outNode * node = (outNode *)malloc(sizeof(outNode));
	node->score = score;
	node->fileA = fileA;
	node->fileB = fileB;
	node->tokenCount = tokenCount;
	node->next = NULL;
	return node;
}

tokenNode * initTokenNode(char * token, tokenNode * next){//constructs tokenNodes
	tokenNode * node = (tokenNode *)malloc(sizeof(tokenNode));
	node->token = token;
	node->next = next;
	node->prob = 0;
	node->freq = 1;
	return node;
}

fileNode * initFileNode(char * name, fileNode * next){//constructs fileNodes
	fileNode * node = (fileNode *)malloc(sizeof(fileNode));
	node->name = name;
	node->tokens = 0;
	node->words = 0;
	node->tokenHead = NULL;
	node->next = next;
	return node;
}

void * makeArgs(char * name, char * path, fileNode ** ptr, pthread_mutex_t lock){//constructs an args and returns it as a void *
	args * ret = (args *)malloc(sizeof(args));
	int len;
	if(path == NULL){
		len = strlen(name);
		if(name[len - 1] == '/'){
			name[len - 1] = '\0';
		}
		ret->name = name;
	}else{
		len = strlen(name) + strlen(path) + 2;
		char * newPath = (char *)malloc(sizeof(char) * len);
		newPath[0] = '\0';
		strcat(newPath, path);
		strcat(newPath, "/");
		strcat(newPath, name);
		ret->name = newPath;
		
	}
	
	ret->ptr = ptr;
	ret->lock = lock;
	return (void *)ret;
}

tokenNode * findAndIncr(char * token, fileNode * filePtr){//will search to see if the token is already present in the list and increment it if it is, and insert it if it isnt. returns new tokenHead
	tokenNode * tokenHead = filePtr->tokenHead;
	if(tokenHead == NULL){
		filePtr->tokens++;
		return initTokenNode(token, NULL);
	}
	tokenNode * ptr = tokenHead;
	tokenNode * prev = NULL;
	while(ptr != NULL){
		if(strcmp(token, ptr->token) == 0){
			ptr->freq++;
			free(token);
			return tokenHead;
		}else if(strcmp(token, ptr->token) < 0){
			if(prev == NULL){
				return initTokenNode(token, ptr);
			}
			prev->next = initTokenNode(token, ptr);
			filePtr->tokens++;
			return tokenHead;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	filePtr->tokens++;
	prev->next = initTokenNode(token, NULL);
	return tokenHead;
}

char * parseBuf(char * buf, int *cPtr){//will start scanning the buffer at index (*cPtr), stop at dilimitor, malloc a new char * to hold token, ret ptr to token
	int start = (*cPtr);
        while((*cPtr) < 100 && isDilimiter( buf[(*cPtr)] ) == 0 && (!ispunct( buf[(*cPtr)] ) || buf[(*cPtr)] == '-')){
                (*cPtr)++;
        }
	int len = (*cPtr) - start + 1;
	char * token = (char *)malloc(sizeof(char) * len);
	int i;
	for(i = 0; i < len - 1; i++){
		token[i] = buf[start + i];
	}
	token[len - 1] = '\0';
	return token;
}

void strLower(char * str){//makes str lowercase
	int i;
	int len = strlen(str);
	for(i = 0; i < len; i++){
		if(str[i] != '-'){
			str[i] = tolower(str[i]);
		}
	}
}

void insertFile(fileNode * head, fileNode * insert){//inserts fileNode to LL
	fileNode * ptr = head;
	fileNode * prev = NULL;
	while(ptr != NULL){
		prev = ptr;
		ptr = ptr->next;
	}
	if(prev == NULL){
		return;
	}
	prev->next = insert;
}

void * fileHandler(void * _arg){//primary file handler. tokenizes the given file and stores the distribution in the shared data structure. no return
	args * arg = (args *)_arg;
	int fd = open(arg->name, O_RDONLY);
	if(fd == -1){
		return NULL;
	}
	fileNode * fPtr = initFileNode(arg->name, NULL);
	pthread_mutex_lock(&(arg->lock));//crit section, reading/writing to main LL
	fileNode * fileHead = *(arg->ptr);
	if(fileHead == NULL){
		*(arg->ptr) = fPtr;
	}else{
		insertFile(fileHead, fPtr);
	}
	
	pthread_mutex_unlock(&(arg->lock));
	char buffer[100];
	char * overflow = NULL;
	int * ctr = (int *)malloc(sizeof(int));
	(*ctr) = 0;
	char * token;
	int toBreak = 0;
	size_t bytesRead = read(fd, buffer, 100);
	while(bytesRead){//read from file 100 bytes at a time, then parse through the buffer, tokenize and construct the tokenNode LL
		toBreak = 0;
		while((*ctr) < bytesRead){//while the whole buffer hasnt been parsed...
			while(!isalpha(buffer[*ctr]) || (buffer[*ctr] == '-' && overflow != NULL)){//clears non-alpha chars
				(*ctr)++;
				free(overflow);
				overflow = NULL;
				if((*ctr) >= bytesRead){
					toBreak++;
					break;
				}
			}
			if(toBreak){
				break;
			}
			token = parseBuf(buffer, ctr);
			if(overflow != NULL){//overflow is used to handle when the buffer ends in the middle of a token
				char * temp = (char *)malloc(sizeof(char) * (strlen(token) + strlen(overflow) + 1));
				strcpy(temp, overflow);
				strcat(temp, token);
				free(token);
				free(overflow);
				token = temp;
				overflow = NULL;
			}
			if((*ctr) >= bytesRead){
                                overflow = token;
                                break;
                        }
			strLower(token);
			pthread_mutex_lock(&(arg->lock));
			fPtr->tokenHead = findAndIncr(token, fPtr);
			fPtr->words++;
			pthread_mutex_unlock(&(arg->lock));
		}
		(*ctr) = 0;
		bytesRead = read(fd, buffer, 100);
	}
	close(fd);
	free(ctr);
	free(arg);
	return NULL;
}

void * dirHandler(void * _arg){//primary directory handler. iterates through given directory and creates threads to handle all files/directories, then joins them. no return
	args * arg = (args *)_arg;
	DIR * fd = opendir(arg->name);
	if(fd == NULL){
		return NULL; //failure
	}
	pthread_attr_t attrs;
	pthread_attr_init(&attrs);
	threadNode * threadHead = NULL;
	struct dirent * entry = readdir(fd);
	void * tempArg;
	while(entry != NULL){
		
		if(entry->d_name[0] != '.' && entry->d_type == DT_DIR){//entry's a directory
			threadNode * newThread = (threadNode *)malloc(sizeof(threadNode));
			newThread->next = threadHead;
			threadHead = newThread;
			tempArg = makeArgs(entry->d_name, arg->name, arg->ptr, arg->lock);
			pthread_create(&(newThread->id), &attrs, dirHandler, tempArg);
		}else if(entry->d_name[0] != '.' && entry->d_type == DT_REG){
			threadNode * newThread = (threadNode *)malloc(sizeof(threadNode));
                        newThread->next = threadHead;
                        threadHead = newThread;
			tempArg = makeArgs(entry->d_name, arg->name, arg->ptr, arg->lock);
                        pthread_create(&(newThread->id), &attrs, fileHandler, tempArg);
		}
		entry = readdir(fd);
	}
	closedir(fd);
	threadNode * tPtr = threadHead;
	threadNode * prev = NULL;
	while(tPtr != NULL){//join threads
		prev = tPtr;
		tPtr = tPtr->next;
		pthread_join(prev->id, NULL);
		free(prev);
	}
	free(arg->name);
	free(arg);
	return NULL;
}
/*
void tst(fileNode * fileHead){
	fileNode * filePtr = fileHead;
	tokenNode * tokenPtr;
	while(filePtr != NULL){
		printf("file: %s      tokens: %d\n      words: %d", filePtr->name, filePtr->tokens, filePtr->words);
		tokenPtr = filePtr->tokenHead;
		while(tokenPtr != NULL){
			printf("token: %s   freq: %d\n", tokenPtr->token, tokenPtr->freq);
			tokenPtr = tokenPtr->next;
		}
		printf("\n\n");
		filePtr = filePtr->next;
	}
}
*/

void initProb(fileNode * file){// initializes the probability of each token for one file
	tokenNode * ptr = file->tokenHead;
	while(ptr != NULL){
		ptr->prob = ((float)(ptr->freq))/((float)(file->words));
		ptr = ptr->next;
	}
}

tokenNode * findMeans(tokenNode * tokenHeadA, tokenNode * tokenHeadB){//takes two tokenNode lists and creates a new tokenNode list with a union of tokens from A and B, with their probabilities being the average of the two probsbilities for that token
	tokenNode * ptrA = tokenHeadA;
	tokenNode * ptrB = tokenHeadB;
	tokenNode * meanHead = NULL;
	tokenNode * meanPtr = NULL;
	while(ptrA != NULL){
		if(ptrB == NULL){
			if(meanPtr == NULL){
				meanPtr = initTokenNode(ptrA->token, NULL);
				meanHead = meanPtr;
			}else{
				meanPtr->next = initTokenNode(ptrA->token, NULL);
				meanPtr = meanPtr->next;
			}
			meanPtr->prob = (ptrA->prob) / 2.0;
			ptrA = ptrA->next;
		}else if(strcmp(ptrA->token, ptrB->token) == 0){
			if(meanPtr == NULL){
				meanPtr = initTokenNode(ptrA->token, NULL);
				meanHead = meanPtr;
			}else{
				meanPtr->next = initTokenNode(ptrA->token, NULL);
				meanPtr = meanPtr->next;
			}
			meanPtr->prob = (ptrA->prob + ptrB->prob) / 2.0;
			ptrA = ptrA->next;
			ptrB = ptrB->next;
		}else if(strcmp(ptrA->token, ptrB->token) < 0){
			if(meanPtr == NULL){
				meanPtr = initTokenNode(ptrA->token, NULL);
                        	meanHead = meanPtr;
			}else{
                        	meanPtr->next = initTokenNode(ptrA->token, NULL); 
                        	meanPtr = meanPtr->next;
			}
			meanPtr->prob = (ptrA->prob) / 2.0;
			ptrA = ptrA->next;
		}else{
			if(meanPtr == NULL){
				meanPtr = initTokenNode(ptrB->token, NULL);
				meanHead = meanPtr;
			}else{
				meanPtr->next = initTokenNode(ptrB->token, NULL);
				meanPtr = meanPtr->next;
			}
			meanPtr->prob = (ptrB->prob) / 2.0;
			ptrB = ptrB->next;
		}
	}
	while(ptrB != NULL){
		if(meanPtr == NULL){
			meanPtr = initTokenNode(ptrB->token, NULL);
			meanHead = meanPtr;
		}else{
			meanPtr->next = initTokenNode(ptrB->token, NULL);
			meanPtr = meanPtr->next;
		}
		meanPtr->prob = (ptrB->prob) / 2.0;
		ptrB = ptrB->next;
	}
	return meanHead;
}
/*
void printTokens(tokenNode * ptr){
	while(ptr != NULL){
		printf("token: %s   freq: %d  prob %f\n", ptr->token, ptr->freq, ptr->prob);
		ptr = ptr->next;
	}
}
*/
void freeToken(tokenNode * ptr, int isMean){//frees a tokenNode list. if isMean, meaning its a mean list, it won't free the char * token since its still in use
	tokenNode * temp;
	while(ptr != NULL){
		if(!isMean){
			free(ptr->token);
		}
		temp = ptr->next;
		free(ptr);
		ptr = temp;
	}
}

void freeFile(fileNode * ptr){//frees a fileNode list, and all related mallocd space
	fileNode * temp;
	while(ptr != NULL){
		free(ptr->name);
		freeToken(ptr->tokenHead, 0);
		temp = ptr->next;
		free(ptr);
		ptr = temp;
	}
}

float divergance(tokenNode * headA, tokenNode * headB, tokenNode * meanHead){//takes two tokenNode lists and their mean list and finds and returns their Jensen-Shannon Distance
	tokenNode * ptrA = headA;
	tokenNode * ptrB = headB;
	tokenNode * meanPtr = meanHead;
	float divA = 0;
	float divB = 0;
	float ratio;
	while(meanPtr != NULL){
		if(ptrA != NULL && strcmp(ptrA->token, meanPtr->token) == 0){
			if(ptrB != NULL && strcmp(ptrB->token, meanPtr->token) == 0){
				ratio = (ptrB->prob)/(meanPtr->prob);
				divB = divB + (ptrB->prob * log10(ratio));
				ptrB = ptrB->next;
			}
			ratio = (ptrA->prob)/(meanPtr->prob);
			divA = divA + (ptrA->prob * log10(ratio));
			ptrA = ptrA->next;
			meanPtr = meanPtr->next;
		}else{
			ratio = (ptrB->prob)/(meanPtr->prob);
			divB = divB + (ptrB->prob * log10(ratio));
			ptrB = ptrB->next;
			meanPtr = meanPtr->next;
		}
	}
	//printf("divA: %f   divB: %f\n", divA, divB);
	return ((divA + divB) / 2.0);
}

outNode * insertOut(outNode * head, outNode * toInsert){//inserts outNode in ascending order by the total tokens between the two files compared
	if(head == NULL){
		return toInsert;
	}
	outNode * ptr = head;
	outNode * prev = NULL;
	while(ptr != NULL){
		if(toInsert->tokenCount < ptr->tokenCount){
			toInsert->next = ptr;
			if(prev == NULL){
				return toInsert;
			}
			prev->next = toInsert;
			return head;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	prev->next = toInsert;
	return head;
}

int main(int argc, char ** argv){
	if(argc < 2){
		printf("No input");
		return EXIT_FAILURE;
	}
	int i;
	DIR * fd = opendir(argv[1]);
	if(fd == NULL){
		printf("Error opening directory");
		return EXIT_FAILURE;
	}
	closedir(fd);
	int dirLen = strlen(argv[1]);
	char * dirName;
	if(argv[1][0] == '"'){
		dirName = (char *)malloc(sizeof(char) * (dirLen-1));
		for(i = 0; i < dirLen - 2; i++){
			dirName[i] = argv[1][i + 1];
		}
		dirName[dirLen - 2] = '\0';
	}else{
		dirName = (char *)malloc(sizeof(char) * (dirLen+1));
		strcpy(dirName, argv[1]);
	}
	pthread_mutex_t lockA;
	pthread_mutex_init(&lockA, NULL);
	fileNode ** fileHeadPtr = (fileNode **)malloc(sizeof(fileNode *));
	(*fileHeadPtr) = NULL;
	void * firstArgs = makeArgs(dirName, NULL, fileHeadPtr, lockA);
	dirHandler(firstArgs);
	//tst((*fileHeadPtr));
	if((*fileHeadPtr) == NULL){
		printf("error");
		return EXIT_FAILURE;
	}
	fileNode * filePtr = (*fileHeadPtr);
	int ctr = 0;
	while(filePtr != NULL){
		initProb(filePtr);
		filePtr = filePtr->next;
		ctr++;
	}
	if(ctr < 2){
		printf("Not enough files");
		return EXIT_FAILURE;
	}
	fileNode * filePtrA = (*fileHeadPtr);
	fileNode * filePtrB;
	outNode * outHead = NULL;
	while(filePtrA != NULL){//begins comparisions
		filePtrB = filePtrA->next;
		while(filePtrB != NULL){
			/*
			printf("file %s:    words: %d\n\n", filePtrA->name, filePtrA->words);
			printTokens(filePtrA->tokenHead);
			printf("\n\n");
			printf("file %s:    words: %d\n\n", filePtrB->name, filePtrB->words);
			printTokens(filePtrB->tokenHead);
			printf("\n\n");
			printf("file %s and file %s\n\n", filePtrA->name, filePtrB->name);
			*/
			
			
			

			tokenNode * meanHead = findMeans(filePtrA->tokenHead, filePtrB->tokenHead);
			//printTokens(meanHead);
			//printf("\n\n");
			float score = divergance(filePtrA->tokenHead, filePtrB->tokenHead, meanHead);

			outNode * outTemp = initOutNode(score, filePtrA->name, filePtrB->name, (filePtrA->tokens + filePtrB->tokens));
			outHead = insertOut(outHead, outTemp);
			freeToken(meanHead, 1);
			filePtrB = filePtrB->next;
		}
		filePtrA = filePtrA->next;
	}

	outNode * outPtr = outHead;
	outNode * outPrev = NULL;
	
	while(outPtr != NULL){
		if(outPtr->score <= 0.1){
			printf("\033[0;31m");
		}else if(outPtr->score <= 0.15){
                        printf("\033[0;33m");
                }else if(outPtr->score <= 0.2){
                        printf("\033[0;32m");
                }else if(outPtr->score <= 0.25){
			printf("\033[0;36m");
		}else if(outPtr->score <= 0.3){
                        printf("\033[0;34m");
		}
                printf("%f\033[0m \"%s\" and \"%s\"\n", outPtr->score, outPtr->fileA, outPtr->fileB);
		outPrev = outPtr;
		outPtr = outPtr->next;
		free(outPrev);
	}
	//
	
	freeFile(*fileHeadPtr);
	free(fileHeadPtr);
	return 1;
}
