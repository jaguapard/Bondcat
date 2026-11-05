#pragma once
#include <array>
#include <string>
#include "Piece.h"
#include "Move.h"
#include <vector>

struct LegalMoves
{
	std::vector<SimpleMove> simple;
	std::vector<CastlingType> castlings;
	std::vector<PromotionMove> promotions;
	//std::array<bool, 4> castlings;
	//std::array<PromotionMove, 8> promotions;
};
struct GameState
{
	std::array<Piece, 64> board;
	std::array<bool, 4> allowedCastlings;
	bool whitesTurn;
	int halfmoves, fullmoves, enpassant;

	LegalMoves getAllLegalMoves();
	static GameState fromFEN(const std::string& fenStr);
};