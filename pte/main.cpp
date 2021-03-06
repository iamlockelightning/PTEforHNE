#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <gsl/gsl_rng.h>
#include "linelib.h"

char nodes_file[MAX_STRING], words_file[MAX_STRING], en_hin_file[MAX_STRING], output_file[MAX_STRING],
		zh_hin_file[MAX_STRING], en_link_file[MAX_STRING], zh_link_file[MAX_STRING], cl_hin_file[MAX_STRING];
int binary = 0, num_threads = 1, vector_size = 100, negative = 5;
long long samples = 1, edge_count_actual;

real alpha = 0.025, starting_alpha;

real learning_rate_1 = 0.01, lambda_1 = 1.0, MARGIN = 1.0;
real learning_rate_2 = 0.01, lambda_2 = 1.0;
int L1_flag = 0, transfer_flag = 0;

// learning_rate {0.001, 0.01, 0.1}
// lambda {1, 2.5, 5, 7.5}
// L1 {1(L1), 0(L2)}
// vector_size {50, 75, 100, 125}



const gsl_rng_type * gsl_T;
gsl_rng * gsl_r;

line_node nodes, words;
line_hin en_text_hin, zh_text_hin, en_link_hin, zh_link_hin, cl_hin;
line_trainer trainer_w_en, trainer_w_zh, trainer_l_en, trainer_l_zh, trainer_c;
int NETWORK_NUM = 5;

double func_rand_num()
{
	return gsl_rng_uniform(gsl_r);
}

void *TrainModelThread(void *id) 
{
	long long edge_count = 0, last_edge_count = 0;
	unsigned long long next_random = (long long)id;
	real *error_vec = (real *)calloc(vector_size, sizeof(real));
	real res = 0;
	int epo = 0;
	while (1)
	{
		if (edge_count > samples / num_threads + 2) break;

		if (edge_count - last_edge_count>10000)
		{
			edge_count_actual += edge_count - last_edge_count;
			last_edge_count = edge_count;
			printf("%cAlpha: %f Progress: %.3lf%%", 13, alpha, (real)edge_count_actual / (real)(samples + 1) * 100);
			fflush(stdout);
			alpha = starting_alpha * (1 - edge_count_actual / (real)(samples + 1));
			if (alpha < starting_alpha * 0.0001) alpha = starting_alpha * 0.0001;
		}
		if (id == 0) {
			printf("Epoch %i in thread #%lld:\nLoss: ", ++epo, (long long)id);
		}
		real total_loss = 0.0;
		// Modified
		for (int i=0; i!=NETWORK_NUM; i+=1) {
			switch(i) {
				case 0:
					trainer_w_en.train_sample(alpha, error_vec, func_rand_num, next_random, (long long)id);
					if (id == 0) {printf("w_en: %lf +", error_vec[0]); total_loss+=error_vec[0];}
					break;
				case 1:
					trainer_w_zh.train_sample(alpha, error_vec, func_rand_num, next_random, (long long)id);
					if (id == 0) {printf("w_zh: %lf +", error_vec[0]); total_loss+=error_vec[0];}
					break;
				case 2:
					trainer_l_en.train_sample(alpha, error_vec, func_rand_num, next_random, (long long)id);
					if (id == 0) {printf("l_en: %lf +", error_vec[0]); total_loss+=error_vec[0];}
					break;
				case 3:
					trainer_l_zh.train_sample(alpha, error_vec, func_rand_num, next_random, (long long)id);
					if (id == 0) {printf("l_zh: %lf +", error_vec[0]); total_loss+=error_vec[0];}
					break;
				case 4:
					// trainer_c.train_transE_sample(res, lambda_1, learning_rate_1, L1_flag, MARGIN, next_random, (long long)id);
					trainer_c.train_intersect_sample(res, lambda_2, learning_rate_2, L1_flag, next_random, (long long)id);
					if (id == 0) {total_loss+=res; printf("transM: %lf = %lf", res, total_loss);}
					break;
				default:
					break;
			}
		}
		// break;
		edge_count += NETWORK_NUM;
	}
	free(error_vec);
	pthread_exit(NULL);
}

void TrainModel() {
	long a;
	pthread_t *pt = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
	starting_alpha = alpha;

	// nodes.init(nodes_file, vector_size);
	words.init(words_file, vector_size);
	en_text_hin.init(en_hin_file, &words, &words);
	zh_text_hin.init(zh_hin_file, &words, &words);
	en_link_hin.init(en_link_file, &words, &words);
	zh_link_hin.init(zh_link_file, &words, &words);
	cl_hin.init(cl_hin_file, &words, &words); // cl_hin.init(cl_hin_file, &nodes, &words);

	trainer_w_en.init('w', &en_text_hin, negative);
	trainer_w_zh.init('w', &zh_text_hin, negative);
	trainer_l_en.init('l', &en_link_hin, negative);
	trainer_l_zh.init('l', &zh_link_hin, negative);
	trainer_c.init('c', &cl_hin, negative);

	clock_t start = clock();
	printf("Training process:\n");
	for (a = 0; a < num_threads; a++) pthread_create(&pt[a], NULL, TrainModelThread, (void *)a);
	for (a = 0; a < num_threads; a++) pthread_join(pt[a], NULL);
	printf("\n");
	clock_t finish = clock();
	printf("Total time: %lf\n", (double)(finish - start) / CLOCKS_PER_SEC);

	words.output(output_file, binary);
	if (transfer_flag) {
		strcat(output_file, (char*)".transferM.txt");
		trainer_c.save_transfer(output_file);
	}
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
		printf("PTE: Predictive Text Embedding\n\n");
		printf("Options:\n");
		printf("Parameters for training:\n");
		printf("\t-nodes <file>\n");
		printf("\t\tThe node set\n");
		printf("\t-words <file>\n");
		printf("\t\tThe word set\n");
		printf("\t-hin <file>\n");
		printf("\t\tThe file of the text HIN\n");
		printf("\t-hin <file>\n");
		printf("\t\tSave word embedding into <file>\n");
		printf("\t-binary <int>\n");
		printf("\t\tSave the resulting vectors in binary moded; default is 0 (off)\n");
		printf("\t-size <int>\n");
		printf("\t\tSet size of word vectors; default is 100\n");
		printf("\t-negative <int>\n");
		printf("\t\tNumber of negative examples; default is 5, common values are 5 - 10 (0 = not used)\n");
		printf("\t-samples <int>\n");
		printf("\t\tSet the number of training samples as <int>Million\n");
		printf("\t-threads <int>\n");
		printf("\t\tUse <int> threads (default 1)\n");
		printf("\t-alpha <float>\n");
		printf("\t\tSet the starting learning rate; default is 0.025\n");
		printf("\t-lr <float>\n");
		printf("\t\tSet the learning rate for transE; default is 0.01\n");
		printf("\t-MARGIN <float>\n");
		printf("\t\tSet the MARGIN for transE; default is 0.01\n");
		printf("\t-lambda <float>\n");
		printf("\t\tSet the balance factor; default is 1.0\n");
		printf("\nExamples:\n");
		printf("./pte -nodes nodes.txt -words words.txt -hin hin.txt -output vec.txt -binary 1 -size 200 -negative 5 -samples 100\n\n");
		return 0;
	}
	output_file[0] = 0;
	if ((i = ArgPos((char *)"-nodes", argc, argv)) > 0) strcpy(nodes_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-words", argc, argv)) > 0) strcpy(words_file, argv[i + 1]);

	if ((i = ArgPos((char *)"-enhin", argc, argv)) > 0) strcpy(en_hin_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-zhhin", argc, argv)) > 0) strcpy(zh_hin_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-enlinkhin", argc, argv)) > 0) strcpy(en_link_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-zhlinkhin", argc, argv)) > 0) strcpy(zh_link_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-clhin", argc, argv)) > 0) strcpy(cl_hin_file, argv[i + 1]);

	if ((i = ArgPos((char *)"-output", argc, argv)) > 0) strcpy(output_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-binary", argc, argv)) > 0) binary = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-size", argc, argv)) > 0) vector_size = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-negative", argc, argv)) > 0) negative = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-samples", argc, argv)) > 0) samples = atoi(argv[i + 1])*(long long)(1000000);
	if ((i = ArgPos((char *)"-alpha", argc, argv)) > 0) alpha = atof(argv[i + 1]);
	if ((i = ArgPos((char *)"-lr_1", argc, argv)) > 0) learning_rate_1 = atof(argv[i + 1]);
	if ((i = ArgPos((char *)"-lr_2", argc, argv)) > 0) learning_rate_2 = atof(argv[i + 1]);
	if ((i = ArgPos((char *)"-MARGIN", argc, argv)) > 0) MARGIN = atof(argv[i + 1]);
	if ((i = ArgPos((char *)"-lambda_1", argc, argv)) > 0) lambda_1 = atof(argv[i + 1]);
	if ((i = ArgPos((char *)"-lambda_2", argc, argv)) > 0) lambda_2 = atof(argv[i + 1]);
	if ((i = ArgPos((char *)"-L1_flag", argc, argv)) > 0) L1_flag = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-transfer_flag", argc, argv)) > 0) transfer_flag = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-threads", argc, argv)) > 0) num_threads = atoi(argv[i + 1]);

	printf("__________________\n");
	printf("size: %d\nnegative: %d\nsamples: %d\nalpha: %f\nlr_1: %f\nMARGIN: %f\nlambda_1: %f\nlr_2: %f\nlambda_2: %f\nL1_flag: %d\n", vector_size, negative, samples, alpha, learning_rate_1, MARGIN, lambda_1, learning_rate_2, lambda_2, L1_flag);
	printf("__________________\n");
	
	gsl_rng_env_setup();
	gsl_T = gsl_rng_rand48;
	gsl_r = gsl_rng_alloc(gsl_T);
	gsl_rng_set(gsl_r, 314159265);

	TrainModel();
	return 0;
}