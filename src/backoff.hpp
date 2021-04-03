/*
 * backoff.hpp
 * Copyright (c) 2021 Yuanming Song
 */

#pragma once

#include "baseTrie.hpp"


namespace smoothPwd
{
	class KatzBackoffModel : public BaseTrieModel {
	private:
		void get_probs(size_t idx); // precompute transition probabilities
	public:
		const ull K;

		KatzBackoffModel(ull _K) : BaseTrieModel(MAX_GRAM_SIZE), K(_K) {};

		void preprocess();
	};
} // namespace smoothPwd
