#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#define MAX_STRING 100000

char title_file[MAX_STRING], vector_file[MAX_STRING], output_file[MAX_STRING];
vector<string> titles;

void Process () {
	ifstream if_1(title_file);
	ifstream if_2(vector_file);
	ofstream of(output_file);
	string line;
	while(getline(if_1, line)) {
		cout << line;

	}
	if_1.close();
	if_2.close();
	of.close();
}

int ArgPos(char *str, int argc, char **argv) {
	int a;
	for (a = 1; a < argc; a++) if (!strcmp(str, argv[a])) {
		if (a == argc - 1) {
			printf("Argument missing for %s\n", str);
			exit(1);
		}
		return a;
	}
	return -1;
}
// ./combine -title ../WikiExtractor/etc/zh_title_40000_all.txt -vector ./workspace/zh.word.emb -output ./workspace/zh.title.text.emb
int main (int argc, char **argv) {
	int i;
    if (argc == 1) {
        return 0;
    }
    output_file[0] = 0;
    if ((i = ArgPos((char *)"-title", argc, argv)) > 0) strcpy(title_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-vector", argc, argv)) > 0) strcpy(vector_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-output", argc, argv)) > 0) strcpy(output_file, argv[i + 1]);
    Process();
    return 0;
}
