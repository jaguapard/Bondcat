#pragma once
#include "enums.h"

struct Piece
{
	PieceType type = PieceType::UNINITIALIZED;
	Team team;
	bool hasMoved = false;
};