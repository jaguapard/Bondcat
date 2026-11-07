#include "BoardView.h"

BoardView::BoardView(SDL_FRect boardRenderRect, int divisionsW, int divisionsH)
{
	this->renderRect = boardRenderRect;
	this->divisionsW = divisionsW;
	this->divisionsH = divisionsH;
}

SDL_Point BoardView::squareAt(SDL_Point p)
{
	float relX = p.x - renderRect.x;
	float relY = p.y - renderRect.y;
	int sx = relX / (renderRect.w / divisionsW);
	int sy = relY / (renderRect.h / divisionsH);
	return { sx,sy };
}
