#pragma once
#include <mutex>
#include "Move.h"
#include <vector>
#include <array>
#include <SDL3/SDL.h>

struct PieceTexture
{
	Team team;
	PieceType type;
	SDL_Texture* tex;
	float w, h;
};

struct Globals
{
	static inline std::mutex tablebase_mutex;
	static void init();
	static void initGUI(SDL_Renderer* rend);
	//static const std::vector<Move>& getPieceMoveCandidates(Square from, PieceType type);

	//Basic piece movements, not considering any obstructions, checks or en passant captures.
	//WARNING: pawn moves are only moves, not captures (i.e. one or two steps forward depending on rank)! 
	//For other pieces, moves == captures.
	//uint64_t at index i indicates whether a corresponding piece at board square i can move into board square j
	//Thus, a question: can bishop at index 36 move to square 21 is answered by expression: bishopMoves[36] & (1ull << 21) != 0
	static inline std::array<uint64_t, 64> kingMoves, queenMoves, rookMoves, bishopMoves, knightMoves, blackPawnMoves, whitePawnMoves, blackPawnCaptures, whitePawnCaptures;
	//first index is origin index, second is direction index. Contains bit mask of sliding piece movement for that direction and origin
	static inline std::array<std::array<uint64_t, 8>, 64> directionRaysByOrigin;

	static inline std::vector<PieceTexture> pieceTextures;
};