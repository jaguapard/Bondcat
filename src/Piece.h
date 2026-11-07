#pragma once
#include "enums.h"

enum Team
{
	WHITE,
	BLACK,
	NONE,
};

enum class PieceType
{
	PAWN,
	ROOK,
	QUEEN,
	KING,
	BISHOP,
	KNIGHT,
	EMPTY,
	UNINITIALIZED,
	COUNT,
};

struct Piece
{
	PieceType type = PieceType::UNINITIALIZED;
	Team team;
	bool hasMoved = false;
};