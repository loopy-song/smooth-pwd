/*
 * kneserNey.hpp
 * Copyright (c) 2021 Yuanming Song
 * The Kneser-Ney implementation here is partly based on https://github.com/smilli/kneser-ney
 */

#pragma once

#include <iostream>

#include "baseTrie.hpp"

namespace smoothPwd
{
	using InterimNode = std::pair<ull, size_t>; // <adj_cnt, fail>

	class NodeTable {
	private:
		// num_count[k][t]: num of k-grams with count t
		std::vector<std::vector<size_t> > num_count;
		// discounts[k][t]: discount for k-gram with count t
		std::vector<std::vector<double> > discounts;

	public:
		const int gram_size;
		const int num_discount_param;
		const size_t tree_size;
		const size_t root;
		std::vector<std::unordered_map<size_t, InterimNode> > tb;

		NodeTable(int _gram_size, int _num_discount_param, size_t _tree_size, size_t _root) :
			gram_size(_gram_size), num_discount_param(_num_discount_param), tree_size(_tree_size), root(_root), tb(_gram_size + 1) {

		}

		inline size_t end_idx(size_t idx) const { return idx + tree_size; }
		inline bool is_end_idx(size_t idx) const { return idx >= tree_size; }
		inline size_t inv_end_idx(size_t idx) const { return idx - tree_size; }

		inline void add_item(int level, size_t idx, ull cnt, size_t fail, bool expand) {
			ull initial_cnt = level == gram_size ? cnt : 0;
			tb[level][idx] = InterimNode(initial_cnt, expand ? idx : fail);
		}

		inline void add_node(int base_level, const Node& nd, size_t idx) {
			add_item(base_level, idx, nd.cnt, nd.fail, base_level > nd.level);
			int end_level = base_level + 1;
			if (nd.cnt_end > 0 && end_level <= gram_size) { // expand end node
				size_t end_fail = (idx == root ? idx : end_idx(nd.fail));
				add_item(end_level, end_idx(idx), nd.cnt_end, end_fail, end_level > nd.level + 1);
			}
		}

		void calc_discount();

		inline double get_discount(size_t level, ull cnt) {
			return discounts[level][cnt > (ull)num_discount_param ? num_discount_param : (size_t)cnt];
		}

	};

	class ModifiedKneserNeyModel : public BaseTrieModel {
	private:
		void build_table(NodeTable& table);

		void get_probs(NodeTable& table);

		void get_pf(size_t idx);

	public:
		const int num_discount_param;

		ModifiedKneserNeyModel(int _gram_size, int _num_discount_param = 3) :
			BaseTrieModel(_gram_size), num_discount_param(_num_discount_param) {
		};

		void preprocess();
	};
} // namespace smoothPwd
