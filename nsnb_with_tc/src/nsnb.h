#ifndef NSNB_H
#define NSNB_H

#include <iostream>
#include <map>
#include <set>
#include <cmath>
#include <ctime>
#include <string>
#include <tcutil.h>
#include <tchdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

using namespace std;

#ifndef MAX_READ_LENGTH
#define MAX_READ_LENGTH (3000)      // 每封邮件最多读入的字节数
#endif
#ifndef NGRAM
#define NGRAM (5)                   // NGRAME 分词的窗口宽度
#endif
#ifndef THICKNESS
#define THICKNESS (0.25)            // 厚阈值法参数
#endif
#ifndef LEARNING_RATE
#define LEARNING_RATE (0.65)
#endif
#ifndef SCALE
#define SCALE (2750)
#endif
#ifndef SMOOTH
#define SMOOTH (1e-5)
#endif
#ifndef HEADER_WEIGTH
#define HEADER_WEIGTH (0.45)        // 目前这个版本暂时用不到 
#endif
#ifndef THRESHOLD					// 仿照nsnb_server.py
#define THRESHOLD (0.50)
#endif
#ifndef ACTIVE_TRADEOFF
#define ACTIVE_TRADEOFF (0.25)
#endif

struct feature {
   // string content;
   int ham;
   int spam;
   double confidence;
};

// #ifndef MAX_DIMENSION
// #define MAX_DIMENSION (10000000)
// #endif

// 这两个参数用来估计先验概率
int total_spam = 0;                 // 垃圾邮件总数
int total_ham = 0;                  // 正常邮件总数

//int spam_words[MAX_DIMENSION];
//int ham_words[MAX_DIMENSION];
//double confidence[MAX_DIMENSION];

TCHDB* featureMap;

inline double logist(double x);     // logist 函数，用于 Result Scale，配合 Scale 参数一起使用
// 将字符串转换为 特征对应的维度的集合，自习体会一下，结合上面最关键的几个数组使用
set<string> transform(string emailContent);
// 作用类似于构造函数，需要进行初始化
//void prepare();
// Train 函数里面会反复的调用

// void train_cell(set<string> featureSet, string emailType, TCHDB* hdb);
// double predict(set<string> featureSet, TCHDB* hdb);
// void train(set<string> featureSet, string emailType, TCHDB* hdb);

void train_cell(set<string> featureSet, string emailType, TCHDB* featureMap);
double predict(set<string> featureSet, TCHDB* featureMap);
void train(set<string> featureSet, string emailType, TCHDB* featureMap);

// 仿照nsnb_server.py
void init();
void predict_a_mail(char *path, char *tag);
void predict_without_train(char *path, char *prediction, double *score);

#endif
