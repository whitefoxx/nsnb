#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
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
	if(argc == 2)
		sprintf(buffer, "%s", argv[1]);
	else if(argc == 3)
		sprintf(buffer, "%s %s", argv[1], argv[2]);
	else if(argc == 4)
		sprintf(buffer, "%s %s %s", argv[1], argv[2], argv[3]);
	else
	{
		printf("ERROR wrong parameters\n");
		close(sockfd);
		exit(0);
	}
	n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
	{
        perror("ERROR writing to socket");
		close(sockfd);
		exit(0);
	}
    bzero(buffer,256);
	if(strcmp(argv[1], "-c") == 0)
	{
		n = read(sockfd, buffer, 255);
		if(n > 0)
		{
			write(sockfd, "over", strlen("over"));
			printf("%s", buffer);
		}
	}
	else if(strcmp(argv[1], "-t") == 0)
	{
		n = read(sockfd, buffer, 255);
		if(n > 0 && strcmp(buffer, "done") == 0)
		{
			write(sockfd, "over", strlen("over"));
		}
	}
	close(sockfd);

    return 0;
}
