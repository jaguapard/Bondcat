#include "GUI_Game.h"
#include <iostream>
#include "globals.h"

GUI_Game::GUI_Game(SDL_Renderer* rend, int w, int h)
{
	this->rend = rend;
	this->w = w;
	this->h = h;
	this->gameState = GameState::newGame();
	this->initialState = gameState;

	float minDim = std::min(w, h);
	this->boardView = BoardView({ 0,0,minDim,minDim }, 8, 8);
}

void GUI_Game::loadFEN(const std::string& fen)
{
	this->gameState = this->initialState = GameState::fromFEN(fen);
}

std::string GUI_Game::dumpFEN()
{
	return gameState.toFEN();
}

void GUI_Game::onClick(SDL_Event event)
{
	using enum GUI_Game_State;
	if (engineDelay.has_value()) return; //disallow click handling while engine is to move

	SDL_Point p;
	p.x = event.motion.x;
	p.y = event.motion.y;
	
	auto clickedAtSquare = boardView.squareAt(p);
	if (clickedAtSquare.outOfBounds()) return;

	if (this->guiState == CHOSE_PIECE)
	{
		if (gameState.getPieceAt(clickedAtSquare).type != PieceType::EMPTY)
		{
			this->setActiveSquare(clickedAtSquare);
			this->guiState = PIECE_CHOSEN;
		}
	}

	else if (guiState == PIECE_CHOSEN)
	{
		bool triesToCastle;
		if (gameState.st.whitesTurn) triesToCastle = gameState.st.castlingRights.whiteKingside || gameState.st.castlingRights.whiteQueenside;
		else triesToCastle = gameState.st.castlingRights.blackKingside || gameState.st.castlingRights.blackQueenside;
		{
			Piece movingPiece = gameState.getPieceAt(*boardDrawCtx.activeSquare);
			Piece targetPiece = gameState.getPieceAt(clickedAtSquare);
			triesToCastle &= ((movingPiece.type == PieceType::KING && targetPiece.type == PieceType::ROOK) || (movingPiece.type == PieceType::ROOK && targetPiece.type == PieceType::KING)) && movingPiece.team == targetPiece.team;
		}
		if (gameState.getPieceAt(clickedAtSquare).team == gameState.currTurnTeam && !triesToCastle) this->setActiveSquare(clickedAtSquare);
		else
		{
			Square from(boardDrawCtx.activeSquare->x, boardDrawCtx.activeSquare->y);
			Square to(clickedAtSquare.x, clickedAtSquare.y);
			Move m(from, to, MoveType::MOVE_TO_SQUARE);

			Piece movingPiece = gameState.getPieceAt(m.from);
			Piece targetPiece = gameState.getPieceAt(m.to);
			bool onLastRank = m.to.y == (gameState.st.whitesTurn ? 0 : 7);
			bool isPawn = movingPiece.type == PieceType::PAWN;
			if (isPawn && onLastRank) {
				m.type = MoveType::MOVE_AND_PROMOTE_TO_QUEEN;
			}

			std::vector<Move> moveCandidates = { m };
			MoveResult result;
			result.applied = false;
			if (triesToCastle)
			{
				Square rookSq = movingPiece.type == PieceType::ROOK ? m.from : m.to;
				Square kingSq = movingPiece.type == PieceType::ROOK ? m.to : m.from;
				for (auto ct : { MoveType::WHITE_KINGSIDE_CASTLE, MoveType::WHITE_QUEENSIDE_CASTLE, MoveType::BLACK_KINGSIDE_CASTLE, MoveType::BLACK_QUEENSIDE_CASTLE })
				{
					CastlingSquares sqrs(ct);
					if (kingSq == sqrs.kingStart && rookSq == sqrs.rookStart) moveCandidates.push_back(Move({}, {}, ct));
				}
			}

			MoveResult res;
			res.applied = false;
			for (auto& it : moveCandidates)
			{
				res = gameState.tryMakeMove(it);
				if (res.applied)
				{
					this->appliedMoves.push_back(it);
					this->postMove();
					break;
				}
			}
			if (res.applied) //if not a single move out of candidates got applied, dump fail reason
			{
				this->guiState = CHOSE_PIECE;
				this->clearActiveSquareSelection();
				if (engineEnabled && !engineDelay.has_value()) engineDelay = 1; //postpone engine delay by 1 frame so the human's move is drawn before engine fires
			}
			else std::cout << StringifyEnum(res.checkStatus) << "\n";
		}
	}
}

void GUI_Game::update()
{
	using enum GUI_Game_State;
	if (guiState == CHOSE_PIECE && engineEnabled && engineTeam == gameState.currTurnTeam)
	{
		if (!engineDelay.has_value()) engineDelay = 1; //this is required for engine playing white
		else if (*engineDelay == 0)
		{
			engineDelay = std::nullopt;
			SearchParams params;
			params.maxDepth = 5;

			std::cout << "Engine thinking...";
			SearchReturn search = this->gameState.findBestMove(params);
			std::cout << " done in " << search.searchSeconds << " sec.\n";

			std::cout << search.toUCI() << "\n";
			if (search.bestMove.from.inBounds()) std::cout << "Engine chose: " << StringifyEnum(gameState.getPieceAt(search.bestMove.from).type) << " " << search.bestMove.toUCI() << "\n";
			MoveResult result = gameState.tryMakeMove(search.bestMove);
			if (!result.applied)
			{
				std::cerr << "Engine malfunction: chose illegal move: " << StringifyEnum(result.checkStatus) << "\n";
				__debugbreak();
			}
			else
			{
				this->appliedMoves.push_back(search.bestMove);
				this->postMove();
			}
		}
		else (*engineDelay)--;
	}

	gameState.updateStatus();
	if (gameState.gameStatus && *gameState.gameStatus != GameStatus::ONGOING && guiState != GAME_OVER)
	{
		std::cout << "GAME OVER: " << StringifyEnum(*gameState.gameStatus) << "\n";
		guiState = GAME_OVER;
	}

}

void GUI_Game::draw()
{
	boardDrawCtx.gameState = &gameState;
	boardDrawCtx.textures = &Globals::pieceTextures;
	boardDrawCtx.greenSquares.clear();
	boardDrawCtx.yellowSquares.clear();
	if (this->attackedSquareDisplay)
	{
		DerivedData dd = this->gameState.deriveStateData();
		uint64_t greenSquares = dd.squaresAttackedByWhite;
		uint64_t yellowSquares = dd.squaresAttackedByBlack;
		if (!gameState.st.whitesTurn) std::swap(greenSquares, yellowSquares);
		for (auto it : Square::all_inBounds())
		{
			uint64_t b = 1ull << it.toIndex();
			//if (greenSquares & b) boardDrawCtx.greenSquares.insert(it);
			if (yellowSquares & b) boardDrawCtx.yellowSquares.insert(it);
		}
	}
	boardView.draw(rend, boardDrawCtx);
}

void GUI_Game::handleEvent(SDL_Event event)
{
	if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == 1)
	{
		this->onClick(event);
	}
	if (event.type == SDL_EVENT_KEY_DOWN)
	{
		if (event.key.scancode == SDL_SCANCODE_H) this->attackedSquareDisplay ^= 1;
		if (event.key.scancode == SDL_SCANCODE_E) this->engineEnabled ^= 1;
		if (event.key.scancode == SDL_SCANCODE_T)
		{
			switch (engineTeam)
			{
			case Team::WHITE:this->engineTeam = Team::BLACK; break;
			case Team::BLACK: this->engineTeam = Team::WHITE; break;
			default: break;
			}
		}
	}
}

void GUI_Game::postMove()
{
	GameState copy = initialState;
	std::stringstream ss;
	for (auto& it2 : appliedMoves)
	{
		if (copy.st.whitesTurn) std::cout << copy.st.fullmoves << ". ";
		std::cout << copy.moveToAlgebraicNotation(it2) << " ";
		if (!copy.st.whitesTurn) std::cout << "\n";
		copy.makeMove(it2);
	}
	std::cout << "\n" << dumpFEN() << "\n";
}

void GUI_Game::setActiveSquare(Square sq)
{
	boardDrawCtx.activeSquare = sq;
	boardDrawCtx.circlesAt.clear();

	DerivedData dd = this->gameState.deriveStateData();
	for (auto& move : dd.legalMoveList)
	{
		if (move.from != *boardDrawCtx.activeSquare) continue;
		boardDrawCtx.circlesAt.insert(move.to);
	}
}

void GUI_Game::clearActiveSquareSelection()
{
	boardDrawCtx.activeSquare = std::nullopt;
	boardDrawCtx.circlesAt.clear();
}
