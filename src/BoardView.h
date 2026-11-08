#pragma once
#include "GameState.h"
#include <SDL3/SDL.h>

struct PieceTexture
{
	Team team;
	PieceType type;
	SDL_Texture* tex;
	float w, h;
};

struct BoardDrawCtx
{
	std::optional<SDL_Point> activeSquare;
	std::vector<SDL_Point> circlesAt;
	const std::vector<PieceTexture>* textures = nullptr;
	GameState* gameState = nullptr;
};
class BoardView
{
public:
	BoardView(SDL_FRect boardRenderRect, int divisionsW, int divisionsH);
	SDL_Point squareAt(SDL_Point p);
	void draw(SDL_Renderer* rend, BoardDrawCtx& ctx);
private:
	SDL_FRect renderRect;
	int divisionsW, divisionsH;
	SDL_Color darkSquaresColor, brightSquaresColor;
};