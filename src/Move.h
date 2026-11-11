#pragma once
#include "enums.h"
#include <optional>
#include "Piece.h"
#include "Square.h"

enum class MoveType
{
	MOVE_TO_SQUARE, //simple move to square and capture an enemy piece located there
	MOVE_AND_PROMOTE_TO_KNIGHT,
	MOVE_AND_PROMOTE_TO_BISHOP,
	MOVE_AND_PROMOTE_TO_ROOK,
	MOVE_AND_PROMOTE_TO_QUEEN,
	WHITE_KINGSIDE_CASTLE,
	BLACK_KINGSIDE_CASTLE,
	WHITE_QUEENSIDE_CASTLE,
	BLACK_QUEENSIDE_CASTLE,
};

//Describes a move attempted by the player. type describes the type of the move, square indices are ignored for castling (use hardcoded ones)
struct Move
{
	Move() = default;
	Move(Square from, Square to, MoveType type);

	Square from, to;
	MoveType type;

	//Returns a promotion target stored or nullopt if there is none. Does not check if it is legal
	std::optional<PieceType> getPromotionTarget() const;
	bool isCastlingMove() const;

	std::string toUCI() const;
	static Move fromUCI(const std::string& uci, bool isCastling);
};