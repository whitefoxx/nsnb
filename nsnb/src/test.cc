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

#ifndef MAX_ACC_LENGTH
#define MAX_ACC_LENGTH MAX_READ_LENGTH + 10
#endif

int main(int argc, char *argv[])
{
	clock_t begin = clock();
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[MAX_ACC_LENGTH];
	char recv_buff[256];
	char content_buff[MAX_READ_LENGTH];
    portno = 21234;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
		perror("ERROR opening socket");
		exit(0);
	}
    server = gethostbyname("127.0.0.1");
    if (server == NULL) {
        printf("ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0) 
	{
        perror("ERROR connecting");
		exit(0);
	}
	
	if(argc < 2)
	{
		printf("ERROR, missing corpusPath.\nusage: test corpusPath\n");
		exit(0);
	}
	string	corpusPath = string(argv[1]);
	string indexPath = corpusPath + "index";
	ifstream index;
	index.open(indexPath.c_str());
	string emailType, emailPath;
	int n;
	while (index >> emailType >> emailPath) {
		memset(content_buff, 0, MAX_READ_LENGTH);
		emailPath = corpusPath + emailPath;
		ifstream email;
		email.open(emailPath.c_str());
		email.read(content_buff, MAX_READ_LENGTH - 1);
		email.close();

		sprintf(buffer, "-c %s", content_buff);
		write(sockfd, buffer, strlen(buffer));
		// 下面语句会产生阻塞,假如没有下面这句的话,服务器端可能会将本来想两次传输的数据一次接受了
		memset(recv_buff, 0, 256);
		n = read(sockfd, recv_buff, 255);
		fprintf(stdout, "%s judge=%s %s\n", emailPath.c_str(), emailType.c_str(), recv_buff);
		sprintf(buffer, "-t %s %s", emailType.c_str(), content_buff);
		write(sockfd, buffer, strlen(buffer));
		// 同上
		n = read(sockfd, recv_buff, 255);
	}
	index.close();
	
	close(sockfd);
	
	clock_t end = clock();
	// printf("Total time: %lf\ns", (end - begin) * 1.0 / CLOCKS_PER_SEC);
	
	return 0;
}
