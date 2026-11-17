#include "Move.h"
#include <assert.h>
#include "BondcatException.h"

Move::Move(Square from, Square to, MoveType type)
{
	this->from = from;
	this->to = to;
	this->type = type;
}

std::optional<PieceType> Move::getPromotionTarget() const
{
	switch (type)
	{
	case MoveType::MOVE_AND_PROMOTE_TO_BISHOP:
		return PieceType::BISHOP;
	case MoveType::MOVE_AND_PROMOTE_TO_KNIGHT:
		return PieceType::KNIGHT;
	case MoveType::MOVE_AND_PROMOTE_TO_ROOK:
		return PieceType::ROOK;
	case MoveType::MOVE_AND_PROMOTE_TO_QUEEN:
		return PieceType::QUEEN;
	default:
		return std::nullopt;
	}
}

bool Move::isCastlingMove() const
{
	switch (type)
	{
	case MoveType::WHITE_KINGSIDE_CASTLE:
	case MoveType::BLACK_KINGSIDE_CASTLE:
	case MoveType::WHITE_QUEENSIDE_CASTLE:
	case MoveType::BLACK_QUEENSIDE_CASTLE:
		return true;
	default: return false;
	}
}

std::string Move::toUCI() const
{
	std::string ret = from.toAlgebraic() + to.toAlgebraic();
	switch (type)
	{
	case MoveType::MOVE_AND_PROMOTE_TO_KNIGHT: ret.push_back('n'); break;
	case MoveType::MOVE_AND_PROMOTE_TO_BISHOP: ret.push_back('b'); break;
	case MoveType::MOVE_AND_PROMOTE_TO_ROOK: ret.push_back('r'); break;
	case MoveType::MOVE_AND_PROMOTE_TO_QUEEN: ret.push_back('q'); break;
	case MoveType::WHITE_KINGSIDE_CASTLE: return "e1g1";
	case MoveType::BLACK_KINGSIDE_CASTLE: return "e8g8";
	case MoveType::WHITE_QUEENSIDE_CASTLE: return "e1c1";
	case MoveType::BLACK_QUEENSIDE_CASTLE: return "e8c8";
	default: break;
	}
	return ret;
}

//logistical challenges to use it in main
Move Move::fromUCI(const std::string& moveStr, bool isCastling)
{
	assert(moveStr.length() == 4 || moveStr.length() == 5);
	std::string src = moveStr.substr(0, 2);
	std::string dst = moveStr.substr(2, 2);

	Move move(Square(src), Square(dst), MoveType::MOVE_TO_SQUARE);
	if (moveStr.length() > 4)
	{
		if (moveStr[4] == 'q') move.type = MoveType::MOVE_AND_PROMOTE_TO_QUEEN;
		if (moveStr[4] == 'n') move.type = MoveType::MOVE_AND_PROMOTE_TO_KNIGHT;
		if (moveStr[4] == 'r') move.type = MoveType::MOVE_AND_PROMOTE_TO_ROOK;
		if (moveStr[4] == 'b') move.type = MoveType::MOVE_AND_PROMOTE_TO_BISHOP;
	}
	else if (isCastling)
	{
		if (moveStr == "e1g1") move.type = MoveType::WHITE_KINGSIDE_CASTLE;
		else if (moveStr == "e1c1") move.type = MoveType::WHITE_QUEENSIDE_CASTLE;
		else if (moveStr == "e8g8") move.type = MoveType::BLACK_KINGSIDE_CASTLE;
		else if (moveStr == "e8c8") move.type = MoveType::BLACK_QUEENSIDE_CASTLE;
		else throw BondcatException("UCI wrong castling: " + moveStr);
	}

	return move;
}

Square Move::getFrom() const
{
	return Square(_from);
}

Move& Move::setFrom(Square s)
{
	assert(s.inBounds());
	_from = s.toIndex();
	return *this;
}

Square Move::getTo() const
{
	return Square(_to);
}

Move& Move::setTo(Square s)
{
	assert(s.inBounds());
	_to = s.toIndex();
	return *this;
}

MoveType Move::getType() const
{
	return MoveType(_type);
}

Move& Move::setType(MoveType m)
{
	_type = uint16_t(m);
	return *this;
}
