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

struct StateStackEntry
{
	CastlingRights castlingRights;
	uint64_t enpassant;
	uint32_t halfmoves, fullmoves;
	bool whitesTurn;
	Board board;
};

struct MoveResult
{
	bool applied;
	MoveCheckStatus checkStatus;
};

struct CastlingSquares
{
	Square kingStart, kingEnd, rookStart, rookEnd;
	CastlingSquares(MoveType type);
};


typedef StaticArray<Move, 280> MoveList; //should probably fit all legal moves in classical, right?
struct DerivedData
{
	//std::array<uint64_t, 64> motionMatrix; //can piece at square i move into square j?
	uint64_t squaresAttackedByBlack, squaresAttackedByWhite;
	uint32_t whiteKingIndex, blackKingIndex;
	bool whiteChecked, blackChecked;
	StaticArray<Move, 280> legalMoveList;
};

struct DeriveOptions
{
	bool makeLegalMoveList = true;
	bool skipPinChecks = false;
};

struct GameState
{
public:
	GameState();
	SearchReturn findBestMove(SearchParams params);
private:
	SearchReturn findBestMoveInner(int remainingDepth, float alpha, float beta, const SearchParams& params);

	//Returns a bit mask of squares a rook at square ind attacks. This only checks obstructions, not legality of moves.
	//rookAllies are used to filter out allies and paths blocked by them and must be a mask of pieces same team as the rook in question
	uint64_t getSquaresAttackedByRookAt(uint64_t ind, uint64_t rookAllies);

	//Returns a bit mask of squares a bishop at square ind attacks. This only checks obstructions, not legality of moves.
	//bishopAllies are used to filter out allies and paths blocked by them and must be a mask of pieces same team as the bishop in question
	uint64_t getSquaresAttackedByBishopAt(uint64_t ind, uint64_t bishopAllies);
public:
	StaticArray<StateStackEntry, 10240> stateStack;

	__declspec(property(get = getTopStateStackEntry)) StateStackEntry& st;

	StateStackEntry& getTopStateStackEntry();
	std::optional<GameStatus> gameStatus;
	std::map<Board, int> seenPositions;

	Piece getPieceAt(Square s);

	void updateStatus();
	//derives various useful data from current game state, such as:
	//legal moves on this position
	//squares attacked by black/white
	//whether or not a king is in check
	DerivedData deriveStateData(DeriveOptions opts = {});

	static GameState fromFEN(const std::string& fen);
	std::string toFEN();
	static GameState newGame();
	void reset();

	MoveResult tryMakeMove(const Move& move); //Applies move if it is legal 
	void makeMove(const Move& move); //Applies move without validation
	void rollbackMove(); //rolls back move. Only supports rolling back the last move applied.
};