#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <cmath>
#include <ctime>
#include <string>
#include <fstream>

#include <tcutil.h>
#include <tchdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "config.h"
#include "nsnb.h"

int dimension = 0;

void train(set<int> featurePosition, string emailType)
{
    double score = predict(featurePosition);
    if (emailType == "ham")
        total_ham++;
    else
        total_spam++;
    while (emailType == "spam" && score < 0.5 + THICKNESS)
    {
        for (set<int>::iterator iter = featurePosition.begin(); iter != featurePosition.end(); iter++)
            confidence[*iter] /= LEARNING_RATE;
        train_cell(featurePosition, emailType);
        total_spam--;
        score = predict(featurePosition);
    }
    while (emailType == "ham" && score > 0.5 - THICKNESS)
    {
        for (set<int>::iterator iter = featurePosition.begin(); iter != featurePosition.end(); iter++)
            confidence[*iter] *= LEARNING_RATE;
        train_cell(featurePosition, emailType);
        total_ham--;
        score = predict(featurePosition);
    }
}

void prepare()
{
    int i = 0;
    for (i = 0; i < MAX_DIMENSION; i++)
        confidence[i] = 1.0;
}

void train_cell(set<int> featurePosition, string emailType)
{
    if (emailType == "spam")
    {
        total_spam++;
        for (set<int>::iterator iter = featurePosition.begin(); iter != featurePosition.end(); iter++)
            spam_words[*iter] += 1;
    }
    else
    {
        total_ham++;
        for (set<int>::iterator iter = featurePosition.begin(); iter != featurePosition.end(); iter++)
            ham_words[*iter] += 1;
    }
}

double predict(set<int> featurePosition)
{
    double score = 0.0;
    int s, h;
    for (set<int>::iterator iter = featurePosition.begin(); iter != featurePosition.end(); iter++)
    {
        s = spam_words[*iter];
        h = ham_words[*iter];
        if (s == 0 && h == 0)
            continue;
        score += log((s + SMOOTH) / (h + SMOOTH) * (total_ham + 2 * SMOOTH) / (total_spam + 2 * SMOOTH) * confidence[*iter]);
    }
    score += log((total_spam + SMOOTH) / (total_ham + SMOOTH));
    score = logist(score / SCALE);
    return score;
}

inline double logist(double x)
{
    return 1.0 / (1.0 + exp(-x));
}

set<int> transform(string emailContent, TCHDB *featureSpace)
{
    set<int> featurePosition;
    int len = emailContent.length();
    int i = 0;
	char* value;
	
    for (i = 0; i <= len - NGRAM; i++)
    {
        string feature = emailContent.substr(i, NGRAM);
		value = tchdbget2(featureSpace, feature.c_str());
        if (value)
        {
            featurePosition.insert(atoi(value));
        }
        else
        {
            featurePosition.insert(dimension);
			value = new char[20]; //this is important, because it is NULL now
			sprintf(value, "%d", dimension);
			tchdbput2(featureSpace, feature.c_str(), value);
            dimension++;
        }
    }
    return featurePosition;
}

TCHDB* getTCHDB(string tableName)
{
	TCHDB* hdb;
	int ecode;
	
	/* create the object */
	hdb = tchdbnew();
	
	/* open the database */
	if(!tchdbopen(hdb, tableName.c_str(), HDBOWRITER | HDBOCREAT))
	{
		ecode = tchdbecode(hdb);
		fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
		return NULL;
	}
	
	return hdb;
}

int main()
{

    clock_t begin = clock();
    prepare();

	TCHDB *featureSpace;
	
	double score;
	ifstream email,index;
	ofstream prediction;
	string path = string(PATH);
	string corpus = string(CORPUS);
	string emailType, fname, predictType;
	vector<string> cases;
	set<int> features;
	
	unsigned i = 0, pos = 0;
	for( i = 0; i < corpus.length(); i++ )
	{	
		if( corpus[i] == ' ' )
		{
			cases.push_back(corpus.substr(pos,i-pos));
			pos = i + 1;
		}
	}
	cases.push_back(corpus.substr(pos,i-pos));
	
	for( vector<string>::iterator iter = cases.begin(); iter != cases.end(); iter++ )
	{
		string data_path = path + "corpus/" + (*iter) + "/";
		index.open((data_path + "index").c_str());
		prediction.open((data_path + "prediction").c_str());
		featureSpace = getTCHDB("tcht_"+(*iter));
		while( index >> emailType >> fname )
		{
			email.open((data_path+fname).c_str());
			ostringstream tmp;
			tmp << email.rdbuf();
			features = transform(tmp.str(), featureSpace);
			score = predict(features);
			predictType = (score > 0.5)? "spam" : "ham";
			prediction << fname << " judge=" << emailType << " class=" << predictType << " score=" << score << endl;
			train(features, emailType);		
			email.close();
			features.clear();
		}
		index.close();
		prediction.close();
		tchdbclose(featureSpace);
		tchdbdel(featureSpace);
		
		cout << "Dimesion: " << dimension << endl;
		dimension = 0;
	}

    clock_t end = clock();
    cout << endl << "Time: " << end - begin << endl;

    return EXIT_SUCCESS;
}
