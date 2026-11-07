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


struct PieceTexture
{
	Team team;
	PieceType type;
	SDL_Texture* tex;
	float w, h;
};


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

	
	while (true)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{

		}

		SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
		SDL_RenderClear(rend);
		for (int y = 0; y < 8; ++y)
		{
			for (int x = 0; x < 8; ++x)
			{
				std::array<Uint8, 3> dark = { 118,150,86 };
				std::array<Uint8, 3> bright = { 238,238,210  };
				auto c = ((x ^ y) % 2) ? dark : bright;
				SDL_SetRenderDrawColor(rend, c[0], c[1], c[2], 255);

				SDL_FRect rect;
				rect.x = side * x;
				rect.y = side * y;
				rect.w = side;
				rect.h = side;
				SDL_RenderFillRect(rend, &rect);

				Piece piece = state.getPieceAt(x, y);
				if (piece.type != PieceType::EMPTY)
				{
					for (auto& it : pieceTextures)
						if (it.team == piece.team && it.type == piece.type)
						{
							SDL_FRect pieceRect;
							float scale = side * 0.9 / std::max(it.w, it.h);
							float scaledW = it.w * scale;
							float scaledH = it.h * scale;
							pieceRect.x = rect.x + (side - scaledW) / 2;
							pieceRect.y = rect.y + (side - scaledH) / 2;
							pieceRect.w = it.w * scale;
							pieceRect.h = it.h * scale;
							SDL_RenderTexture(rend, it.tex, nullptr, &pieceRect);
						}
				}
			}
		}
		SDL_RenderPresent(rend);
	}

	//auto legal = state.getAllLegalMoves();
	return 0;
}