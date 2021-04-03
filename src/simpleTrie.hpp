/*
 * simpleTrie.hpp
 * Copyright (c) 2021 Yuanming Song
 */

#pragma once

#include <cstdio>
#include <cstring>

#include <vector>
#include <bitset>
#include <algorithm>
#include <mutex>

#include "common.hpp"
#include "baseNode.hpp"

namespace smoothPwd
{
	class SimpleNode : public BaseNode {
	public:
		char *s;

		SimpleNode(ull _cnt = 0, char *_s = nullptr, int w = 2) : BaseNode(_cnt, w), s(_s) {
		}
	};

	class SimpleTrie {
	private:
		void add_pfx(const char *s, ull cnt = 1, size_t idx = 0); // add prefixes of s to trie

		inline size_t add_node(ull cnt = 0, char *s = nullptr, int w = 2) {
			//std::unique_lock<std::mutex> lock(tree_mut); // we may use this in parallel versions
			tree.emplace_back(cnt, s, w);
			return (size_t)(tree.size() - 1);
		}


		void pushdown(size_t x);

	public:
		size_t root; // root node
		size_t start_ch;
		std::vector<SimpleNode> tree;
		int gram_size;

		SimpleTrie(int _gram_size = MAX_GRAM_SIZE);

		~SimpleTrie();

		void add_sub(const char *s, ull cnt = 1) { // add all substrings of s to trie
			size_t l = strlen(s);
			tree[root].cnt += cnt;
			add_pfx(s, cnt, start_ch);
			for (size_t i = 0; i < l; i++) { // excludes end symbol
				add_pfx(s + i, cnt, root);
			}
		}
	};
} // namespace smoothPwd
