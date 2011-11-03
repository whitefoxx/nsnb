#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <cmath>
#include <ctime>
#include <string>
#include <fstream>



#include "config.h"
#include "nsnb.h"
#include "feature.h"

void train(set<string> featureSet, string emailType, TCHDB* hdb)
{
    double score = predict(featureSet, hdb);
	//cout << "score = " << score << endl;
	void* vp;
	Feature* fp;
	
    if (emailType == "ham")
        total_ham++;
    else
        total_spam++;

    while (emailType == "spam" && score < 0.5 + THICKNESS)
    {
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++)
		{
			string fstr = (*iter);
			int siz = tchdbvsiz(hdb, fstr.c_str(), fstr.length());
			cout << siz << endl;	
			if(siz != -1)
				siz = tchdbget3(hdb, fstr.c_str(), fstr.length(), vp, siz);
			if( siz != -1 )
			{
			//	cout << "spam here" << endl;
				fp = (Feature*)vp;
				cout << siz << " "<< fp->confidence << endl;
				fp->confidence /= LEARNING_RATE;
			}
			else
			{
			//	cout << "vp == NULL" << endl;
			}
		}
		cout << "end for" << endl;
        train_cell(featureSet, emailType, hdb);
        total_spam--;
        score = predict(featureSet, hdb);
    }
    while (emailType == "ham" && score > 0.5 - THICKNESS)
    {
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++)
		{
			string fstr = (*iter);
			int siz = tchdbvsiz(hdb, fstr.c_str(), fstr.length());	
			if(siz != -1)
				siz = tchdbget3(hdb, fstr.c_str(), fstr.length(), vp, siz);
			if( siz != -1 )
			{
				fp = (Feature*)vp;
				cout << siz << " "<< fp->confidence << endl;
				fp->confidence *= LEARNING_RATE;
			}
		}
        train_cell(featureSet, emailType, hdb);
        total_spam--;
        score = predict(featureSet, hdb);
    }
}

/*
void prepare()
{
    int i = 0;
    for (i = 0; i < MAX_DIMENSION; i++)
        confidence[i] = 1.0;
}
*/

void train_cell(set<string> featureSet, string emailType, TCHDB* hdb)
{
	void* vp;
	Feature* fp;
	int* sp;
	cout << "train_cell" << endl;
    if (emailType == "spam")
    {
        total_spam++;
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++)
		{
			string fstr = (*iter);
			vp = tchdbget(hdb, fstr.c_str(), fstr.length(), sp);
			if( vp != NULL )
			{
				fp = (Feature*)vp;
				fp->spam += 1;
			
				//cout << "spam" << endl;
			}
		}
    }
    else
    {
        total_ham++;
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++)
		{
			string fstr = (*iter);
			vp = tchdbget(hdb, fstr.c_str(), fstr.length(), sp);
			if( vp != NULL )
			{
				fp = (Feature*)vp;
				fp->ham += 1;
				//cout << "ham" << endl;
			}
		}
    }
}

double predict(set<string> featureSet, TCHDB *hdb)
{
	int s,h;
	double cf;
	double score = 0.0;
	void* vp;
	Feature* fp;
	set<string>::iterator iter;
	
	for (iter = featureSet.begin(); iter != featureSet.end(); iter++)
    {	
		string fstr = (*iter);
		int siz = tchdbvsiz(hdb, fstr.c_str(), fstr.length());	
		if(siz != -1)
			siz = tchdbget3(hdb, fstr.c_str(), fstr.length(), vp, siz);
		if( siz == -1 )
		{
			Feature ft = Feature(fstr, 0, 0, 1.0);
			if( !tchdbputkeep(hdb, fstr.c_str(), fstr.length(), &ft, sizeof(ft)) )
				cout << "err in predict!" << endl;
		}
		else
		{
			fp = (Feature*)vp;
			s = fp->spam;
			h = fp->ham;
			cf = fp->confidence;
			score += log((s + SMOOTH) / (h + SMOOTH) * (total_ham + 2 * SMOOTH) / (total_spam + 2 * SMOOTH) * cf);
		}
	}
	score += log((total_spam + SMOOTH) / (total_ham + SMOOTH));
    score = logist(score / SCALE);

	cout << score << endl;
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
    //prepare();

	TCHDB* hdb;
	
	double score;
	ifstream email,index;
	ofstream prediction;
	string path = string(PATH);
	string corpus = string(CORPUS);
	string emailType, fname, predictType;
	vector<string> cases;
	set<string> featureSet;
	
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
		hdb = getTCHDB("tchdb_" + (*iter));
		while( index >> emailType >> fname )
		{
			email.open((data_path+fname).c_str());
			ostringstream content;
			content << email.rdbuf();
			featureSet = transform(content.str());
			//cout << "transform done" << endl;
			score = predict(featureSet, hdb);
			predictType = (score > 0.5)? "spam" : "ham";
			prediction << fname << " judge=" << emailType << " class=" << predictType << " score=" << score << endl;			
			train(featureSet, emailType, hdb);
			//cout << "train done" << endl;		
			email.close();
			featureSet.clear();
		}
		index.close();
		prediction.close();
		tchdbclose(hdb);
		tchdbdel(hdb);
	}

    clock_t end = clock();
    cout << endl << "Time: " << end - begin << endl;

    return EXIT_SUCCESS;
}
