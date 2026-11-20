#pragma once
#include "GameState.h"
#include <SDL3/SDL.h>
#include "Square.h"
#include "globals.h"

struct BoardDrawCtx
{
	std::optional<Square> activeSquare;
	std::vector<Square> circlesAt;
	const std::vector<PieceTexture>* textures = nullptr;
	GameState* gameState = nullptr;
};
class BoardView
{
public:
	BoardView() = default;
	BoardView(SDL_FRect boardRenderRect, int divisionsW, int divisionsH);
	Square squareAt(SDL_Point p);
	void draw(SDL_Renderer* rend, BoardDrawCtx& ctx);
private:
	SDL_FRect renderRect;
	int divisionsW, divisionsH;
	SDL_Color darkSquaresColor, brightSquaresColor;
};