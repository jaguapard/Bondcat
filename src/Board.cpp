#include "Board.h"
#include <assert.h>
#include <string.h>
#include <bit>
void Board::removePiece(int index)
{
	assert(index >= 0 && index < 64);
	uint64_t mask = ~(1ull << index);
	white &= mask;
	black &= mask;
	kings &= mask;
	queens &= mask;
	rooks &= mask;
	bishops &= mask;
	knights &= mask;
	pawns &= mask;
}

void Board::movePiece(int fromIndex, int toIndex)
{
	uint64_t moveFrom = 1ull << fromIndex;
	uint64_t moveTo = 1ull << toIndex;

	uint64_t movingBlack = black & moveFrom;
	uint64_t movingWhite = white & moveFrom;
	uint64_t movingPawn = pawns & moveFrom;
	uint64_t movingKnight = knights & moveFrom;
	uint64_t movingBishop = bishops & moveFrom;
	uint64_t movingRook = rooks & moveFrom;
	uint64_t movingQueen = queens & moveFrom;
	uint64_t movingKing = kings & moveFrom;

	//clear moving and target
	black &= ~moveFrom & ~moveTo;
	white &= ~moveFrom & ~moveTo;
	pawns &= ~moveFrom & ~moveTo;
	knights &= ~moveFrom & ~moveTo;
	bishops &= ~moveFrom & ~moveTo;
	rooks &= ~moveFrom & ~moveTo;
	queens &= ~moveFrom & ~moveTo;
	kings &= ~moveFrom & ~moveTo;

	white |= (movingWhite >> fromIndex) << toIndex;
	black |= (movingBlack >> fromIndex) << toIndex;
	pawns |= (movingPawn >> fromIndex) << toIndex;
	knights |= (movingKnight >> fromIndex) << toIndex;
	bishops |= (movingBishop >> fromIndex) << toIndex;
	rooks |= (movingRook >> fromIndex) << toIndex;
	queens |= (movingQueen >> fromIndex) << toIndex;
	kings |= (movingKing >> fromIndex) << toIndex;
}

/*
bool Board::checkConsistency(bool classicRules) const
{
	if (white & black) return false;
	if (classicRules)
	{
		if (std::popcount(kings) != 2) return false;
		if (std::popcount(kings & black) != 1) return false;
		if (std::popcount(kings & white) != 1) return false;
		if (std::popcount(pawns & white) > 8) return false;
		if (std::popcount(pawns & black) > 8) return false;
		//technically, you can have all pawns promoted
		int qc = std::popcount(queens&)
		if (std::popcount(knights & black) > 10) return false;
		if (std::popcount(knights & white) > 10) return false;
	}
	if ()
	return true;
}*/

bool Board::operator<(const Board& other) const
{
	static_assert(sizeof(*this) == 8 * 8); //8 64-bit uints with no padding
	return memcmp(this, &other, sizeof(*this)) < 0;
}
