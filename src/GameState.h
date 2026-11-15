#pragma once
#include <array>
#include <string>
#include "Piece.h"
#include "Move.h"
#include <vector>
#include <optional>
#include <assert.h>
#include <chrono>
#include <map>
#include "StaticArray.h"
#include "FEN_Codec.h"
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
	float evaluation = 0;
	std::optional<int> mateIn;
	
	uint64_t nodesSearched = 0;
	double searchSeconds, nodesPerSecond;
	std::string toUCI() const;
};

struct CompressedPosition
{
	CompressedPosition() = default;

	std::array<uint64_t, 8> bits;
	CompressedPosition(const std::array<Piece, 64>& board);
	bool operator<(const CompressedPosition& other) const;
};

struct MoveRollbackData
{
	CastlingRights castlingRights; //castling rights before the move
	std::optional<Square> enpassant; //en passant square before the move
	uint32_t halfmoves, fullmoves;
	std::array<Piece, 64> board;
};

struct MoveResult
{
	bool applied;
	MoveCheckStatus checkStatus;
	MoveRollbackData rollbackData;
};

struct CastlingSquares
{
	Square kingStart, kingEnd, rookStart, rookEnd;
	CastlingSquares(MoveType type);
};
typedef StaticArray<Move, 4100> MoveList;
struct GameState
{
public:
	SearchReturn findBestMove(SearchParams params);
private:
	SearchReturn findBestMoveInner(int remainingDepth, float alpha, float beta, const SearchParams& params);
public:
	std::array<Piece, 64> board;
	CastlingRights castlingRights;
	bool whitesTurn;
	int halfmoves, fullmoves;
	std::optional<Square> enpassant;

	std::optional<GameStatus> gameStatus;
	std::map<CompressedPosition, int> seenPositions;

	static GameState newGame();

	void updateStatus();

	//Resets the game state to the standard starting position
	void reset();

	void getAllLegalMoves(MoveList& ret);
	void getAllLegalMovesForPieceAt(Square sq, MoveList& ret);

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

	MoveResult tryMakeMove(const Move& move); //Applies move if it is legal 
	MoveRollbackData makeMove(const Move& move); //Applies move without validation
	void rollbackMove(const MoveRollbackData& rollbackData); //rolls back move. Only supports rolling back the last move applied.
};