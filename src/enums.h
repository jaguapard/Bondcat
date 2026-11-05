#pragma once
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

enum class CastlingType
{
	WHITE_KINGSIDE,
	WHITE_QUEENSIDE,
	BLACK_KINGSIDE,
	BLACK_QUEENSIDE,
};
