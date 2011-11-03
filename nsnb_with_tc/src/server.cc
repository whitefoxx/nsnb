#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <fstream>

#include "nsnb.h"

void train(set<string> featureSet, string emailType, TCHDB* featureMap)
{
    void* buff = (struct feature*)malloc(sizeof(struct feature));
    int rcode;
	int ecode;
    
    double score = predict(featureSet, featureMap);
    
    if (emailType == "ham") 
        total_ham++;
    else 
        total_spam++;
    
    while (emailType == "spam" && score < 0.5 + THICKNESS)
    {
        train_cell(featureSet, emailType, featureMap);
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++) {
            string str = *iter;
            rcode = tchdbget3(featureMap, str.c_str(), str.length(), buff, sizeof(struct feature));
            struct feature* tmp = (struct feature*)buff;
            tmp->confidence /= LEARNING_RATE;
            if(!tchdbput(featureMap, str.c_str(), str.length(), tmp, sizeof(struct feature)))
			{
				ecode = tchdbecode(featureMap);
		        fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
			}
        }
        total_spam--;
        score = predict(featureSet, featureMap);
    }
    while (emailType == "ham" && score > 0.5 - THICKNESS)
    {
        train_cell(featureSet, emailType, featureMap);
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++) {
            string str = *iter;
            rcode = tchdbget3(featureMap, str.c_str(), str.length(), buff, sizeof(struct feature));
            struct feature* tmp = (struct feature*)buff;
            tmp->confidence *= LEARNING_RATE;
            if(!tchdbput(featureMap, str.c_str(), str.length(), tmp, sizeof(struct feature)))
			{
				ecode = tchdbecode(featureMap);
		        fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
			}
        }
        total_ham--;
        score = predict(featureSet, featureMap);
    }
    free(buff);
}

void train_cell(set<string> featureSet, string emailType, TCHDB* featureMap)
{
    void* buff = (struct feature*)malloc(sizeof(struct feature));
    int rcode;
	int ecode;
    
    if (emailType == "spam")
    {
        total_spam++;
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++) {
            string str = *iter;
            rcode = tchdbget3(featureMap, str.c_str(), str.length(), buff, sizeof(struct feature));
            if (rcode == -1) {
                struct feature tmp = {0, 1, 1.0};
                if(!tchdbput(featureMap, str.c_str(), str.length(), &tmp, sizeof(struct feature)))
				{
					ecode = tchdbecode(featureMap);
			        fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
				}
            }
            else {
                struct feature* tmp = (struct feature*)buff;
                tmp->spam += 1;
                if(!tchdbput(featureMap, str.c_str(), str.length(), tmp, sizeof(struct feature)))
				{
					ecode = tchdbecode(featureMap);
			        fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
				}
            }
        }
    }
    else
    {
        total_ham++;
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++) {
            string str = *iter;
            rcode = tchdbget3(featureMap, str.c_str(), str.length(), buff, sizeof(struct feature));
            if (rcode == -1) {
                struct feature tmp = {1, 0, 1.0};
                if(!tchdbput(featureMap, str.c_str(), str.length(), &tmp, sizeof(struct feature)))
				{
					ecode = tchdbecode(featureMap);
			        fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
				}
            }
            else {
                struct feature* tmp = (struct feature*)buff;
                tmp->ham += 1;
                if(!tchdbput(featureMap, str.c_str(), str.length(), tmp, sizeof(struct feature)))
				{
					ecode = tchdbecode(featureMap);
			        fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
				}	
            }
        }
    }
    free(buff);
}

double predict(set<string> featureSet, TCHDB* featureMap)
{
    void* buff = (struct feature*)malloc(sizeof(struct feature));
    int rcode;
    
    double score = 0.0;
    int s, h;
    double c;
    for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++)
    {
        string str = *iter;
        rcode = tchdbget3(featureMap, str.c_str(), str.length(), buff, sizeof(struct feature));
        if (rcode == -1)
            continue;
        struct feature* tmp = (struct feature*)buff;
        s = tmp->spam;
        h = tmp->ham;
        c = tmp->confidence;
        score += log((s + SMOOTH) / (h + SMOOTH) * (total_ham + 2 * SMOOTH) / (total_spam + 2 * SMOOTH) * c);
    }
    score += log((total_spam + SMOOTH) / (total_ham + SMOOTH));
    score = logist(score / SCALE);
    free(buff);
    return score;
}

inline double logist(double x)
{
    return 1.0 / (1.0 + exp(-x));
}

set<string> transform(string emailContent)
{
    set<string> featureSet;
    int len = emailContent.length();
    int i = 0;
    for (i = 0; i <= len - NGRAM; i++)
    {
        string feature = emailContent.substr(i, NGRAM);
        featureSet.insert(feature);
    }
    return featureSet;
}

void predict_without_train(char *path, char *prediction, double *score)
{
	ifstream in;
	char buff[MAX_READ_LENGTH];
	set<string> featureSet;

	in.open(path);
	in.read(buff, MAX_READ_LENGTH);
	in.close();
	featureSet = transform(string(buff));
	*score = predict(featureSet, featureMap);
	if(*score > 0.5)
		strcpy(prediction, "spam");
	else
		strcpy(prediction, "ham");		
}

void predict_a_mail(char *path, char *tag)
{
	ifstream in;
	char buff[MAX_READ_LENGTH];
	set<string> featureSet;

	in.open(path);
	in.read(buff, MAX_READ_LENGTH);
	in.close();
	featureSet = transform(string(buff));
	train(featureSet, string(tag), featureMap);	
}

void init()
{
    int ecode;
    featureMap = tchdbnew();
    if(!tchdbopen(featureMap, "db/featureMap.tch", HDBOWRITER | HDBOCREAT)){
        ecode = tchdbecode(featureMap);
        fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
    }
	
}

int main(int argc, char *argv[])
{
	 int sockfd, newsockfd, portno, clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
	 char predict[5];
	 char *path = NULL;
	 char *catg = NULL;
	 char *parameter = NULL;
	 char labelReq[10];
	 char result[100];
	 double score;

	 init(); //initial featureMap in nsnb

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
	 {
         perror("ERROR opening socket");
		 exit(1);
	 }
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = 21234; 
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	 {
         perror("ERROR on binding");
		 exit(1);
	 }
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
	 while(true)
	 {
		 newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)&clilen);
		 if (newsockfd < 0) 
		 {
			 perror("ERROR on accept");
			 continue;
		 }
		 while(true)
		 {
			 bzero(buffer, 256);
			 n = read(newsockfd, buffer, 255);
			 if(n < 0)
			 {
				 perror("ERROR reading from socket");
				 break;
			 }
			 parameter = strtok(buffer, " ");
			 if(parameter == NULL)
			 {
				 printf("ERROR no parameter\n");
				 break;
			 }
			 if(strcmp("exit", parameter) == 0)
			 {
				 close(newsockfd);
				 close(sockfd);
				 exit(0);
			 }
			 if(strcmp("-t", parameter) == 0)
			 {
				 catg = strtok(NULL, " ");
				 if(catg != NULL)
					 path = strtok(NULL, " ");
				 else
			     {
					 printf("ERROR parameter after -t\n");
					 break;
				 }
				 if(path == NULL)
				 {
					 printf("ERROR parameter after -t\n");
					 break;
				 }
				 predict_a_mail(path, catg);
				 write(newsockfd, "done", strlen("done"));
			 }
			 if(strcmp("-c", parameter) == 0)
			 {
				 path = strtok(NULL, " ");
				 if(path == NULL)
				 {
					 printf("ERROR parameter after -c\n");
					 break;
				 }
				 predict_without_train(path, predict, &score);
				 if(score > THRESHOLD - ACTIVE_TRADEOFF && score < THRESHOLD + ACTIVE_TRADEOFF)
					 strcpy(labelReq, "labelN");
				 else
					 strcpy(labelReq, "noRequest");
			     sprintf(result, "class=%s score=%lf tfile=* labelReq=%s", predict, score, labelReq);
				 write(newsockfd, result, strlen(result));
			 }
			 if(strcmp("over", parameter) == 0)
			 {
				 break;
			 }
		 }
		 close(newsockfd);
	 }
	 close(sockfd);

     return 0; 
}
