/*
 * baseTrie.hpp
 * Copyright (c) 2021 Yuanming Song
 */

#pragma once

#include <ctime>
#include <algorithm>
#include <random>
#include <tuple>
#include <memory>
#include <functional>
//#include <thread>

#include "common.hpp"
#include "baseNode.hpp"
#include "simpleTrie.hpp"

namespace smoothPwd
{
	class BaseTrieModel {
	private:
		std::unique_ptr<SimpleTrie> s_trie;

		inline size_t add_node(char c, int level, ull cnt, ull cnt_end, int bucket = 4) {
			tree.emplace_back(c, level, cnt, cnt_end, bucket);
			return (size_t)(tree.size() - 1);
		}

		void aggressive_prune();

		double ch_prob(size_t pred, char c, size_t &nt) const;

		void ch_search(size_t idx, std::string &s, const bset v, double p) const;

		std::tuple<char, double, size_t> sample_ch(size_t idx, const bset v, double rand_val) const;

		size_t add_from_trie(char cur_char, size_t idx, const ull prune = 0, const int level = 0);

	protected:
		std::vector<Node> tree;
		size_t root, start_idx;
		
		// global variables for threshold search; shall be adapted to parallel versions later
		std::function<void(std::string &, double)> oracle;
		double min_threshold, max_threshold;
		
		std::uniform_real_distribution<double> unif;
		std::mt19937 re;

		void get_fail();

		void build_trie(ull prune = 0); // wrapper for add_from_trie and get_fail :P

	public:
		const int gram_size;

		BaseTrieModel(int _gram_size = MAX_GRAM_SIZE) : root(0), start_idx(0), oracle(nullptr), min_threshold(1.0), max_threshold(1.0), unif(0.0, 1.0), re((unsigned int)time(nullptr)), gram_size(_gram_size) {
			re.discard(700000); // https://codereview.stackexchange.com/questions/109260/seed-stdmt19937-from-stdrandom-device
			s_trie = std::unique_ptr<SimpleTrie>(new SimpleTrie(gram_size));
		}

		void setOracle(const decltype(oracle)& _oracle) {
			oracle = _oracle;
		}

		inline void add(const char *s, ull cnt = 1) {
			s_trie->add_sub(s, cnt);
		}

		virtual void preprocess() = 0;

		void sanity_check();

		void train(const std::vector<std::string> &data) {
			std::unordered_map<std::string, ull> counter;
			for (const auto& s : data) {
				counter[s]++;
			}
			train(counter);
		}

		template <typename T>
		void train(const std::unordered_map<std::string, T> &data) {
			for (const auto& item : data) {
				add(item.first.c_str(), (ull)item.second);
			}
			preprocess();
			//sanity_check();
		}

		double pwd_prob(const char *s) const;

		double pwd_prob(const std::string& s) const {
			return pwd_prob(s.c_str());
		}

		StrProb sample();

		//StrProb sample_brute();

		void threshold_search(double min_thres, double max_thres = 1.0) {
			std::string s;
			min_threshold = min_thres; max_threshold = max_thres; // set global threshold
			ch_search(start_idx, s, empty_bset, 1.0); // the real search part
			min_threshold = max_threshold = 1.0; // clear threshold
		}

		// some wrappers below

		std::vector<StrProb> generate_by_threshold(double min_thres, double max_thres = 1.0) {
			std::vector<StrProb> guesses;
			oracle = [&guesses](std::string &s, double p) { guesses.emplace_back(s, p); };
			threshold_search(min_thres, max_thres);
			oracle = nullptr;
			std::sort(guesses.begin(), guesses.end(), [](const StrProb &a, const StrProb &b) { return a.second > b.second; });
			return guesses;   // hopefully the compiler would perform a move operation here
		}

		std::vector<StrProb> generate(ull cnt, bool strict = false);

		std::vector<StrProb> generate_by_montecarlo(ull cnt, size_t num_samples = 10000);

	};

	class PosEstimator {
		// ref: Dell'Amico and Filippone, "Monte Carlo Strength Evaluation:
		// Fast and Reliable Password Checking," CCS'15. https://github.com/matteodellamico/montecarlopwd
	public:
		std::vector<double> probs, ranks;
		const size_t N;

		PosEstimator(std::vector<StrProb>& samples): N(samples.size()) {
			for (const auto& item : samples) probs.push_back(item.second);
			sort(probs.begin(), probs.end(), std::greater<double>());
			ranks.push_back(0.0);
			for (size_t i = 0; i < N; i++) {
				ranks.push_back(ranks[i] + 1.0 / (N * probs[i]));
			}
		}

		double position(double prob) const {
			// 1. find pos s.t. probs[pos - 1] > prob and probs[pos] <= prob (pos \in [0, probs.size()])
			size_t pos = std::distance(probs.begin(), std::lower_bound(probs.begin(), probs.end(), prob, std::greater<double>()));
			// 2. return ranks[pos] = \sum_{i=0}^{pos-1}probs[i]
			// assert(fabs(ranks[pos] - dummy_position(prob)) < EPS);
			return ranks[pos];
		}

		// DEBUG: reference impl. for position()
		/*
		double dummy_position(double prob) const {
			double rank = 0;
			for (int i = 0; i < probs.size(); i++) {
				if (probs[i] > prob) rank += 1.0 / (N * probs[i]);
			}
			return rank;
		}
		*/

		double inv_position(double val) const {
			// find first pos s.t. rank[pos] >= cnt
			size_t pos = std::distance(ranks.begin(), std::lower_bound(ranks.begin(), ranks.end(), val));
			// so probs[pos-1] is what we want, since position(probs[pos-1] - eps) = rank[pos] >= cnt
			if (pos == 0) return 1.0;
			else if (pos == N + 1) return 0.0;
			else return probs[pos - 1];
		}

	};
} // namespace smoothPwd
