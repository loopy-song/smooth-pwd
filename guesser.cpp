/*
 * guesser.cpp
 * Copyright (c) 2021 Yuanming Song
 */

#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include "backoff.hpp"
#include "kneserNey.hpp"

using std::vector;
using std::string;
using std::unique_ptr;
using std::cout;
using std::endl;

int main(int argc, char *argv[]) {
	// example: ./guesser ../data/phpbb_train.txt ../result.txt  10000000 kneserney 8
	std::ios::sync_with_stdio(false);
	if (argc < 6) {
		cout << "too few arguments!" << endl;
		cout << "Expected: guesser train_path output_path guess_num model_name model_arg" << endl;
		return -1;
	}
	string train_path(argv[1]);
	string output_path(argv[2]);
	long long guess_num = atoll(argv[3]);
	string model_name(argv[4]); // "kneserney" or "backoff"
	int model_arg = atoi(argv[5]);

	unique_ptr<smoothPwd::BaseTrieModel> model;
	if (model_name == "backoff") {
		model = unique_ptr<smoothPwd::BaseTrieModel>(new smoothPwd::KatzBackoffModel(model_arg));
		cout << "Katz backoff model, threshold: " << model_arg << endl;
	}
	else { // kneserney by default
		model = unique_ptr<smoothPwd::BaseTrieModel>(new smoothPwd::ModifiedKneserNeyModel(model_arg));
		cout << "Modified Kneser-Ney model, gram size: " << model_arg << endl;
	}

	{
		vector<string> train_data;
		std::ifstream ftr(train_path);
		string line;
		while (std::getline(ftr, line)) {
			train_data.push_back(line);
		}
		clock_t tr_clock = clock();
		model->train(train_data);
		cout << "training size: " << train_data.size()
			<< " time: " << (double)(clock() - tr_clock) / CLOCKS_PER_SEC << endl;
	}

	clock_t ts_clock = clock();
	auto guesses = model->generate(guess_num, false);
	cout << "generated " << guesses.size() << " guesses, time: "
		<< (double)(clock() - ts_clock) / CLOCKS_PER_SEC << endl;

	std::ofstream fout(output_path);
	for (const auto& n : guesses) {
		fout << n.first << '\n';
	}
	return 0;
}
