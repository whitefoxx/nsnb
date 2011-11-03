#ifndef FEATURE_H
#define FEATURE_H

#include <string>
using namespace std;

class Feature
{
public:
	Feature(string s, int sp, int h, double cf);
	string fstr;
	int spam;
	int ham;
	double confidence;
};

#endif

