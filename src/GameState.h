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

#include "Board.h"

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
	
	uint64_t nodesSearched = 0, depth;
	double searchSeconds, nodesPerSecond;
	bool endgameTablebaseHit;

	std::string toUCI() const;
};

/*
struct CompressedPosition
{
	CompressedPosition() = default;

	std::array<uint64_t, 8> bits;
	//CastlingRights castlingRights;
	CompressedPosition(const Board& board);
	bool operator<(const CompressedPosition& other) const;
};*/

struct MoveRollbackData
{
	CastlingRights castlingRights; //castling rights before the move
	uint64_t enpassant; //en passant square before the move
	uint32_t halfmoves, fullmoves;
	bool whitesTurn;
	Board board;
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
	MoveRollbackData st;

	std::optional<GameStatus> gameStatus;
	std::map<Board, int> seenPositions;

	static GameState newGame();

	void updateStatus();

	void reset();

	static GameState fromFEN(const std::string& fen);
	std::string toFEN();

	MoveResult tryMakeMove(const Move& move); //Applies move if it is legal 
	MoveRollbackData makeMove(const Move& move); //Applies move without validation
	void rollbackMove(const MoveRollbackData& rollbackData); //rolls back move. Only supports rolling back the last move applied.
};