#pragma once
#include <mutex>
#include "Move.h"
#include <vector>

struct Globals
{
	static inline std::mutex tablebase_mutex;

	static inline std::vector<std::vector<Move>> pawnAttacks, knightAttacks, bishopAttacks, rookAttacks, queenAttacks, kingAttacks;
};