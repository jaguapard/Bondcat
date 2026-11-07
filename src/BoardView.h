#pragma once
#include "GameState.h"
#include <SDL3/SDL.h>

class BoardView
{
public:
	BoardView(SDL_FRect boardRenderRect, int divisionsW, int divisionsH);
	SDL_Point squareAt(SDL_Point p);
private:
	SDL_FRect renderRect;
	int divisionsW, divisionsH;
	SDL_Color darkSquaresColor, brightSquaresColor;
};