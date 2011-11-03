#include <fstream>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <fstream>

#include "nsnb.h"

/* MAX_RECV_LENGTH 指server socket每次最多接收的字符个数, MAX_READ_LENGTH 指邮件字符的最多个数, 
   由于加上一些控制过滤器predict或train的参数(-c, -t...) 所以要比 MAX_READ_LENGTH 个数要多些 */

#define MAX_RECV_LENGTH MAX_READ_LENGTH + 10
#define PORTNO 21234

inline bool check_err(const bool err, const char *errMsg)
{
	if(err)	fprintf(stderr, "%s\n", errMsg);
	return err;
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, clilen;
	char result[64];
    char buffer[MAX_RECV_LENGTH];
    struct sockaddr_in serv_addr, cli_addr;
	char *catg = NULL;
	char *parameter = NULL;

	TCHDB* featureMap = tchdbnew();
	if (!tchdbopen(featureMap, "db/featureMap.tch", HDBOWRITER | HDBOCREAT)) {
		int ecode = tchdbecode(featureMap);
		fprintf(stderr, "TC open error: %s\n", tchdberrmsg(ecode));
	}

	// init socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(check_err(sockfd < 0, "ERROR: opening socket")) exit(1);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = PORTNO; 								// 连接的端口号
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
	// converts a port number in host byte order to a port number in network byte order
    serv_addr.sin_port = htons(portno);				
    if(check_err(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0, "ERROR: on binding"))
		exit(1);
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

	int check_count = 0;
	bool exit = false;
	while(!exit)
	{
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)&clilen);
		if(check_err(newsockfd < 0, "ERROR: on accept a socket")) continue;
		while(true)
	 	{
		 	bzero(buffer, MAX_RECV_LENGTH);
			int n = read(newsockfd, buffer, MAX_RECV_LENGTH);
			if(n <= 0) {
				write(newsockfd, "fail: email content is empty", strlen("fail: email content is empty"));
				break;
			}
			parameter = strtok(buffer, " ");
			if(check_err(parameter == NULL, "ERROR: no parameter")) {
				write(newsockfd, "fail: no parameter", strlen("fail: no parameter"));
				break;
			}
			// case train:
			if(strcmp("exit", parameter) == 0)
			{
				exit = true;
				break;
			}
			else if(strcmp("-t", parameter) == 0)
			{
				catg = strtok(NULL, " ");
				if(check_err(catg == NULL, "ERROR: no emailType")) {
					write(newsockfd, "fail: no email type for train", strlen("fail: no email type for train"));
					break;
				}
				string emailType = string(catg);
				buffer[4 + emailType.length() + MAX_READ_LENGTH] = '\0';
				set<string> featureSet = transform(string(buffer + 4 + emailType.length()));
				nsnb_train(featureSet, emailType, featureMap);
				write(newsockfd, "trained", strlen("trained"));
			}
			// case predict:
			else if(strcmp("-c", parameter) == 0)
			{
				buffer[3 + MAX_READ_LENGTH] = '\0';
				set<string> featureSet = transform(string(buffer + 3));
				double score = nsnb_predict(featureSet, featureMap);
				string prediction;
				if (score > 0.618) prediction = "spam";
				else prediction = "ham";
				sprintf(result, "class=%-4s score=%.8f", prediction.c_str(), score);
				write(newsockfd, result, strlen(result));
			}
			// case over:
			else if(strcmp("over", parameter) == 0) {
				break;
			}
			else {
				fprintf(stderr, "ERROR: wrong parameter '%s'\n", parameter);
			}
		}
		close(newsockfd);
	}
	close(sockfd);

	if (!tchdbclose(featureMap)) {
		int ecode = tchdbecode(featureMap);
		fprintf(stderr, "TC close error: %s\n", tchdberrmsg(ecode));
	}

	tchdbdel(featureMap);

	return EXIT_SUCCESS;
}
