/*
 * baseNode.hpp
 * Copyright (c) 2021 Yuanming Song
 */

#pragma once

#include "common.hpp"

namespace smoothPwd
{
	class BaseNode {
	public:
		ull cnt;                // count of this node
		ull cnt_end;            // count of \0 kid of this node
		std::vector<size_t> ch; // kids
		bset v;                 // also kids (but in bits)

		BaseNode(ull _cnt = 0, int w = 2) : cnt(_cnt), cnt_end(0), ch(), v(0) {
			ch.reserve(w); // reserve w slots for ch
		}

		inline size_t idx_count(int x) const {
			return (v << (CHAR_NUM - x)).count();
		}

		inline void add_ch(char c, size_t nd) { // add a new child
			int x = ord(c);
			if (v[x])
				return;
			v.set(x);
			auto pos = ch.begin() + idx_count(x);
			ch.insert(pos, nd);
		}

		inline size_t find_ch(char c) const {
			int x = ord(c);
			if (!v[x])
				return 0; // 0 is no. of root; would never be a child node, so it's ok
			return ch[idx_count(x)];
		}

		inline int has_ch(char c) const { return v[ord(c)]; }

		inline void remove_ch(char c) {
			if (!has_ch(c)) return; // not found
			int x = ord(c);
			auto pos = ch.begin() + idx_count(x);
			ch.erase(pos);
		}
	};

	class Node : public BaseNode {
	public:
		const char c;
		const int level;
		double prob;     // (pre-calculated) transition probability
		double prob_end; // (pre_calculated) end symbol transition prob.
		double b;        // backoff factor
		double pf;       // pruning factor; optimal path from current node to [ed]
		size_t fail;     // fail index

		Node(char _c, int _level, ull _cnt = 0, ull _cnt_end = 0, int bucket = 4) :
			BaseNode(_cnt, bucket), c(_c), level(_level), prob(0.0), prob_end(0.0), b(1.0), pf(1.0), fail(0) {
			cnt_end = _cnt_end;
		}
	};
} // namespace smoothPwd
