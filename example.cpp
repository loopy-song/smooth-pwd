/*
 * example.cpp
 * Copyright (c) 2021 Yuanming Song
 */

#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstring>
#include <ctime>

#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>

#include "kneserNey.hpp"
#include "backoff.hpp"

using std::ifstream;
using std::ofstream;
using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::unordered_map;
using namespace smoothPwd;

const string train_path("../data/phpbb_train.txt");
const string test_path("../data/phpbb_test.txt");
const string guess_path("../phpbb_kneserney_8_guess.txt");
const string result_path("../phpbb_kneserney_8_result.txt");

const bool output_guesses = false;

inline vector<string> read_in(const string path) {
	vector<string> data;
	ifstream fin(path);
	string line;
	while (std::getline(fin, line)) {
		data.push_back(line);
	}
	return data;
}

inline bool take_plot(ull x) {
	ull disc = (x >> 8);
	ull tar = 1;
	while (disc) {
		disc >>= 1;
		tar <<= 1;
	}
	return !(x & (tar - 1)) || ((x % 10000) == 0);
}

inline double elapsed_time(clock_t last_clock) {
	return (double)(clock() - last_clock) / CLOCKS_PER_SEC;
}

int main() {
	std::ios::sync_with_stdio(false);
	if (sizeof(size_t) < 8){
		cout << sizeof(size_t) << "\nYou should consider switching to 64-bit builds." << endl;
	}
	size_t test_size = 0;

	ModifiedKneserNeyModel model(8);
	// KatzBackoffModel model(5);
	{ // training example
		auto train_data = read_in(train_path);

		clock_t tr_clock = clock();
		model.train(train_data);
		std::cout << "training size: " << train_data.size() << " tr_time: " << elapsed_time(tr_clock) << endl;
	}

	// sanity check
	cout << model.pwd_prob("password") << endl;
	cout << model.pwd_prob("password123456") << endl;
	cout << model.pwd_prob("loopy-song@github.io") << endl;

	// sampling example
	vector<std::pair<string, double> > samples;
	int sample_num = 100000;
	double sample_avr = 0.0;
	clock_t sample_clock = clock();
	for (int i = 0; i < sample_num; i++) {
		auto sample = model.sample();
		sample_avr += sample.second;
		samples.push_back(sample);
	}
	sample_avr /= sample_num;
	cout << "tested " << sample_num << " samples, avr: " << sample_avr << " time: " << elapsed_time(sample_clock) << endl;

	// guessing example
	clock_t ts_clock = clock();
	//auto guesses = model.generate_by_threshold(1e-8);
	//auto guesses = model.generate((ull)1e6);
	auto guesses = model.generate_by_montecarlo((ull)1e6);
	cout << "generated " << guesses.size() << " guesses. time: " << elapsed_time(ts_clock) << endl;

	ull guessed_num = 0, cracked_num = 0;

	std::ifstream fts(test_path);
	unordered_map<string, ull> crack;
	string line;
	while (std::getline(fts, line)) {
		crack[line]++;
		++test_size;
	}

	std::ofstream fout(result_path);
	std::ofstream fguess;
	if (output_guesses) fguess.open(guess_path);

	for (const auto& n : guesses) {
		if (output_guesses) fguess << n.first << " " << n.second << endl;
		auto it = crack.find(n.first);
		if (it != crack.end()) {
			cracked_num += it->second;
			crack.erase(it);
		}
		++guessed_num;

		if (take_plot(guessed_num))
			fout << guessed_num << " " << cracked_num << endl;
		if ((guessed_num % 500000) == 0)
			cout << guessed_num << " " << cracked_num << endl;
	}

	cout << "guesses: " << guessed_num << " cracked: " << cracked_num
		<< " test_size: " << test_size << " fraction: " << (double)cracked_num / test_size << endl;

	//system("pause");
	return 0;
}
