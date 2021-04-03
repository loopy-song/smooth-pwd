/*
 * simpleTrie.cpp
 * Copyright (c) 2021 Yuanming Song
 */
#include "simpletrie.hpp"

#include <cassert>

using smoothPwd::SimpleTrie;
using smoothPwd::ull;

SimpleTrie::SimpleTrie(int _gram_size) : gram_size(_gram_size) {
	root = add_node(0, nullptr, CHAR_NUM);
	assert(root == 0);

	for (int i = 0; i < CHAR_NUM; i++) {
		char c = chr(i);
		size_t nd = add_node(0, nullptr, CHAR_NUM);
		assert(nd == (size_t)(i + 1));
		tree[root].add_ch(c, nd);
	}
	start_ch = tree[root].find_ch('\0');
}

SimpleTrie::~SimpleTrie() {
	for (auto it = tree.begin(); it != tree.end(); it++) {
		delete[] it->s;
	}
}

void SimpleTrie::pushdown(size_t x) {
	char *xs = tree[x].s;
	if (xs == nullptr)
		return;
	tree[x].s = nullptr;

	int l = 0;
	while (xs[l] == '\0')
		++l;

	size_t kid = add_node(tree[x].cnt, xs);

	tree[x].add_ch(xs[l], kid);
	xs[l] = '\0';

	tree[kid].cnt_end = tree[x].cnt_end;
	tree[x].cnt_end = 0;

	if (xs[l + 1] == '\0') { // would be empty
		delete[] xs;
		tree[kid].s = nullptr;
	}
}

void SimpleTrie::add_pfx(const char *s, ull cnt, size_t idx) {
	size_t cur = idx;
	int l = (int)strlen(s);
	int real_limit = gram_size;
	if (idx != root)
		real_limit--; // start symbol inserted

	int pfx_len = std::min(l, real_limit);
	bool reach_end = (l < real_limit); // shall we update cnt_end?

	for (int i = 0; i < pfx_len; i++) {
		char c = s[i];

		pushdown(cur);
		assert(tree[cur].s == nullptr);
		tree[cur].cnt += cnt;

		size_t kid = tree[cur].find_ch(c);
		if (kid != 0) { // found
			cur = kid;
		}
		else {
			size_t nd;
			if (i == pfx_len - 1) { // reached end; no need to trick yourself
				nd = add_node(cnt);
			}
			else { // (pfx_len - 1) - (i + 1) + 1 = pfx_len - i - 1 char not allocated
				int buffer_size = pfx_len - i;
				char *p = new char[buffer_size];
				memcpy(p, s + i + 1, sizeof(char) * (buffer_size - 1));
				p[buffer_size - 1] = '\0'; // end marker
				assert(strlen(p) > 0);
				nd = add_node(cnt, p);
			}

			tree[cur].add_ch(c, nd);
			if (reach_end)
				tree[nd].cnt_end = cnt;
			return;
		}
	}

	pushdown(cur);
	tree[cur].cnt += cnt;
	if (reach_end)
		tree[cur].cnt_end += cnt; // end symbol
}
