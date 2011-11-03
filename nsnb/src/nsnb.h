#include <iostream>
#include <map>
#include <set>
#include <cmath>
#include <ctime>
#include <string>
#include <tcutil.h>
#include <tchdb.h>
#include <cstdlib>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

using namespace std;
#ifndef NSNB_H_
#define NSNB_H_

#define MAX_READ_LENGTH (3000)
#define NGRAM (5)
#define THICKNESS (0.25)
#define LEARNING_RATE (0.65)
#define SCALE (5000)
#define SMOOTH (1e-5)

struct feature {
	int ham;
	int spam;
	double confidence;
};

static int total_spam = 0;
static int total_ham = 0;
// 一块开辟好的缓冲区
static void* buffer = malloc(sizeof(struct feature));
static const struct feature new_spam_feature = { 0, 1, 1.0 };
static const struct feature new_ham_feature = { 1, 0, 1.0 };

static inline double logist(double x);
static void train_cell(set<string> &featureSet, string emailType,
		TCHDB* featureMap);

// nsnb module 对外可见的部份
extern set<string> transform(string emailContent);
extern double nsnb_predict(set<string> &featureSet, TCHDB* featureMap);
extern void nsnb_train(set<string> &featureSet, string emailType, TCHDB* featureMap);

#endif
