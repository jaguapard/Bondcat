#pragma once
#include <mutex>
#include "Move.h"
#include <vector>
#include <array>

struct Globals
{
	static inline std::mutex tablebase_mutex;
	static void init();
	//static const std::vector<Move>& getPieceMoveCandidates(Square from, PieceType type);

	//Basic piece movements, not considering any obstructions, checks or en passant captures.
	//WARNING: pawn moves are only moves, not captures (i.e. one or two steps forward depending on rank)! 
	//For other pieces, moves == captures.
	//uint64_t at index i indicates whether a corresponding piece at board square i can move into board square j
	//Thus, a question: can bishop at index 36 move to square 21 is answered by expression: bishopMoves[36] & (1ull << 21) != 0
	static inline std::array<uint64_t, 64> kingMoves, queenMoves, rookMoves, bishopMoves, knightMoves, blackPawnMoves, whitePawnMoves, blackPawnCaptures, whitePawnCaptures;
};