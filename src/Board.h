#pragma once
#include <stdint.h>
struct Board
{
	uint64_t white = 0, black = 0, kings = 0, queens = 0, rooks = 0, bishops = 0, knights = 0, pawns = 0;
	void removePiece(int index);
	void movePiece(int fromIndex, int toIndex);
	//bool checkConsistency(bool classicRules = true) const; //checks whether the board makes sense (i.e. no pieces belonging to different types or teams at the same time). If classicRules, also checks if count of pieces makes sense
	//static Board makeEmptyBoard();

	bool operator<(const Board& other) const;
};