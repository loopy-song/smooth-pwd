/*
 * kneserNey.cpp
 * Copyright (c) 2021 Yuanming Song
 * The Kneser-Ney implementation here is partly based on https://github.com/smilli/kneser-ney
 */

#include "kneserNey.hpp"

#include <queue>
#include <cassert>

using smoothPwd::NodeTable;
using smoothPwd::ModifiedKneserNeyModel;

using std::vector;
using std::pair;
using std::min;
using std::max;
using std::cout;
using std::endl;

void ModifiedKneserNeyModel::build_table(NodeTable& table) {
	for (size_t idx = 0; idx < tree.size(); idx++) {
		const Node& nd = tree[idx];
		table.add_node(nd.level, nd, idx);
	}

	// subtree of start node
	std::queue<size_t> Q;
	const Node &start_nd = tree[start_idx];
	for (int j = 2; j < gram_size; j++) table.add_node(j, start_nd, start_idx); // only n-1 start symbols
	for (size_t ch_idx : start_nd.ch) Q.push(ch_idx);

	while (!Q.empty()) {
		size_t idx = Q.front();
		const Node &nd = tree[idx];
		Q.pop();

		for (int j = nd.level + 1; j <= gram_size; j++) { // expand start symbol
			table.add_node(j, nd, idx);
		}
		for (size_t ch_idx : nd.ch) Q.push(ch_idx);
	}

	// DEBUG: see how much memory we saved (or, how little)
#ifndef NDEBUG
	size_t table_size = 0;
	for (const auto& row : table.tb) { table_size += row.size(); }
	cout << "compressed: " << tree.size() << " expanded: " << table_size << endl;
#endif

	// calculate adjusted count
	for (size_t level = 2; level < table.tb.size(); level++) {
		auto& last_row = table.tb[level - 1];
		const auto& cur_row = table.tb[level];
		for (const auto& item : cur_row) {
			size_t idx = item.first;
			if (table.is_end_idx(idx))
				assert(tree[table.inv_end_idx(idx)].cnt_end > 0);
			else
				assert(tree[idx].cnt > 0);

			last_row[item.second.second].first++; // item.second = (cnt, fail) => last_row[fail].cnt++
		}
	}

	table.calc_discount();
}

void NodeTable::calc_discount() {
	num_count.clear();
	discounts.clear();
	for (int k = 0; k <= gram_size; k++) {
		num_count.emplace_back(num_discount_param + 2, 0);
		discounts.emplace_back(num_discount_param + 1, 0.0);
		for (const auto& item : tb[k]) {
			ull cur_cnt = item.second.first;
			if (cur_cnt < (ull)num_discount_param + 2) num_count[k][(size_t)cur_cnt]++;
		}
	}

	for (int k = 1; k <= gram_size; k++) {
		const auto& k_count = num_count[k];
		size_t denom = k_count[1] + 2 * k_count[2];
		double factor = 1.0;
		if (denom > 0) factor = (double)k_count[1] / (double)denom;

		for (int t = 1; t <= num_discount_param; t++) {
			double t_disc = 0.0;
			if (k_count[t] == 0) t_disc = 0.0;
			else {
				t_disc = t - ((t + 1) * factor * k_count[t + 1]) / k_count[t];
			}
			discounts[k][t] = max(t_disc, 0.0);
		}
	}
#ifndef NDEBUG
	for (int k = 1; k <= gram_size; k++) {
		cout << "--" << k << " gram--\n";
		cout << "N_k: ";
		for (int t = 0; t <= num_discount_param + 1; t++) {
			cout << num_count[k][t] << ' ';
		}
		cout << "\ndiscounts: ";
		for (int t = 0; t <= num_discount_param; t++) {
			cout << discounts[k][t] << ' ';
		}
		cout << '\n';
	}
#endif
}

void ModifiedKneserNeyModel::get_probs(NodeTable& table) {
	tree[root].prob = 1.0 / (CHAR_NUM);
	tree[start_idx].prob = 0.0;

	for (size_t level = 1; level < table.tb.size(); level++) {
		const auto& row = table.tb[level - 1];
		const auto& next_row = table.tb[level];
		// std::unordered_map<size_t, double> prob_table;
		for (const auto& item : row) {
			size_t idx = item.first;
			if (table.is_end_idx(idx)) continue;
			Node& nd = tree[idx];
			ull pref_cnt = 0;
			double bo_prob = 0.0;

			vector<size_t> ch(nd.ch);
			size_t ch_end_idx = table.end_idx(idx);
			if (next_row.count(ch_end_idx) > 0) ch.push_back(ch_end_idx);

			vector<double> probs(ch.size());

			// calculate prefix count and backoff probability...
			for (size_t i = 0; i < ch.size(); i++) { // "normal" children
				ull adj_cnt = next_row.at(ch[i]).first;
				double disc = table.get_discount(level, adj_cnt);
				probs[i] = (double)adj_cnt - disc;
				pref_cnt += adj_cnt;
				bo_prob += disc;
			}

			bo_prob = pref_cnt > 0 ? bo_prob / (double)pref_cnt : 1.0;

			// ...and then calculate transition probabilities for each child
			for (size_t i = 0; i < ch.size(); i++) {
				size_t ch_idx = ch[i];
				size_t ch_fail_idx = next_row.at(ch_idx).second;
				double trans_prob = pref_cnt > 0 ? probs[i] / (double)pref_cnt : 0.0;
				double fail_prob = 0.0;
				// interpolation 
				if (table.is_end_idx(ch_fail_idx)) {
					fail_prob = tree[table.inv_end_idx(ch_fail_idx)].prob_end;
				}
				else fail_prob = tree[ch_fail_idx].prob;

				trans_prob += bo_prob * fail_prob;

				if (table.is_end_idx(ch_idx)) {
					tree[table.inv_end_idx(ch_idx)].prob_end = trans_prob;
				}
				else tree[ch_idx].prob = trans_prob;
				//prob_table[ch_idx] = trans_prob;
			}
			nd.b *= bo_prob;
		}
	}

	// interpolate prob_end
	std::queue<size_t> Q;
	for (size_t ch_idx : tree[root].ch) Q.push(ch_idx);
	Q.push(start_idx);
	while (!Q.empty()) {
		size_t cur_idx = Q.front(); Q.pop();
		Node& cur_nd = tree[cur_idx];
		if (cur_nd.cnt_end == 0) cur_nd.prob_end = cur_nd.b * tree[cur_nd.fail].prob_end;
		for (size_t ch_idx : cur_nd.ch) Q.push(ch_idx);
	}
}

void ModifiedKneserNeyModel::get_pf(size_t idx) {
	Node& nd = tree[idx];
	double pf = max(nd.prob_end, nd.b);

	for (size_t ch_idx : nd.ch) {
		Node& ch_nd = tree[ch_idx];
		get_pf(ch_idx);
		double chpf = ch_nd.prob * ch_nd.pf;
		pf = max(pf, chpf);
	}
	nd.pf = pf;
}

void ModifiedKneserNeyModel::preprocess() {
	build_trie(0); // normally root would be 0
	NodeTable table(gram_size, num_discount_param, tree.size(), root);
	build_table(table);
	get_probs(table);
	get_pf(root);
}
