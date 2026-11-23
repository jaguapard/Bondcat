#include "BoardView.h"

BoardView::BoardView(SDL_FRect boardRenderRect, int divisionsW, int divisionsH)
{
	this->renderRect = boardRenderRect;
	this->divisionsW = divisionsW;
	this->divisionsH = divisionsH;
	this->darkSquaresColor = { 118,150,86,255 };
	this->brightSquaresColor = { 238,238,210,255 };
}

Square BoardView::squareAt(SDL_Point p)
{
	float relX = p.x - renderRect.x;
	float relY = p.y - renderRect.y;
	int8_t sx = relX / (renderRect.w / divisionsW);
	int8_t sy = relY / (renderRect.h / divisionsH);
	return { sx,sy };
}

void BoardView::draw(SDL_Renderer* rend, BoardDrawCtx& ctx)
{
	float squareW = this->renderRect.w / this->divisionsW;
	float squareH = this->renderRect.h / this->divisionsH;
	float minDim = std::min(squareW, squareH);

	for (int y = 0; y < 8; ++y)
	{
		for (int x = 0; x < 8; ++x)
		{
			Square sq(x, y);
			SDL_FRect rect;
			rect.x = squareW * x;
			rect.y = squareH * y;
			rect.w = squareW;
			rect.h = squareH;

			SDL_Color squareColor = ((x ^ y) % 2) ? this->darkSquaresColor : this->brightSquaresColor;
			SDL_SetRenderDrawColor(rend, squareColor.r, squareColor.g, squareColor.b, squareColor.a);
			SDL_RenderFillRect(rend, &rect);

			if (ctx.activeSquare && ctx.activeSquare->x == x && ctx.activeSquare->y == y)
			{
				SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
				SDL_RenderFillRect(rend, &rect);
				//SDL_FRect active = rect;
				//rect.w = 
			}

			if (ctx.gameState)
			{
				if (ctx.textures)
				{
					Piece piece = ctx.gameState->getPieceAt(Square(x, y));
					if (piece.type != PieceType::EMPTY)
					{
						for (auto& it : *ctx.textures)
							if (it.team == piece.team && it.type == piece.type)
							{
								SDL_FRect pieceRect;
								float scale = minDim * 0.9 / std::max(it.w, it.h);
								float scaledW = it.w * scale;
								float scaledH = it.h * scale;
								pieceRect.x = rect.x + (minDim - scaledW) / 2;
								pieceRect.y = rect.y + (minDim - scaledH) / 2;
								pieceRect.w = it.w * scale;
								pieceRect.h = it.h * scale;
								SDL_RenderTexture(rend, it.tex, nullptr, &pieceRect);
							}
					}
				}

				if (ctx.greenSquares.find(sq) != ctx.greenSquares.end())
				{
					SDL_SetRenderDrawColor(rend, 255, 0, 0, 64);
					SDL_RenderFillRect(rend, &rect);
				}
				if (ctx.yellowSquares.find(sq) != ctx.yellowSquares.end())
				{
					SDL_SetRenderDrawColor(rend, 255, 255, 0, 64);
					SDL_RenderFillRect(rend, &rect);
				}
				if (ctx.circlesAt.find(sq) != ctx.circlesAt.end())
				{
					SDL_SetRenderDrawColor(rend, 160, 160, 160, 255);
					SDL_FRect small = rect;
					float smallScale = 0.25;
					small.w *= smallScale;
					small.h *= smallScale;
					small.x += (1 - smallScale) / 2 * rect.w;
					small.y += (1 - smallScale) / 2 * rect.h;

					SDL_RenderFillRect(rend, &small);
				}
			}

		}
	}
}
