#include <iostream>
#include <string>
#include <sstream>
#include <array>
#include <assert.h>
#include <vector>
#include <optional>
#include <unordered_map>
#include "globals.h"

#include <tbprobe.h>

#include <SDL3/SDL_image.h>

#include "GameState.h"
#include "Piece.h"
#include "BoardView.h"

#include <SDL3/SDL.h>

#pragma comment(lib,"SDL3.lib")
#pragma comment(lib,"SDL3_image.lib")
#undef main

#include <string_view>
#include "BondcatException.h" 
#include "GUI_Game.h"

void UCI()
{
	GameState gameState;
	std::string idString = "id name Bondcat v0.4: Kestrel\nid author jaguapard\nuciok\n";
	std::cout << idString;
	while (true)
	{
		std::string cmd;
		std::getline(std::cin, cmd);

		if (cmd == "uci")
		{
			std::cout << idString;
		}

		if (cmd.substr(0, 7) == "isready") std::cout << "readyok\n";
		if (cmd.substr(0, 10) == "ucinewgame") gameState.reset();
		if (cmd.substr(0, 8) == "position")
		{
			std::stringstream ss;
			ss.str(cmd.substr(9));
			std::string acc, tmp;

			while (ss >> tmp)
			{
				if (tmp == "moves") break;
				acc += tmp + " ";
			}

			if (acc.substr(0, 8) == "startpos") gameState.reset();
			else if (acc.substr(0, 3) == "fen") gameState = GameState::fromFEN(acc.substr(4));

			std::string moveStr;
			int moveCounter = 0;
			while (ss >> moveStr)
			{
				std::string src = moveStr.substr(0, 2);
				std::string dst = moveStr.substr(2, 2);

				Move move(Square(src), Square(dst), MoveType::MOVE_TO_SQUARE);
				Piece srcPiece = gameState.getPieceAt(move.from);
				Piece dstPiece = gameState.getPieceAt(move.to);
				if (moveStr.length() > 4)
				{
					if (moveStr[4] == 'q') move.type = MoveType::MOVE_AND_PROMOTE_TO_QUEEN;
					if (moveStr[4] == 'n') move.type = MoveType::MOVE_AND_PROMOTE_TO_KNIGHT;
					if (moveStr[4] == 'r') move.type = MoveType::MOVE_AND_PROMOTE_TO_ROOK;
					if (moveStr[4] == 'b') move.type = MoveType::MOVE_AND_PROMOTE_TO_BISHOP;
				}

				if (srcPiece.type == PieceType::KING && abs(move.from.x - move.to.x) > 1)
				{
					if (moveStr == "e1g1") move.type = MoveType::WHITE_KINGSIDE_CASTLE;
					else if (moveStr == "e1c1") move.type = MoveType::WHITE_QUEENSIDE_CASTLE;
					else if (moveStr == "e8g8") move.type = MoveType::BLACK_KINGSIDE_CASTLE;
					else if (moveStr == "e8c8") move.type = MoveType::BLACK_QUEENSIDE_CASTLE;
					else throw BondcatException("UCI wrong castling: " + moveStr);
				}

				MoveResult result = gameState.tryMakeMove(move);
				if (!result.applied) throw BondcatException((std::stringstream() << "UCI illegal move: " << moveStr << " at position " << moveCounter << ": " << StringifyEnum(result.checkStatus)).str());
				gameState.updateStatus();
				moveCounter++;
			}
		}

		if (cmd.substr(0, 2) == "go")
		{
			SearchParams params;
			params.maxDepth = 5;

			std::string token = "";
			std::stringstream ss;
			ss.str(cmd.substr(3));
			while (ss >> token) if (token == "depth") ss >> params.maxDepth;

			SearchReturn search = gameState.findBestMove(params);
			std::cout << search.toUCI() << "\n";
		}
		if (cmd == "quit") return;
	}
}

struct Sentinel
{
	~Sentinel()
	{
		tb_free();
	}
};
int main(int argc, char** argv)
{
	Globals::init();
	Sentinel tablebaseNannyDontRemoveMe;
	bool tablebaseLoaded = [&]()
	{
		const char* tableBasePath = getenv("BONDCAT_ENDGAME_TABLES_PATH");
		if (!tableBasePath) { std::cerr << "BONDCAT_ENDGAME_TABLES_PATH environment variable not found" << "\n"; return false; }
		if (!tb_init(tableBasePath) || TB_LARGEST < 3) return false;
		return true;
	}();
	if (tablebaseLoaded) std::cout << "Successfully loaded tablebases for up to " << TB_LARGEST << " pieces.\n";
	else std::cerr << "Failed to initialize tablebases. Tablebase probing will be disabled.\n";
	
	try
	{
		bool engineEnabled = true;
		std::cout << "Select mode: gui or uci or fen (string) to import a position in GUI mode: ";
		std::string line;
		std::getline(std::cin, line);

		if (line == "uci")
		{
			UCI();
			return 0;
		}

		SDL_Window* wnd;
		SDL_Renderer* rend;
		int w = 1920, h = 1080;
		SDL_CreateWindowAndRenderer("Bondcat the Chess Engine", w, h, 0, &wnd, &rend);
		SDL_SetRenderVSync(rend, 1);
		Globals::initGUI(rend);
		SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

		std::optional<GUI_Game> guiGameInProgress = GUI_Game(rend, w, h);
		//state = GameState::fromFEN("rnb1kbnr/pppp1ppp/4p3/8/3Pq3/4B1P1/PPP2P1P/RN1QKBNR b KQkq - 1 4"); //queen blundering
		//state = GameState::fromFEN("rnbqkbnr/pppp3p/4p3/5pp1/5P2/5NPB/PPPPP2P/RNBQK2R w KQkq - 0 5"); //castling test
		//state = GameState::fromFEN("4k3/2p3P1/p2pb3/5p2/1P2p3/B1N3P1/P2P4/R3K2R w KQkq - 0 27");
		//state = GameState::fromFEN("8/2p5/pk1p2R1/Q7/1P6/B1N1P1P1/R7/4K3 b KQkq - 2 35");
		//state = GameState::fromFEN("1nbk2nr/prppqppp/8/1p1N4/3PP2P/P4P2/1P1Q2P1/2KR1BNR b kq - 4 14");
		//state = GameState::fromFEN("1k4n1/6PP/4R3/1p6/1P6/8/P1K5/8 b kq - 0 56");
		//state = GameState::fromFEN("8/8/8/2K2k2/8/8/8/8 w - - 0 1"); //king vs king test
		//state = GameState::fromFEN("8/8/1k6/5N2/1r6/6B1/5K2/8 b - - 0 1"); //5-piece endgame test
		//state = GameState::fromFEN("7k/5K2/6Q1/8/8/8/8/8 w - - 0 1"); //checkmate detection test
		//state = GameState::fromFEN("rnb1kb1r/pp1ppppp/8/2qpP3/5P2/5N2/PPPPQ1PP/R1B1KB1R b KQk - 1 7"); //another queen blunder
		//state = GameState::fromFEN("rnb1kb1r/pp1ppppp/8/3pP3/3P1P2/5N2/PPq1Q1PP/R1B1KB1R b KQk d3 0 8"); //direct followup of queen blunder
		//state = GameState::fromFEN("rnb1k2r/pppp1p1p/3b1q2/3P1p2/3N4/6PB/PPP4P/R1BQK2R b KQkq - 0 11"); //wrong checks
		//state = GameState::fromFEN("8/kQ6/2K5/8/8/8/8/8 b - - 6 61"); //mate garbage
		//state = GameState::fromFEN("8/8/8/2k5/4b1p1/5q2/p1p4K/8 b - - 97 533"); //mate avoidance, here black can mate with Qg2# or Qf2# but choses to delay
		//state = GameState::fromFEN("6nn/p1p1p1p1/1p1p1p1p/8/8/2k5/K1p5/8 b - - 0 1"); //underpromotion test: promoting to queen is stalemate. More pieces to avoid endgame lookup. ...c1=R, then king only has Ka3, Ra1#
		//state = GameState::fromFEN("r6k/3n4/3n4/8/bb1R2pp/8/3N4/K2B4 w - - 0 1"); //Rook move test
		//state = GameState::fromFEN("rb5k/3n4/3n3Q/8/bb1B1Bpp/8/KR1N4/3B4 w - - 0 1"); //bishop move test
		//state = GameState::fromFEN("7k/8/8/7R/8/8/8/K7 w - - 0 1");
		if (line.substr(0,3) == "fen")
		{
			guiGameInProgress->loadFEN(line.substr(4));
			guiGameInProgress->engineEnabled = false;
		}

		
		while (true)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (guiGameInProgress) guiGameInProgress->handleEvent(event);
			}

			SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
			SDL_RenderClear(rend);
			if (guiGameInProgress)
			{
				guiGameInProgress->update();
				guiGameInProgress->draw();
			}
			SDL_RenderPresent(rend);
		}

		//auto legal = state.getAllLegalMoves();
		return 0;
	}
	catch (const BondcatException& e)
	{
		std::cerr << e.asString() << "\n";
		return -1;
	}
	catch (const std::exception& e)
	{
		std::cerr << "std exception: " << e.what() << "\n";
		return -1;
	}
}