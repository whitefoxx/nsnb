#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>
#include <string>
#include <fstream>
#include <ctime>

#include "nsnb.h"

#define MAX_SEND_LENGTH MAX_READ_LENGTH + 10
#define PORTNO 21234
#define HOSTNAME "127.0.0.1"

inline bool check_err(const bool err, const char *errMsg)
{
	if(err)	fprintf(stderr, "%s\n", errMsg);
	return err;
}

int main(int argc, char *argv[])
{
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

	if (argc < 2) {
		perror("ERROR: parameters were not enough.\nusage ...");
		exit(0);
	}

    char *email_buff;
	char recv_buff[256];
	char send_buff[MAX_SEND_LENGTH];
    portno = PORTNO;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
		perror("ERROR: opening socket");
		exit(0);
	}
    server = gethostbyname(HOSTNAME);
    if (server == NULL) {
        perror("ERROR: no such host");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0) 
	{
        perror("ERROR: connecting");
		exit(0);
	}

	if(strcmp(argv[1], "exit") == 0) {
		write(sockfd, "exit", strlen("exit"));
		return 0;		
	}
	
	char ch;
	int n, len_par;
	int nbyte = 0;
	int length = 4096;
	email_buff = (char*)malloc(sizeof(char)*length);
	if(check_err(email_buff == NULL, "ERROR: no enough memory"))
		exit(0);

	while (scanf("%c", &ch) != EOF) {
		if(nbyte >= length) {
			char *tmp = email_buff;
			length *= 2;
			email_buff = (char*)malloc(sizeof(char)*length);
			if(check_err(email_buff == NULL, "ERROR: no enough memory"))
				exit(0);
			memcpy(email_buff, tmp, nbyte);
			free(tmp);
		}
		email_buff[nbyte++] = ch;
	}

	bzero(send_buff, MAX_SEND_LENGTH);
	bzero(recv_buff, 256);
	if (strcmp(argv[1], "-c") == 0) {
		strcpy(send_buff, "-c ");
		len_par = 3;
	}
	else if (strcmp(argv[1], "-t") == 0 ) {
		sprintf(send_buff, "-t %s ", argv[2]);
		len_par = 4 + strlen(argv[2]);
	}
	if(nbyte >= MAX_READ_LENGTH) {
		memcpy(send_buff + len_par, email_buff, MAX_READ_LENGTH);
		nbyte = MAX_READ_LENGTH;
	}
	else {
		memcpy(send_buff + len_par, email_buff, nbyte);
	}
	write(sockfd, send_buff, nbyte + len_par);
	n = read(sockfd, recv_buff, 255);
	if (strcmp(argv[1], "-c") == 0) {
		printf("%s\n", recv_buff);
	}
	//fprintf(stdout, "%s\n%s", recv_buff, email_buff);
	write(sockfd, "over", strlen("over"));

	close(sockfd);
	free(email_buff);
	
	return 0;
}
