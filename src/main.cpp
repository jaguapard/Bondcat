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
					state.applyMove(m, false);

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