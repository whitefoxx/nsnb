#include "nsnb.h"

void nsnb_train(set<string> &featureSet, string emailType, TCHDB* featureMap) {
	int rcode;
	double score = nsnb_predict(featureSet, featureMap);

	if (emailType == "ham")
		total_ham++;
	else
		total_spam++;

	while (emailType == "spam" && score < 0.5 + THICKNESS) {
		train_cell(featureSet, emailType, featureMap);
		for (set<string>::iterator iter = featureSet.begin(); iter
				!= featureSet.end(); iter++) {
			string str = *iter;
			rcode = tchdbget3(featureMap, str.c_str(), str.length(), buffer,
					sizeof(struct feature));
			struct feature* tmp = (struct feature*) buffer;
			tmp->confidence /= LEARNING_RATE;
			tchdbput(featureMap, str.c_str(), str.length(), tmp,
					sizeof(struct feature));
		}
		total_spam--;
		score = nsnb_predict(featureSet, featureMap);
	}
	while (emailType == "ham" && score > 0.5 - THICKNESS) {
		train_cell(featureSet, emailType, featureMap);
		for (set<string>::iterator iter = featureSet.begin(); iter
				!= featureSet.end(); iter++) {
			string str = *iter;
			rcode = tchdbget3(featureMap, str.c_str(), str.length(), buffer,
					sizeof(struct feature));
			struct feature* tmp = (struct feature*) buffer;
			tmp->confidence *= LEARNING_RATE;
			tchdbput(featureMap, str.c_str(), str.length(), tmp,
					sizeof(struct feature));
		}
		total_ham--;
		score = nsnb_predict(featureSet, featureMap);
	}
}

static void train_cell(set<string> &featureSet, string emailType, TCHDB* featureMap) {
	if (emailType == "spam") {
		total_spam++;
		for (set<string>::iterator iter = featureSet.begin(); iter
				!= featureSet.end(); iter++) {
			string str = *iter;
			int rcode = tchdbget3(featureMap, str.c_str(), str.length(),
					buffer, sizeof(struct feature));
			if (rcode == -1) {
				tchdbput(featureMap, str.c_str(), str.length(),
						&new_spam_feature, sizeof(struct feature));
			} else {
				struct feature* tmp = (struct feature*) buffer;
				tmp->spam += 1;
				tchdbput(featureMap, str.c_str(), str.length(), tmp,
						sizeof(struct feature));
			}
		}
	} else {
		total_ham++;
		for (set<string>::iterator iter = featureSet.begin(); iter
				!= featureSet.end(); iter++) {
			string str = *iter;
			int rcode = tchdbget3(featureMap, str.c_str(), str.length(),
					buffer, sizeof(struct feature));
			if (rcode == -1) {
				tchdbput(featureMap, str.c_str(), str.length(),
						&new_ham_feature, sizeof(struct feature));
			} else {
				struct feature* tmp = (struct feature*) buffer;
				tmp->ham += 1;
				tchdbput(featureMap, str.c_str(), str.length(), tmp,
						sizeof(struct feature));
			}
		}
	}
}

double nsnb_predict(set<string> &featureSet, TCHDB* featureMap) {
	int rcode;

	double score = 0.0;
	int s, h;
	double c;
	for (set<string>::iterator iter = featureSet.begin(); iter
			!= featureSet.end(); iter++) {
		string str = *iter;
		rcode = tchdbget3(featureMap, str.c_str(), str.length(), buffer,
				sizeof(struct feature));
		if (rcode == -1)
			continue;
		struct feature* tmp = (struct feature*) buffer;
		s = tmp->spam;
		h = tmp->ham;
		c = tmp->confidence;
		score += log((s + SMOOTH) / (h + SMOOTH) * (total_ham + 2 * SMOOTH)
				/ (total_spam + 2 * SMOOTH) * c);
	}
	score += log((total_spam + SMOOTH) / (total_ham + SMOOTH));
	score = logist(score / SCALE);
	return score;
}

static inline double logist(double x) {
	return 1.0 / (1.0 + exp(-x));
}

set<string> transform(string emailContent) {
	set<string> featureSet;
	int len = emailContent.length();
	int i = 0;
	for (i = 0; i <= len - NGRAM; i++) {
		string feature = emailContent.substr(i, NGRAM);
		featureSet.insert(feature);
	}
	return featureSet;
}

