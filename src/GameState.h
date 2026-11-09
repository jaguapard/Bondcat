#pragma once
#include <array>
#include <string>
#include "Piece.h"
#include "Move.h"
#include <vector>
#include <optional>
#include <assert.h>
#include <chrono>
/*
struct LegalMoves
{
	std::vector<SimpleMove> simple;
	std::vector<CastlingType> castlings;
	std::vector<PromotionMove> promotions;
	//std::array<bool, 4> castlings;
	//std::array<PromotionMove, 8> promotions;
};*/

struct Path
{
	int startX, startY, stepX, stepY, stepCount;
	bool isPointOnPath(int x, int y);
};

struct SearchParams
{
	int maxDepth = 5; //in plies
	//uint64_t timeDeadline = SDL_GetTicksNS();
};

struct SearchReturn
{
	Move bestMove;
	float evaluation;
	std::optional<int> mateIn;
};
struct GameState
{
	std::array<Piece, 64> board;
	bool blackCanQueensideCastle = false, blackCanKingsideCastle = false;
	bool whiteCanQueensideCastle = false, whiteCanKingsideCastle = false;
	bool whitesTurn;
	int halfmoves, fullmoves;
	Square enpassant;
	GameStatus status; //TODO: turn this into optional
	bool statusUpToDate = false;

	SearchReturn findBestMove(const SearchParams& params, int currDepth, uint64_t* nodesVisited = nullptr);
	static GameState newGame();

	void updateStatus();

	//Resets the game state to the standard starting position
	void reset();

	std::vector<Move> getAllLegalMoves();
	std::vector<Move> getAllLegalMovesForPieceAt(Square sq);

	Piece& getPieceAt(Square s);
	//const Piece& getPieceAt(int x, int y) const;
	//Piece& getPieceAt(const char* algebraic);
	//const Piece& getPieceAt(const char* algebraic) const;

	static GameState fromFEN(const std::string& fenStr);

	static std::string xyToAlgebraic(int x, int y);

	std::string toFEN();

	//Converts algebraic notation of a square to it's XY coordinated (i.e "a8" -> (0,0), "h1" -> (7,7))
	void rankAndFileToXY(const char* rankAndFile, int& retX, int& retY) const;

	//Returns path reachable by using discrette steps of (sizeX,sizeY)
	//If path is obstructed by an allied piece, the path ends on a square right before it
	//If path is obstructed by enemy piece, path ends on is it's square
	//If path is not obstructed at all, path ends on last location inside board's bounds.
	//Undefined behavior for out of bounds starting location.
	//If no steps can be made, stepCount is 0 and steps data is garbage.
	//Data at steps[stepCount] and after it is garbage
	//Path contains at most 7 points (unobstructed sliding piece movement from the corner, not counting the piece itself)
	Path getPath(int startX, int startY, int stepX, int stepY);

	//returns true if the square (x,y) is attacked by enemies of the input team 
	//(i.e. true for white means that black pieces attack this square), or false otherwise
	bool squareIsAttackedFor(int x, int y, Team team);

	//Returns pair of references to pieces at locations hardcoded based on type of the move supplied.
	//For castling moves, first element of the returned pair is the reference to starting king square
	//Second - target rook square
	//Undefined behavior for non-castling inputs
	//Note that it returns only the references to pieces at target squares, they are not guaranteed to be occupied, or be kings or rooks, castling legality has to be checked separately.
	std::pair<Piece&, Piece&> getPiecesAtCaslingSquares(Move move);

	//Checks if this move is a legal castling move for this board. Returns false for non-castling moves.
	MoveCheckStatus isLegalCastlingMove(Move move);

	//Checks if a move is legal, considering input Move adversarial (checking the validity of inputs, legality of the move, king checks). There're no requirements for src, dst squares or movement types, but hasMoved flag is trusted as-is (since it's filled out by our engine)
	MoveCheckStatus isLegalMove(Move move);

	//Returns true if team's king is checked or false if he's not.
	//Returns false if there is no king for this team on the board
	//Throws an exception if team is NONE
	bool isTeamsKingChecked(Team team);

	//@brief Attempts to apply a move to the current game state. If the attempt was successfull, applies the move and advances game state by one ply.
	//@return First element of the pair is true if move was successfully applied (due to being legal or legality check skipped), or false otherwise.
	//Second element of the pair returns the corresponding status
	std::pair<bool, MoveCheckStatus> tryApplyMove(Move m, bool skipValidation = false);
};