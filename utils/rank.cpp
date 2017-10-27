#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <string.h>
#include <map>
#include <vector>
#include <algorithm>

using namespace std;

#define MAX_STRING 100000

typedef float real;                    // Precision of float numbers

char input_file[MAX_STRING], pair_file[MAX_STRING];
long long vector_dim;

map<string, real*> embedding_dict;
vector<pair<string, string> > cl_pairs;

void ReadEmbedding() {
	long long num_vertices, a, b;
	char name[MAX_STRING], ch;
	real *vec;

	FILE *fi;
	fi = fopen(input_file, "rb");
	if (fi == NULL) {
		printf("Vector file 1 not found\n");
		exit(1);
	}

	fscanf(fi, "%lld %lld", &num_vertices, &vector_dim);
	for (a = 0; a < num_vertices; a++) {
		vec = (real *)malloc(vector_dim * sizeof(real));
		fscanf(fi, "%s ", name);
		for (b = 0; b < vector_dim; b++) fscanf(fi, "%f ", &vec[b]);
		string nam(name);
		embedding_dict.insert(map<string, real*>::value_type(nam, vec));
	}
	printf("embedding_dict: %lu\n", embedding_dict.size());
	fclose(fi);
}

void ReadPairs() {
	long long num_pairs, a;
	char head[MAX_STRING], tail[MAX_STRING], str[MAX_STRING], tp;
	real wei;
	FILE *fi;
	fi = fopen(pair_file, "rb");
	num_pairs = 0;
	while (fgets(str, sizeof(str), fi)) num_pairs++;
	fclose(fi);
	
	fi = fopen(pair_file, "rb");
	for (a = 0; a < num_pairs; a++) {
		fscanf(fi, "%s\t%s\t%f\t%c", head, tail, &wei, &tp);
		map<string, real*>::iterator h = embedding_dict.find(head);
        map<string, real*>::iterator e = embedding_dict.find(tail);
        if (h!=embedding_dict.end() && e!=embedding_dict.end()) {
			cl_pairs.push_back(make_pair(head, tail));
		}
	}
	printf("cl_pairs: %lu\n", cl_pairs.size());
}

real L1norm (real *a, real *b) {
	real ans = 0;
	for (long long i = 0; i < vector_dim; i += 1) {
		ans += fabs(a[i] - b[i]);
	}
	return ans;
}
real L2norm (real *a, real *b) {
	real ans = 0;
	for (long long i = 0; i < vector_dim; i += 1) {
		ans += sqrt((a[i] - b[i])*(a[i] - b[i]));
	}
	return ans;
}
real cos_dist(real *a, real *b) {
	real dotProduct = 0;
	real normA = 0;
	real normB = 0;
    for (int i = 0; i < vector_dim; i += 1) {
        dotProduct += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }   
    return dotProduct/(sqrt(normA) * sqrt(normB));
}

bool cmp_pair (pair<int, real> a, pair<int, real> b) {	// low -> high
	if (a.second < b.second) {
		return true;
	} else {
		return false;
	}
}

void Rank() {
	ReadEmbedding();
	ReadPairs();

	int mid = cl_pairs.size() / 2;

	real *avg_diff_vec = (real *)malloc(vector_dim * sizeof(real)),
			*vec_a = (real *)malloc(vector_dim * sizeof(real));

	for (int d = 0; d < vector_dim; d++) avg_diff_vec[d] = 0;
	for (int i = 0; i < mid; i += 1) {
		if (i%1000==0) {
			printf("__%d\n", i);
		}
        map<string, real*>::iterator h = embedding_dict.find(cl_pairs[i].first);
        map<string, real*>::iterator e = embedding_dict.find(cl_pairs[i].second);
        if (h!=embedding_dict.end() && e!=embedding_dict.end()) {
        	for (int d = 0; d < vector_dim; d++) avg_diff_vec[d] += ((*e).second[d] - (*h).second[d]);
        } else {
        	cout << "Something wrong in Rank()... \n";
        }
    }
    for (int d = 0; d < vector_dim; d++) avg_diff_vec[d] /= mid;

	vector<int> ranks;
	for (int i = mid; i < cl_pairs.size(); i += 1) {
		if (i%1000==0) {
			printf("__%d\n", i);
		}
		int r = -1;
		vector<pair<int, real> > scores;
        for (int j = mid; j < cl_pairs.size(); j += 1) {
        	map<string, real*>::iterator h = embedding_dict.find(cl_pairs[i].first);
        	map<string, real*>::iterator e = embedding_dict.find(cl_pairs[j].second);
        	if (h!=embedding_dict.end() && e!=embedding_dict.end()) {
        		for (int d = 0; d < vector_dim; d++) vec_a[d] = avg_diff_vec[d] + (*h).second[d];
        		scores.push_back(make_pair(j, cos_dist(vec_a, (*e).second)));
        	}
        }
        sort(scores.begin(), scores.end(), cmp_pair);
        for (int j = 0; j < scores.size(); j += 1) {
        	if (scores[j].first == i) {
        		r = j;
        	}
        }
        if (r == -1) {
        	printf("Something wrong.... in Rank()");
        }
        ranks.push_back(r);
    }

    real mean_rank = 0.0;
    for(std::vector<int>::iterator it = ranks.begin(); it != ranks.end(); it += 1) {
		mean_rank += *it;
    }
    mean_rank /= ranks.size();

    real hits_at_10 = 0.0;
    for(std::vector<int>::iterator it = ranks.begin(); it != ranks.end(); it += 1) {
		if ((*it) < 10) {
			hits_at_10 += 1.0;
		}
    }
    hits_at_10 /= ranks.size();

    printf("Mean Rank: %f, Hits@10: %f\n", mean_rank, hits_at_10);
    
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

int main(int argc, char **argv) {
	int i;
	if (argc == 1) {
		printf("Rank\n\n");
		printf("Options:\n");
		printf("Parameters for training:\n");
		printf("\t-input <file>\n");
		printf("\t\tThe original vertex embeddings\n");
		printf("\t-pairs <file>\n");
		printf("\t\tUse <file> to save the normalized vertex embeddings\n");
		printf("\nExamples:\n");
		printf("./rank -input vec_all.txt -pair cl.test.40000.net\n\n");
		return 0;
	}
	if ((i = ArgPos((char *)"-input", argc, argv)) > 0) strcpy(input_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-pair", argc, argv)) > 0) strcpy(pair_file, argv[i + 1]);
	Rank();
	return 0;
}
