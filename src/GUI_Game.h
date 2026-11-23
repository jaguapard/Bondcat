#pragma once
#include "Move.h"
#include <vector>
#include "GameState.h"
#include <SDL3/SDL.h>
#include "BoardView.h"

class GUI_Game
{
public:
	GUI_Game(SDL_Renderer* rend, int w, int h);
	void loadFEN(const std::string& fen);
	std::string dumpFEN();

	void update();
	void draw();
	void handleEvent(SDL_Event event);

	Team engineTeam = Team::BLACK;
	bool engineEnabled = true;
private:
	SDL_Renderer* rend;
	int w, h;
	std::vector<Move> appliedMoves;
	GameState gameState, initialState;
	BoardView boardView;
	BoardDrawCtx boardDrawCtx;
	bool attackedSquareDisplay = false;
	
	std::optional<int> engineDelay;
	enum class GUI_Game_State
	{
		CHOSE_PIECE,
		PIECE_CHOSEN,
		GAME_OVER,
	};

	GUI_Game_State guiState = GUI_Game_State::CHOSE_PIECE;

	void onClick(SDL_Event event);
	void postMove();
	void setActiveSquare(Square sq);
	void clearActiveSquareSelection();
};