#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>


typedef struct packet{
	int isErr;
	int error;
	int len;
	int currentSection;//to assist parsing, 0 = msg type, 1 = len, 2 = msg, 3 = complete
	char * message;//also used to store overflow when parsing
} packet;

char * parse(char * buf, int * _idx, int size){//takes the buffer and returns the next token from it, incrementing the idx as well
	int idx = (*_idx);
	int start = idx;
	int len = 0;
	while(idx < size){
		if(buf[idx] == '|'){
			break;
		}
		idx++;
		len++;
	}
	if(len == 0){
		return NULL;
	}
	char * ret = (char *)malloc(sizeof(char) * len + 1);
	int i;
	for(i = 0; i < len; i++){
		ret[i] = buf[start + i];
	}
	ret[len] = '\0';
	(*_idx) = idx;
	return ret;
}

int parseBuf(char * buf, packet *  message, int size){//returns 1 if incomplete, 0 otherwise, parses buffer into a packet struct
	int idx = 0;
	char * word;
	while(idx < size || (message->currentSection < 3 && message->isErr != 1)){
		if(buf[idx] == '|'){
			idx++;
			continue;
		}
		word = parse(buf, &idx, size);
		if(message->message != NULL){
			int len = strlen(message->message) + strlen(word) + 1;
			char * newWord = (char *)malloc(sizeof(char) * len);
			strcpy(newWord, message->message);
			strcat(newWord, word);
			free(message->message);
			free(word);
			word = newWord;
			message->message = NULL;
		}
		if(buf[idx] != '|'){
			message->message = word;
			break;
		}
		if(message->currentSection == 0){
			if(strcmp(word, "REG") == 0){
				message->isErr = 0;
			}else if(strcmp(word, "ERR") == 0){
				message->isErr = 1;
			}else{
				return -1;
			}
			message->currentSection++;
			free(word);
		}else if(message->currentSection == 1 && message->isErr != 1){
			message->len = atoi(word);
			free(word);
			message->currentSection++;
		}else{
			message->message = word;
			message->currentSection++;
		}
	}
	
	if(message->currentSection != 3 && message->isErr != 1){
		return 1;
	}
	return 0;
}

packet * constructPacket(int fd){//main incoming message handler, takes a socket, listens for the next message, and parses it into a usable message. returns the parsed packet
	char buffer[256];
        int bufSize = read(fd, buffer, 256);
	packet * message = (packet *)malloc(sizeof(packet));
	message->currentSection = 0;
	message->message = NULL;
	int parseRet = parseBuf(buffer, message, bufSize);
	while(parseRet  == 1){
		bufSize = read(fd, buffer, 256);
		parseRet = parseBuf(buffer, message, bufSize);
	}
	if(parseRet == -1){
		message->isErr = 1;
	}

	return message;
}
char ** makeMsgList(){//just to save time this hard codes the messages
	char ** msgs = (char **)malloc(sizeof(char *) * 6);
	msgs[0] = "Knock, Knock.";
	msgs[1] = "Who's there?";
	msgs[2] = "Who.";
	msgs[3] = "Who, who?";
	msgs[4] = "I didn't know you were an owl!";
	msgs[5] = "Ugh.";
	return msgs;
}

char * makeMsg(int isErr, char * word){//given the string word and whether or not its an error message, this returns a properly formatted message
	char * msg;
	if(isErr){
		msg = (char *)malloc(sizeof(char) * 10);
		strcpy(msg, "ERR|");
		strcat(msg, word);
		strcat(msg, "|");
		
	}else{
		int wordLen = strlen(word);
		int wordLenChars = log10(wordLen) + 1;
		msg = (char *)malloc(sizeof(char) * (wordLen + wordLenChars + 7));
		strcpy(msg, "REG|");
		
		char * lenTemp = (char *)malloc(sizeof(char) * (wordLenChars + 1));
		sprintf(lenTemp, "%d", wordLen);
		lenTemp[wordLenChars] = '\0';
		//printf("msg: %s\nlenTemp: %s\nmsg malloc:%d, msg size:%ld\nlenTemp malloc:%d, lenTemp size:%ld\n", msg, lenTemp, (wordLen +wordLenChars+7), strlen(msg), (wordLenChars+1), strlen(lenTemp));
		strcat(msg, lenTemp);
		free(lenTemp);
		strcat(msg, "|");
		strcat(msg, word);
		strcat(msg, "|");
	}
	return msg;
}

void freePacket(packet * ptr){//frees the packet struct
	free(ptr->message);
	free(ptr);
}

void incomingErrorHandler(packet * message){//handles errors when they are recieved, given the error message. Prints out appropriate description
	char * messages[6][3];
        messages[0][0] = "M0CT";
        messages[0][1] = "M0LN";
        messages[0][2] = "M0FT";
        messages[1][0] = "M1CT";
        messages[1][1] = "M1LN";
        messages[1][2] = "M1FT";
        messages[2][0] = "M2CT";
        messages[2][1] = "M2LN";
        messages[2][2] = "M2FT";
        messages[3][0] = "M3CT";
        messages[3][1] = "M3LN";
        messages[3][2] = "M3FT";
        messages[4][0] = "M4CT";
        messages[4][1] = "M4LN";
        messages[4][2] = "M4FT";
        messages[5][0] = "M5CT";
        messages[5][1] = "M5LN";
        messages[5][2] = "M5FT";
	char * responses[6][3];
	responses[0][0] = "message 0 content was not correct";
	responses[0][1] = "message 0 length value was incorrect";
        responses[0][2] = "message 0 format was broken";
        responses[1][0] = "message 1 content was not correct";
        responses[1][1] = "message 1 length value was incorrect";
        responses[1][2] = "message 1 format was broken";
        responses[2][0] = "message 2 content was not correct";
        responses[2][1] = "message 2 length value was incorrect";
        responses[2][2] = "message 2 format was broken";
        responses[3][0] = "message 3 content was not correct";
        responses[3][1] = "message 3 length value was incorrect";
        responses[3][2] = "message 3 format was broken";
        responses[4][0] = "message 4 content was not correct";
        responses[4][1] = "message 4 length value was incorrect";
        responses[4][2] = "message 4 format was broken";
        responses[5][0] = "message 5 content was not correct";
        responses[5][1] = "message 5 length value was incorrect";
        responses[5][2] = "message 5 format was broken";
	int i;
	int j;
	for(i = 0; i < 6; i++){
		for(j = 0; j < 3; j++){

			if(strcmp(message->message, messages[i][j]) == 0 ){
				printf("%s\n", responses[i][j]);
				return;
			}
		}
	}
	printf("Error message handling\n");
}

int errorCheck(packet * message, int step){//checks incoming messages for errors in format/content/length
	char * errorMsg;
	char * messages[6][3];
	messages[0][0] = "M0CT";
	messages[0][1] = "M0LN";
	messages[0][2] = "M0FT";
	messages[1][0] = "M1CT";
	messages[1][1] = "M1LN";
	messages[1][2] = "M1FT";
	messages[2][0] = "M2CT";
	messages[2][1] = "M2LN";
	messages[2][2] = "M2FT";
        messages[3][0] = "M3CT";
        messages[3][1] = "M3LN";
        messages[3][2] = "M3FT";
        messages[4][0] = "M4CT";
        messages[4][1] = "M4LN";
        messages[4][2] = "M4FT";
        messages[5][0] = "M5CT";
        messages[5][1] = "M5LN";
        messages[5][2] = "M5FT";
        char ** correctMsgs = makeMsgList();
	int isOutgoingErr = 0;
	


	if(message->isErr){
		int i;
		int j;
		for(i = 0; i < 6; i++){//because of the ambiguity around whether isErr was set due to an error during the parsing process or if it was set because the packet is an error message, this check is necessary
			for(j = 0; j < 3; j++){
				if(strcmp(message->message, messages[i][j]) == 0){
					incomingErrorHandler(message);
					return 2;
				}
			}
		}
		free(message->message);
		message->message = messages[step][2];
		free(correctMsgs);
		return 1;
	}
	if(strlen(message->message) != message->len){
		free(message->message);
		message->isErr = 1;
		message->message = messages[step][1];
		free(correctMsgs);
		return 1;
	}
	if(strcmp(message->message, correctMsgs[step]) != 0){
		free(message->message);
		message->isErr = 1;
		message->message = messages[step][0];
		free(correctMsgs);
		return 1;
	}
	free(correctMsgs);
	return 0;
}

void serverDriver(int fd){//main program driver. given a socket fd, it will walk through the knock knock joke structure unless it encounters an error
	int step = 0;
	char **	 msgList = makeMsgList();
	char * msg = makeMsg(0, msgList[0]);
	write(fd, msg, strlen(msg));
	step++;
	free(msg);
	packet * message = constructPacket(fd);
	int errorStatus = errorCheck(message, step);
	if(errorStatus == 1){
		msg = makeMsg(1, message->message);
		write(fd, msg, strlen(msg));
		free(message);
		return;
	}else if(errorStatus == 2){
		free(message);
		return;
	}
	freePacket(message);
	step++;
	msg = makeMsg(0, msgList[2]);
	write(fd, msg, strlen(msg));//
	free(msg);
	step++;
	message = constructPacket(fd);
	errorStatus = errorCheck(message, step);
	if(errorStatus == 1){
                msg = makeMsg(1, message->message);
                write(fd, msg, strlen(msg));
                free(message);
                return;
        }else if(errorStatus == 2){
		free(message);
		return;
	}
	freePacket(message);//
	step++;
	msg = makeMsg(0, msgList[4]);
	write(fd, msg, strlen(msg));
	free(msg);//
	step++;
	message = constructPacket(fd);
	errorStatus = errorCheck(message, step);
        if(errorStatus == 1){
                msg = makeMsg(1, message->message);
                write(fd, msg, strlen(msg));
                free(message);
                return;
        }else if(errorStatus == 2){
		free(message);
		return;
	}
        freePacket(message);//
}

int main(int argc, char ** argv){
	if(argc < 2){
		return EXIT_FAILURE;
	}
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serveStruct;
	serveStruct.sin_family = AF_INET;
	serveStruct.sin_addr.s_addr = INADDR_ANY;
	serveStruct.sin_port = atoi(argv[1]);
	int serveLen = sizeof(serveStruct);
	if(bind(sfd, (struct sockaddr *)&serveStruct, serveLen) == -1){
		return EXIT_FAILURE;
	}
	if(listen(sfd, 10) == -1){
		return EXIT_FAILURE;
	}
	int newSock;
	while(1){
		newSock = accept(sfd, (struct sockaddr *)&serveStruct, &serveLen);
		if(newSock == -1){
			return EXIT_FAILURE;
		}
		serverDriver(newSock);
	}
}
