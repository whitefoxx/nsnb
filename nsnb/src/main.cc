#include <time.h>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cstdlib>

#include "nsnb.h"
#define CORPUSPATH "/home/cyb/data/spamassassin_corpus/"

int main(int argc, char **argv) {
	clock_t begin = clock();

	TCHDB* featureMap = tchdbnew();
	// tchdbsetxmsiz(featureMap, 67108864);
	if (!tchdbopen(featureMap, "db/featureMap.tch", HDBOWRITER | HDBOCREAT)) {
		int ecode = tchdbecode(featureMap);
		fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
	}

	string corpusPath = CORPUSPATH; 
	string indexPath = corpusPath + "index";
	ifstream index;
	char buff[MAX_READ_LENGTH];
	index.open(indexPath.c_str());
	string emailType, emailPath;
	while (index >> emailType >> emailPath) {
		memset(buff, 0, MAX_READ_LENGTH);
		emailPath = corpusPath + emailPath;
		ifstream email;
		email.open(emailPath.c_str());
		email.read(buff, MAX_READ_LENGTH);
		email.close();
		set<string> featureSet = transform(string(buff));
		double score = nsnb_predict(featureSet, featureMap);
		nsnb_train(featureSet, emailType, featureMap);
		string prediction;
		if (score > 0.618)
			prediction = "spam";
		else
			prediction = "ham";
		printf("%s judge=%-4s class=%-4s score=%.8f\n", emailPath.c_str(),
				emailType.c_str(), prediction.c_str(), score);
	}
	index.close();

	if (!tchdbclose(featureMap)) {
		int ecode = tchdbecode(featureMap);
		fprintf(stderr, "close error: %s\n", tchdberrmsg(ecode));
	}

	tchdbdel(featureMap);

	clock_t end = clock();
	fprintf(stdout, "Time: %.2f\n", (end - begin + 0.0) / CLOCKS_PER_SEC);

	free(buffer);
	return EXIT_SUCCESS;
}
