#include <iostream>
#include <string>
#include <sstream>
#include <array>
#include <assert.h>
#include <vector>
#include <optional>
#include <unordered_map>

#include <SDL3/SDL_image.h>

#include "GameState.h"
#include "Piece.h"
#include "BoardView.h"

#include <SDL3/SDL.h>

#pragma comment(lib,"SDL3.lib")
#pragma comment(lib,"SDL3_image.lib")
#undef main

#include <string_view>

std::string_view moveCheckStatusToString(MoveCheckStatus status)
{
	using enum MoveCheckStatus;

	switch (status)
	{
		// General
	case LEGAL:
		return "Legal move.";

	case SRC_OUT_OF_BOUNDS:
		return "The source square is outside the chessboard.";

	case DST_OUT_OF_BOUNDS:
		return "The destination square is outside the chessboard.";

	case STATIONARY_MOTION:
		return "The source and destination squares are the same.";

	case TURN_TEAM_MISMATCH:
		return "Attempted to move a piece belonging to the other side.";

	case MOVING_EMPTY_SQUARE:
		return "Attempting to move empty square.";

	case MOVING_OBSTRUCTED_BY_ALLY:
		return "Destination square is occupied by an allied piece.";

	case TRYING_TO_CAPTURE_KING:
		return "A king cannot be captured.";

		// Castling
	case CASTLING_CHECK_ON_NONCASTLING_MOVE:
		return "Castling validation was requested for a non-castling move.";

	case CASTLING_RIGHTS_ABSENT:
		return "The corresponding team has no castling right for this castle.";

	case CASTLING_NOT_KING:
		return "The expected king square does not contain a king.";

	case CASTLING_NOT_ROOK:
		return "The expected rook square does not contain a rook.";

	case CASTLING_KING_MOVED:
		return "The king has already moved, so castling is not allowed.";

	case CASTLING_ROOK_MOVED:
		return "The rook has already moved, so castling is not allowed.";

	case CASTLING_KING_ROOK_TEAM_MISMATCH:
		return "The castling king and rook belong to different sides.";

	case CASTLING_TEAM_MISMATCHED_KING_TEAM:
		return "The king does not belong to the side attempting to castle.";

	case CASTLING_ATTEMPTED_ON_OPPONENTS_TURN:
		return "Castling attempted on opponent's turn.";

	case CASTLING_PATH_OBSTRUCTED:
		return "Castling path is obstructed by another piece.";

	case CASTLING_PATH_PASSES_THROUGH_CHECK:
		return "Castling path passes out of, through, or into check.";

		// Knight
	case MOVING_BAD_KNIGHT_JUMP:
		return "A knight must move in an L-shape: two squares in one direction and one perpendicular.";

		// Pawn
	case PROMOTION_OF_NON_PAWN_PIECE:
		return "Attempted to promote non-pawn piece.";

	case PAWN_PROMOTION_ON_NONLAST_RANK:
		return "A pawn can only promote when reaching the last rank.";

	case PAWN_PROMOTION_ELIDED:
		return "A pawn reaching the last rank must promote.";

	case PAWN_BACKWARDS_MOVE:
		return "A pawn must move forward toward the opponent's side.";

	case MOVED_PAWN_STEP_TOO_FAR:
		return "A pawn that has already moved can only advance one square.";

	case NOT_MOVED_PAWN_STEP_TOO_FAR:
		return "A pawn on its starting square can advance at most two squares.";

	case PAWN_MORE_THAN_ONE_FILE:
		return "A pawn cannot move more than one file sideways.";

	case PAWN_DIAGONAL_MOVE_ON_FIRST_2MOVE:
		return "A pawn cannot move two squares diagonally.";

	case PAWN_WRONG_ENPASSANT:
		return "Invalid en passant capture. An en passant capture must be a one-square diagonal move.";

	case PAWN_WRONG_ENPASSANT_2:
		return "The en passant target square must be reached with a one-square diagonal pawn move.";

	case PAWN_FILE_MOVE_ON_NON_CAPTURE:
		return "A pawn cannot move diagonally to an empty square unless capturing en passant.";

	case PAWN_NO_FILE_MOVE_ON_CAPTURE:
		return "A pawn must capture diagonally.";

		// King
	case KING_TOO_FAR:
		return "A king can only move one square in any direction, except when castling.";

		// Rook, bishop, and queen
	case MOVING_SLIDING_PATHS_EXIST_BUT_NONE_MATCH:
		return "Sliding piece path exists but none match (this usually means that chosen square is on the right path, but obstructed).";

		// Final king safety check
	case MOVING_TEAMS_KING_IN_CHECK:
		return "Move puts or leaves moving team's king in check.";
	}

	return "Unknown move-check status.";
}

int main()
{
	SDL_Window* wnd;
	SDL_Renderer* rend;
	int w = 1920, h = 1080;
	SDL_CreateWindowAndRenderer("Chess engine", w, h, 0, &wnd, &rend);
	SDL_SetRenderVSync(rend, 1);

	auto state = GameState::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); //starting position
	float side = std::min(w, h) / 8.0;

	float minDim = std::min(w, h);
	BoardView boardView({ 0,0,minDim,minDim }, 8, 8);
	std::vector<PieceTexture> pieceTextures;
	{
		std::pair<PieceType,std::string> paths[] = {
			{ PieceType::PAWN, "pawn" },
			{ PieceType::ROOK, "rook" },
			{ PieceType::KING, "king" },
			{ PieceType::QUEEN, "queen" },
			{ PieceType::BISHOP, "bishop" },
			{ PieceType::KNIGHT, "knight" },
		};
		for (auto& it : paths)
		{
			std::string pth_w = "textures/white_" + it.second + ".png";
			std::string pth_b = "textures/black_" + it.second + ".png";
			SDL_Surface* s1 = IMG_Load(pth_w.c_str());
			SDL_Surface* s2 = IMG_Load(pth_b.c_str());
			
			PieceTexture t;
			t.type = it.first;
			t.team = Team::WHITE;
			t.tex = SDL_CreateTextureFromSurface(rend,s1);
			t.w = s1->w;
			t.h = s1->h;
			pieceTextures.emplace_back(t);
			t.team = Team::BLACK;
			t.tex = SDL_CreateTextureFromSurface(rend,s2);
			t.w = s2->w;
			t.h = s2->h;
			pieceTextures.emplace_back(t);
		}
	}

	BoardDrawCtx boardDrawCtx;
	boardDrawCtx.gameState = &state;
	boardDrawCtx.textures = &pieceTextures;
	while (true)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == 1)
			{
				SDL_Point p;
				p.x = event.motion.x;
				p.y = event.motion.y;
				auto clickedAtSquare = boardView.squareAt(p);
				if (!boardDrawCtx.activeSquare)
				{
					boardDrawCtx.activeSquare = clickedAtSquare;
					boardDrawCtx.circlesAt.clear();
					for (auto& move : state.getAllLegalMovesForPieceAt(boardDrawCtx.activeSquare->x, boardDrawCtx.activeSquare->y))
					{
						boardDrawCtx.circlesAt.push_back({ move.dstX, move.dstY });
					}
				}
				else
				{
					Move m;
					m.type = MoveType::MOVE_TO_SQUARE;
					m.srcX = boardDrawCtx.activeSquare->x;
					m.srcY = boardDrawCtx.activeSquare->y;
					m.dstX = clickedAtSquare.x;
					m.dstY = clickedAtSquare.y;
					
					auto [moveApplied, status] = state.applyMove(m, false);
					if (!moveApplied) std::cout << moveCheckStatusToString(status) << "\n";

					boardDrawCtx.activeSquare = std::nullopt;
					boardDrawCtx.circlesAt.clear();
				}
			}
		}

		SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
		SDL_RenderClear(rend);
		
		boardView.draw(rend, boardDrawCtx);
		SDL_RenderPresent(rend);
	}

	//auto legal = state.getAllLegalMoves();
	return 0;
}