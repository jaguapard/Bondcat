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

	__declspec(property(get = getFrom, put = setFrom)) Square from;
	__declspec(property(get = getTo, put = setTo)) Square to;
	__declspec(property(get = getType, put = setType)) MoveType type;

	//Returns a promotion target stored or nullopt if there is none. Does not check if it is legal
	std::optional<PieceType> getPromotionTarget() const;
	bool isCastlingMove() const;

	std::string toUCI() const;
	static Move fromUCI(const std::string& uci, bool isCastling);

	Square getFrom() const;
	Move& setFrom(Square s);
	Square getTo() const;
	Move& setTo(Square s);
	MoveType getType() const;
	Move& setType(MoveType m);
private:
	uint16_t _from : 6, _to : 6, _type : 4; //bits 0..5 store source square index, 6..11 target square index, 12..15 type 
};