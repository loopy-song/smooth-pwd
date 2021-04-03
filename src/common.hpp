/*
 * common.hpp
 * Copyright (c) 2021 Yuanming Song
 */

#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>
#include <bitset>

namespace smoothPwd
{
	const int MAX_LENGTH = 1024;               // max password length
	const int MAX_GRAM_SIZE = MAX_LENGTH + 10; // max gram size
	const double PRUNE_EPS = 0.999;            // pruning tolerance; closer to 1 -> faster but less accurate
	const double EPS = 1e-8;                   // relative error tolerance
	const int CHAR_NUM = 96;                   // 95 printables + start/end symbol
	// I fused start symbol -- [st] and end symbol -- [ed] together in code,
	// but they are actually very different. (I carefully crafted the code to avoid accuracy loss.)

	using StrProb = std::pair<std::string, double>;
	using ull = unsigned long long;
	using bset = std::bitset<CHAR_NUM>;

	const bset empty_bset(0);
	const int end_ord = CHAR_NUM - 1;

	inline char chr(int x) { return x == end_ord ? '\0' : x + 0x20; }

	inline int ord(char c) { return c == '\0' ? end_ord : c - 0x20; }

	inline void cleanse(char *s) {
		size_t l = strlen(s);
		while (l && (s[l - 1] == '\n' || s[l - 1] == '\r'))
			--l;
		s[l] = '\0';
	}

} // namespace smoothPwd
