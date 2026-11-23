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
#include "PreallocatedArray.h"
#include "FEN_Codec.h"

#include "Board.h"

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

struct StateStackEntry
{
	CastlingRights castlingRights;
	uint64_t enpassant;
	uint32_t halfmoves, fullmoves;
	bool whitesTurn;
	Board board;

	bool operator==(const StateStackEntry& rhs) const
	{
		return whitesTurn == rhs.whitesTurn && castlingRights == rhs.castlingRights && board == rhs.board && enpassant == rhs.enpassant;
	}
};

struct MoveResult
{
	bool applied;
	MoveCheckStatus checkStatus;
	uint32_t plyIndex;
};

struct CastlingSquares
{
	Square kingStart, kingEnd, rookStart, rookEnd;
	CastlingSquares(MoveType type);
};


typedef PreallocatedArray<Move, 280> MoveList; //should probably fit all legal moves in classical, right?
struct DerivedData
{
	//std::array<uint64_t, 64> motionMatrix; //can piece at square i move into square j?
	uint64_t squaresAttackedByBlack, squaresAttackedByWhite;
	uint32_t whiteKingIndex, blackKingIndex;
	bool whiteChecked, blackChecked;
	PreallocatedArray<Move, 280> legalMoveList;
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

	//Returns true if square is attacked for: white if forWhite == true, else for black
	bool isSquareAttacked(uint64_t sqInd, bool forWhite);
public:
	PreallocatedArray<StateStackEntry, 10240> stateStack;

	__declspec(property(get = getTopStateStackEntry)) StateStackEntry& st;
	__declspec(property(get = getCurrentTurnTeam)) Team currTurnTeam;

	Team getCurrentTurnTeam();
	StateStackEntry& getTopStateStackEntry();
	std::optional<GameStatus> gameStatus = GameStatus::ONGOING;

	Piece getPieceAt(Square s);

	//Sets the game's termination status based on it's current position (if useDD is null) or using existing derived data (if it is not null)
	//The latter case is useful when the status needs to be updated while the current game state's data is already available
	//(i.e. reusing the DD after generating legal moves)
	void updateStatus(const DerivedData* useDD = nullptr);
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
	//Applies the move without validation. If rememberPosition is false, the resulting position will not be remembered for the purposes of 3-fold repetition rule.
	//Returns the ply index of the move made (1 on first white's move in the start of the game, increments by one for each ply)
	//This index is the same as state stack index of the position after the move is applied
	uint32_t makeMove(const Move& move, bool rememberPosition = true);

	std::string moveToAlgebraicNotation(Move move);

	void rollbackMove(bool forgetPosition = true); //rolls back move. Only supports rolling back the last move applied. if forgetPosiition is false, will not erase the position from 3-fold repetition table
};