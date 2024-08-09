
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include "mymalloc.h"

float runtime(struct timeval startTime, struct timeval endTime){
	int secs = endTime.tv_sec - startTime.tv_sec;
        float uSecs = endTime.tv_usec - startTime.tv_usec;
	float frac = uSecs/1000000;
	return frac + secs;
}

float workloadA(){
	struct timeval startTime;
	struct timeval endTime;
	gettimeofday(&startTime, NULL);
	int i;
	for(i = 0; i < 120; i++){
		void * ptr = malloc(1);
		free(ptr);
	}
	gettimeofday(&endTime, NULL);
	return runtime(startTime, endTime);
}

float workloadB(){
	struct timeval startTime;
        struct timeval endTime;
        gettimeofday(&startTime, NULL);
	void * ptrArray[120];
	int i;
	for(i = 0; i < 120; i++){
		ptrArray[i] = malloc(1);
	}
	for(i = 0; i < 120; i++){
		free(ptrArray[i]);
	}
	gettimeofday(&endTime, NULL);
	return runtime(startTime, endTime);
}

float workloadC(){
	struct timeval startTime;
        struct timeval endTime;
        gettimeofday(&startTime, NULL);
	int mallocCtr = 0;
	int freeCtr = 0;
	int i;
	void * ptrs[120];
	for(i = 0; i < 240; i++){
		//printf("i = %d, mallocCtr = %d, freeCtr = %d\n", i, mallocCtr, freeCtr);
		if(mallocCtr == freeCtr){
			ptrs[mallocCtr] = malloc(1);
			mallocCtr++;
		}else if(mallocCtr >= 120){
			free(ptrs[freeCtr]);
			freeCtr++;
		}else{
			if(rand() % 2 == 0){
				ptrs[mallocCtr] = malloc(1);
				mallocCtr++;
			}else{
				if(ptrs[freeCtr] != NULL){
					free(ptrs[freeCtr]);
					freeCtr++;
				}else{
					printf("error in workloadC");
				}
			}
		}
	}
	gettimeofday(&endTime, NULL);
	return runtime(startTime, endTime);
}

float workloadD(){
	struct timeval startTime;
        struct timeval endTime;
        gettimeofday(&startTime, NULL);
	int mallocCtr = 0;
        int freeCtr = 0;
        int i;
        void * ptrs[120];
        for(i = 0; i < 240; i++){
		int r = (rand() % 20) + 1;
                //printf("i = %d, mallocCtr = %d, freeCtr = %d\n", i, mallocCtr, freeCtr);
                if(mallocCtr == freeCtr){
                        ptrs[mallocCtr] = malloc(r);
                        mallocCtr++;
                }else if(mallocCtr >= 120){
                        free(ptrs[freeCtr]);
                        freeCtr++;
                }else{
                        if(rand() % 2 == 0){
                                ptrs[mallocCtr] = malloc(r);
                                mallocCtr++;
                        }else{
                                if(ptrs[freeCtr] != NULL){
                                        free(ptrs[freeCtr]);
                                        freeCtr++;
                                }else{
					printf("error in workloadD");
				}
                        }
                }
        }
	gettimeofday(&endTime, NULL);
        return runtime(startTime, endTime);

}
void useMem(void * vPtr, int size){//simulates use of allocated memory by randomly changing values in block
	char * ptr = (char *)vPtr;
	int i;
	for(i = 0; i < size; i++){
		char num = (rand() % 10) + '0';
		ptr[i] = num;
	}
}


float workloadE(){
	struct timeval startTime;
        struct timeval endTime;
        gettimeofday(&startTime, NULL);
	int mallocCtr = 0;
        int freeCtr = 0;
        int i;
        void * ptrs[120];
        for(i = 0; i < 240; i++){
                int r = (rand() % 20) + 1;
                //printf("i = %d, mallocCtr = %d, freeCtr = %d\n", i, mallocCtr, freeCtr);
                if(mallocCtr == freeCtr){
                        ptrs[mallocCtr] = malloc(r);
			useMem(ptrs[mallocCtr], r);
                        mallocCtr++;
                }else if(mallocCtr >= 120){
                        free(ptrs[freeCtr]);
                        freeCtr++;
                }else{
                        if(rand() % 2 == 0){
                                ptrs[mallocCtr] = malloc(r);
				useMem(ptrs[mallocCtr], r);
                                mallocCtr++;
                        }else{
                                if(ptrs[freeCtr] != NULL){
                                        free(ptrs[freeCtr]);
                                        freeCtr++;
                                }else{
					printf("error in workloadE");
				}
                        }
                }
        }
	gettimeofday(&endTime, NULL);
        return runtime(startTime, endTime);
}

int main(int argc, char ** argv){
	int i;
	float meanA = 0;
        float meanB = 0;
        float meanC = 0;
        float meanD = 0;
        float meanE = 0;
	for(i = 0; i < 50; i++){
		meanA += workloadA();
		meanB += workloadB();
		meanC += workloadC();
		meanD += workloadD();
		meanE += workloadE();
	}
	meanA = meanA/50;
	meanB = meanB/50;
	meanC = meanC/50;
	meanD = meanD/50;
	meanE = meanE/50;
	printf("Mean runtimes:\nWorkload A: %f seconds\nWorkload B: %f seconds\nWorkload C: %f seconds\nWorkload D: %f seconds\nWorkload E: %f seconds\n", meanA, meanB, meanC, meanD, meanE);
	return 0;
}
