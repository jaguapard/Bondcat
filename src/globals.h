#pragma once
#include <mutex>
#include "Move.h"
#include <vector>

struct Globals
{
	static inline std::mutex tablebase_mutex;
	static const std::vector<Move>& getPieceMoveCandidates(Square from, PieceType type);
};