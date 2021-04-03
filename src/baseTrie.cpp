/*
 * baseTrie.cpp
 * Copyright (c) 2021 Yuanming Song
 */

#include "baseTrie.hpp"

#include <iostream>
#include <queue>

#include <cassert>

using smoothPwd::BaseTrieModel;
using smoothPwd::StrProb;
using smoothPwd::ull;
using smoothPwd::bset;
using std::string;
using std::vector;
using std::tuple;
using std::make_tuple;
using std::max;
using std::get;

size_t BaseTrieModel::add_from_trie(char cur_char, size_t tx, ull prune, int level) {
	// just turn a simple trie subtree into a trie subtree for now;
	// you can try out some pruning techniques with this function yourself.
	// please make sure sn_cnt > K when calling this function!

	SimpleNode &sn = s_trie->tree[tx];
	int tot = (int)sn.ch.size();
	assert(sn.cnt > prune);
	ull cnt_end = sn.cnt_end > prune ? sn.cnt_end : 0;
	size_t idx = add_node(cur_char, level, sn.cnt, cnt_end, max(tot, 1));

	if (tot == 0) { // leaf node; expand tail
		if (sn.s == nullptr)
			return idx;        // no tail
		tree[idx].cnt_end = 0; // restore cnt_end

		int ptr = 0;
		while (sn.s[ptr] == '\0')
			++ptr;

		size_t prev_idx = idx, ch_idx = idx;
		int ch_level = level + 1;
		while (sn.s[ptr] != '\0') { // create a chain of nodes
			char c = sn.s[ptr];
			ch_idx = add_node(c, ch_level, sn.cnt, 0, 1);
			tree[prev_idx].add_ch(c, ch_idx);
			prev_idx = ch_idx;
			++ptr;
			++ch_level;
		}
		tree[ch_idx].cnt_end = cnt_end; // real position of cnt_end

		return idx;
	}
	else {
		int ith = 0;
		for (int i = 0; i < CHAR_NUM; i++) {
			if (ith >= tot)
				break;
			if (!sn.v[i])
				continue;
			size_t sn_ch = sn.ch[ith];
			++ith;
			if (s_trie->tree[sn_ch].cnt <= prune)
				continue;

			size_t ch_idx = add_from_trie(chr(i), sn_ch, prune, level + 1);
			tree[idx].add_ch(chr(i), ch_idx);
		}
		tree[idx].ch.shrink_to_fit();
		return idx;
	}
}

void BaseTrieModel::aggressive_prune() {
	// experimental feature
}

void BaseTrieModel::get_fail() {
	std::queue<size_t> Q;

	for (size_t ch_idx : tree[root].ch) {
		tree[ch_idx].fail = root;
		Q.push(ch_idx);
	}

	while (!Q.empty()) {
		size_t cur_idx = Q.front();
		Q.pop();

		Node &cur_node = tree[cur_idx];
		Node &cur_fail = tree[cur_node.fail];
		assert(cur_node.ch.size() == cur_node.v.count());

		for (size_t ch_idx : cur_node.ch) {
			Node &ch_node = tree[ch_idx];
			char c = ch_node.c;

			size_t ch_fail_idx = cur_fail.find_ch(c);
			assert(ch_fail_idx != 0);
			ch_node.fail = ch_fail_idx;

			Q.push(ch_idx);
		}
	}
}

void BaseTrieModel::build_trie(ull prune) {
	root = add_from_trie('\0', s_trie->root, prune, 0);
	tree.shrink_to_fit();
	s_trie.reset(nullptr);

	// set fail edges
	start_idx = tree[root].find_ch('\0');
	tree[root].fail = root;
	get_fail();

	// remove start_idx from root's children
	tree[root].remove_ch('\0');
	assert(tree[root].cnt_end == 0);
	tree[root].cnt_end = tree[start_idx].cnt;
}

double BaseTrieModel::ch_prob(size_t pred, char c, size_t &nt) const {
	const Node &pred_nd = tree[pred];
	if (c == '\0') {
		return pred_nd.prob_end; // is precomputed even if pred_nd.cnt_end == 0
	}
	else if (pred_nd.has_ch(c)) { // found
		size_t ch_idx = pred_nd.find_ch(c);
		nt = ch_idx;
		return tree[ch_idx].prob;
	}
	else { // not found
		size_t fail_idx = pred_nd.fail;
		nt = fail_idx;
		if (pred == root) // reached root; stop failing
			return pred_nd.b * pred_nd.prob;
		else
			return pred_nd.b * ch_prob(fail_idx, c, nt);
	}
}

void BaseTrieModel::ch_search(size_t idx, string &s, const bset v, double p) const {
	const Node &nd = tree[idx];
	if (p * nd.pf <= PRUNE_EPS * min_threshold) return; // pruned

	if (!v[end_ord]) { // end symbol
		double ch_p = p * nd.prob_end;
		if (ch_p > min_threshold && ch_p <= max_threshold) // (min_threshold, max_threshold]
			oracle(s, ch_p);
	}

	for (size_t ch_idx : nd.ch) {
		const Node &cur_ch = tree[ch_idx];
		char c = cur_ch.c;
		if (v[ord(c)])
			continue; // banned
		else {
			double ch_p = p * cur_ch.prob;
			if (ch_p <= min_threshold)
				continue; // pruned
			s.push_back(c);
			ch_search(ch_idx, s, empty_bset, ch_p);
			s.pop_back();
		}
	}

	double fail_p = p * nd.b;
	if (fail_p <= min_threshold)
		return; // not much prob. left

	bset fail_v = v | (nd.v);
	fail_v.set(end_ord);

	if (fail_v.all())
		return; // no need to fail

	if (idx == root) {
		assert(fail_v[end_ord]); // \0 is always banned

		fail_p = fail_p * tree[root].prob;
		if (fail_p <= min_threshold)
			return;

		for (int i = 0; i < CHAR_NUM; i++) {
			if (fail_v[i])
				continue;
			char c = chr(i);
			s.push_back(c);
			ch_search(root, s, empty_bset, fail_p);
			s.pop_back();
		}
	}
	else {
		ch_search(nd.fail, s, fail_v, fail_p);
	}
}

tuple<char, double, size_t> BaseTrieModel::sample_ch(size_t idx, const bset v, double rand_val) const {
	const Node &nd = tree[idx];

	if (!v[end_ord]) {
		double prob = nd.prob_end;
		rand_val -= prob;
		if (rand_val < 0)
			return make_tuple('\0', prob, idx);
	}

	for (size_t ch_idx : nd.ch) {
		const Node &cur_ch = tree[ch_idx];
		char c = cur_ch.c;
		if (v[ord(c)])
			continue; // banned
		else {
			double prob = cur_ch.prob;
			rand_val -= prob;
			if (rand_val < 0)
				return make_tuple(c, prob, ch_idx);
		}
	}

	bset fail_v = v | (nd.v);
	fail_v.set(end_ord);
	if (fail_v.all()) // it's possible because of floating point errors!
		return make_tuple('\0', -1.0, idx); // error; try to handle that yourself

	if (idx == root) {
		assert(fail_v[end_ord]); // \0 is always banned

		double prob = nd.b * nd.prob;

		for (int i = 0; i < CHAR_NUM; i++) {
			if (fail_v[i])
				continue;
			char c = chr(i);
			rand_val -= prob;
			if (rand_val < 0) {
				return make_tuple(c, prob, idx);
			}
		}
		return make_tuple('\0', -1.0, idx); // also error
	}
	else {
		assert(nd.b > 0.0);
		auto res = sample_ch(nd.fail, fail_v, rand_val / nd.b);
		get<1>(res) *= nd.b;
		return res;
	}
}

void BaseTrieModel::sanity_check() {
	for (size_t idx = 0; idx < tree.size(); idx++) {
		size_t nt;
		double cum_prob = 0.0, direct_prob = 0.0, fail_prob = 0.0;
		for (int i = 0; i < CHAR_NUM; i++) {
			char c = chr(i);
			double cur_prob = ch_prob(idx, c, nt);
			if (tree[idx].has_ch(c) || (c == '\0' && tree[idx].cnt_end > 0)) direct_prob += cur_prob;
			else fail_prob += cur_prob;
			cum_prob += cur_prob;
		}

		if (fabs(1 - cum_prob) >= EPS) {
			std::cerr << start_idx << std::endl;
			std::cerr << idx << " " << cum_prob << " " << direct_prob << " " << fail_prob << std::endl;
			for (int j = 0; j < CHAR_NUM; j++) {
				char c = chr(j);
				std::cerr << "--" << c << "--: " << ch_prob(idx, c, nt) << std::endl;
			}
			throw "Trie Error";
		}
	}
}

double BaseTrieModel::pwd_prob(const char *s) const {
	size_t nt = start_idx;
	size_t len = strlen(s);
	double p = 1.0;

	for (size_t i = 0; i <= len && p > 0.0; i++) { // include end symbol
		size_t cur = nt;
		p *= ch_prob(cur, s[i], nt);
		//std::cout << s << ": " << cur << " " << tree[cur].c << "->" 
		//	<< nt << " " << tree[nt].c << ", " << p << std::endl;
		if (p == 0.0) break;
	}
	return p;
}

StrProb BaseTrieModel::sample() {
	// TODO: a somewhat costly implementation for now; will consider improving it later
	string s;
	double p = 1.0;
	size_t idx = start_idx;
	while (true) {
		double rand_val = unif(re);
		auto res = sample_ch(idx, empty_bset, rand_val); // the real search part
		//DEBUG

		double trans_prob = get<1>(res);
		if (trans_prob <= 0.0) {
			// std::cerr << "Invalid sample_ch result; random value: " << rand_val << std::endl;
			continue;
		}
		p *= trans_prob;

		char c = get<0>(res);
		if (c == '\0') break;
		s.push_back(c);
		idx = get<2>(res);
	}

	// DEBUG: validation
	assert(fabs(p - pwd_prob(s.c_str())) < EPS);
	return StrProb(s, p);
}
/*
StrProb BaseTrieModel::sample_brute() {
	// reference implementation; try not to use it
	string s;
	double p = 1.0;
	size_t idx = start_idx;
	while (true) {
		double rand_val = unif(re);
		size_t nt;
		char c;
		for (int i = 0; i < CHAR_NUM; i++) {
			c = chr(i);
			double cur_prob = ch_prob(idx, c, nt);
			rand_val -= cur_prob;
			if (rand_val < 0) {
				p *= cur_prob;
				break;
			}
		}
		idx = nt;
		if (c == '\0') break;
		else s.push_back(c);
	}

	// DEBUG: validation
	assert(fabs(p - pwd_prob(s.c_str())) < EPS);
	return StrProb(s, p);
}
*/

vector<StrProb> BaseTrieModel::generate(ull cnt, bool strict) {
	// reference: Ma et al., "A Study of Probabilistic Password Models". Oakland'14. 
	// it's possible that this implementation would produce (a small number of) duplicates.
	vector<StrProb> guesses;
	oracle = [&guesses](string &s, double p) { guesses.emplace_back(s, p); };
	min_threshold = 1.0 / cnt; max_threshold = 1.0; // start with a conservative range of (1/cnt, 1]
	while (guesses.size() < cnt) {
		string s;
		ch_search(start_idx, s, empty_bset, 1.0); // the real search part
		size_t guesses_size = max(guesses.size(), (size_t)1); // avoid division by 0
#ifndef NDEBUG
		std::cout << "search (" << min_threshold << ", " << max_threshold << "], tot: " << guesses.size() << std::endl;
#endif
		// calculate new threshold
		max_threshold = min_threshold;
		min_threshold = min_threshold / max(2.0, 1.5 * cnt / guesses_size);
	}

	sort(guesses.begin(), guesses.end(), [](const StrProb &a, const StrProb &b) { return a.second > b.second; });
	if (strict) guesses.resize((size_t)cnt);

	min_threshold = max_threshold = 1.0; // clear threshold
	oracle = nullptr; // clear oracle
	return guesses;
}

vector<StrProb> BaseTrieModel::generate_by_montecarlo(ull cnt, size_t num_samples) {
	// experimental feature
	std::vector<StrProb> samples;
	for (size_t i = 0; i < num_samples; i++) samples.push_back(sample());
	PosEstimator estimator(samples);
	double prob = estimator.inv_position(cnt * 1.1);
	std::cout << "Monte Carlo threshold: " << prob << std::endl;
	return generate_by_threshold(prob);
}