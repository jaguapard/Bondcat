#include "Move.h"

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
