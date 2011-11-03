#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <cmath>
#include <ctime>
#include <string>
#include <fstream>

// #include "config.h"
#include "nsnb.h"

void train(set<string> featureSet, string emailType, map<string, struct feature> &featureMap)
{
    double score = predict(featureSet, featureMap);
    
    if (emailType == "ham") 
        total_ham++;
    else 
        total_spam++;
    
    while (emailType == "spam" && score < 0.5 + THICKNESS)
    {
        train_cell(featureSet, emailType, featureMap);
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++)
            featureMap[*iter].confidence /= LEARNING_RATE;    
        total_spam--;
        score = predict(featureSet, featureMap);
    }
    while (emailType == "ham" && score > 0.5 - THICKNESS)
    {
        train_cell(featureSet, emailType, featureMap);
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++)
            featureMap[*iter].confidence *= LEARNING_RATE;
        total_ham--;
        score = predict(featureSet, featureMap);
    }
}

// void prepare()
// {
//     int i = 0;
//     for (i = 0; i < MAX_DIMENSION; i++)
//         confidence[i] = 1.0;
// }

void train_cell(set<string> featureSet, string emailType, map<string, struct feature> &featureMap)
{
    if (emailType == "spam")
    {
        total_spam++;
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++) {
            if (featureMap.count(*iter) != 0)
                featureMap[*iter].spam += 1;
            else {
                struct feature tmp = {0, 1, 1.0};
                featureMap[*iter] = tmp;
            }
        }
    }
    else
    {
        total_ham++;
        for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++) {
            if (featureMap.count(*iter) != 0)
                featureMap[*iter].ham += 1;
            else {
                struct feature tmp = {1, 0, 1.0};
                featureMap[*iter] = tmp;
            }
        }
    }
}

double predict(set<string> featureSet, map<string, struct feature> &featureMap)
{
    double score = 0.0;
    int s, h;
    double c;
    for (set<string>::iterator iter = featureSet.begin(); iter != featureSet.end(); iter++)
    {
        if (featureMap.count(*iter) == 0) 
            continue;
        struct feature* tmp = &featureMap[*iter];
        s = tmp->spam;
        h = tmp->ham;
        c = tmp->confidence;
        score += log((s + SMOOTH) / (h + SMOOTH) * (total_ham + 2 * SMOOTH) / (total_spam + 2 * SMOOTH) * c);
    }
    score += log((total_spam + SMOOTH) / (total_ham + SMOOTH));
    score = logist(score / SCALE);
    return score;
}

inline double logist(double x)
{
    return 1.0 / (1.0 + exp(-x));
}

set<string> transform(string emailContent)
{
    // cout << emailContent.length() << endl;
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

int main()
{

    clock_t begin = clock();
    map<string, struct feature> featureMap;
    set<string> featureSet;
    double score;
    string prediction;
    
    string corpusPath = "/Users/freiz/Corpus/spamassassin/";
    string indexPath = corpusPath + "index";
    ifstream in;
    ifstream tmp;
    char buff[MAX_READ_LENGTH];
    in.open(indexPath.c_str());
    string emailType, emailPath;
    while (in >> emailType >> emailPath) {
        memset(buff, 0, MAX_READ_LENGTH);
        emailPath = corpusPath + emailPath;
        tmp.open(emailPath.c_str());
        tmp.read(buff, MAX_READ_LENGTH);
        tmp.close();
        featureSet = transform(string(buff));
        score = predict(featureSet, featureMap);
        // cout << score << endl;
        train(featureSet, emailType, featureMap);
        // cout << "Now: " << count << " Path: " << emailPath << " Score: " << score << endl;
        if (score > 0.618)
            prediction = "spam";
        else
            prediction = "ham";
        cout << emailPath << " judge=" << emailType << " class=" << prediction << " score=" << score << endl;
    }
    
    in.close();
    
    // string test = "Haha~This is a test, It is a good day";
    // map<string, feature> featureMap;
    // set<string> featureSet = transform(test);
    // cout << "Predicting: " << predict(featureSet, featureMap) << endl;
    // train(featureSet, "spam", featureMap);
    // cout << "\nAfter training...\n\nPredicting: " << predict(featureSet, featureMap) << endl;
    // featureSet.clear();
    
    // prepare();

      // map<string, struct feature> featureMap;
      //     double score;
      //     ifstream email,index;
      //     ofstream prediction;
      //     string path = string(PATH);
      //     string corpus = string(CORPUS);
      //     string emailType, fname, predictType;
      //     vector<string> cases;
      //     set<int> features;
      //     
      //     unsigned i = 0, pos = 0;
      //     for( i = 0; i < corpus.length(); i++ )
      //     {    
      //      if( corpus[i] == ' ' )
      //      {
      //          cases.push_back(corpus.substr(pos,i-pos));
      //          pos = i + 1;
      //      }
      //     }
      //     cases.push_back(corpus.substr(pos,i-pos));
      //     
      //     for( vector<string>::iterator iter = cases.begin(); iter != cases.end(); iter++ )
      //     {
      //      string data_path = path + "corpus/" + (*iter) + "/";
      //      index.open((data_path + "index").c_str());
      //      prediction.open((data_path + "prediction").c_str());
      //      while( index >> emailType >> fname )
      //      {
      //          email.open((data_path+fname).c_str());
      //          ostringstream tmp;
      //          tmp << email.rdbuf();
      //          features = transform(tmp.str(), featureSpace);
      //          score = predict(features);
      //          predictType = (score > 0.5)? "spam" : "ham";
      //          prediction << fname << " judge=" << emailType << " class=" << predictType << " score=" << score << endl;
      //          train(features, emailType);     
      //          email.close();
      //          features.clear();
      //      }
      //      index.close();
      //      prediction.close();
      //      featureSpace.clear();
      //     }

    clock_t end = clock();
    // cout << endl << "Time: " << end - begin << endl;

    return EXIT_SUCCESS;
}
