/*
 * backoff.cpp
 * Copyright (c) 2021 Yuanming Song
 */

#include "backoff.hpp"

#include <queue>
#include <cassert>

using smoothPwd::KatzBackoffModel;
using std::max;

void KatzBackoffModel::get_probs(size_t idx) {
	Node& nd = tree[idx];
	Node& fail_nd = tree[nd.fail];
	nd.prob_end = (double)nd.cnt_end / nd.cnt;
	double pf = nd.prob_end;

	ull disc = nd.cnt - nd.cnt_end;
	ull lowp_nom = nd.cnt_end > 0 ? tree[nd.fail].cnt_end : 0;
	for (size_t ch_idx : nd.ch) {
		Node& ch_nd = tree[ch_idx];
		ch_nd.prob = (double)ch_nd.cnt / nd.cnt;
		disc -= ch_nd.cnt;
		lowp_nom += tree[ch_nd.fail].cnt;

		get_probs(ch_idx);
		double chpf = ch_nd.prob * ch_nd.pf;
		pf = max(pf, chpf);
	}

	double leftover = (double)disc / nd.cnt;
	double lower_prob;

	if (idx == root) {
		get_probs(start_idx);
		size_t nom = nd.ch.size() + 1;
		if (nom == CHAR_NUM) lower_prob = 1.0;
		else lower_prob = 1.0 - (double)nom / CHAR_NUM;
	}
	else {
		if (lowp_nom == fail_nd.cnt) lower_prob = 1.0;
		else lower_prob = 1 - (double)lowp_nom / fail_nd.cnt;
		if (idx == 1) {
			double cum_prob = 0.0;
			for (size_t ch_idx : nd.ch) {
				cum_prob += tree[ch_idx].prob;
			}
			//std::cout << "cum_prob: " << cum_prob << " prob_end: " << nd.prob_end << std::endl;
			// std::cout << "leftover: " << leftover << " nom: " << lowp_nom << " lower prob:" << lower_prob << std::endl;
		}
	}

	nd.b = leftover / lower_prob;
	nd.pf = max(pf, leftover);
}

void KatzBackoffModel::preprocess() {
	build_trie(K);
	Node& root_nd = tree[root];
	root_nd.prob = 1.0 / (CHAR_NUM);

	assert(tree[root].cnt_end > K);
	get_probs(root);

	// interpolate prob_end
	std::queue<size_t> Q;
	for (size_t ch_idx : root_nd.ch) Q.push(ch_idx);
	Q.push(start_idx);
	while (!Q.empty()) {
		size_t cur_idx = Q.front(); Q.pop();
		Node& cur_nd = tree[cur_idx];
		if (cur_nd.cnt_end == 0) cur_nd.prob_end = cur_nd.b * tree[cur_nd.fail].prob_end;

		for (size_t ch_idx : cur_nd.ch) Q.push(ch_idx);
	}

}
