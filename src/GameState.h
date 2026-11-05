#pragma once
#include <array>
#include <string>
#include "Piece.h"
#include "Move.h"
#include <vector>
#include <optional>

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
	Piece& getPieceAt(int x, int y);
	static GameState fromFEN(const std::string& fenStr);

	//Attempts to move from srcX,srcY using discrette steps of size stepX,stepY until encountering allied piece or capturable enemy piece.
	//If maxSteps is > 0, stops the movement after making maxSteps steps
	//Returns the result as pair of X and Y positions, or nullopt if not a single step can be made.
	//Move must contain in-bounds src and dst coordinates. Does not check the legality of moves, only pathing
	std::optional<std::pair<int, int>> getFurthestReachableSquare(const SimpleMove& move, int stepX, int stepY, int maxSteps = -1);//, bool ignoreDst = true);

	//Returns the count of steps made if the target square can be reached from starting square by using at most (stepX,stepY) discrette steps.
	//Or -1 if that's impossible.
	//Returns 0 when starting == target
	int countStepsToPathTo(const SimpleMove& move, int stepX, int stepY, int maxSteps) const;

	//Returns true if destination can be reached from source square by using (stepX,stepY)-sized discrette steps.
	//Returns false for empty pieces, out-of-bounds positions and if source == destination.
	//If this function returned true, that means that target square is either empty, or first enemy piece on the way
	bool pieceCanPathTo(const SimpleMove& move, int stepX, int stepY);
};