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

std::string StringifyEnum(MoveCheckStatus status)
{
	using enum MoveCheckStatus;
	switch (status)
	{
	case LEGAL: return "Legal move.";
	case LEGALITY_CHECK_SKIPPED: return "Legality check skipped.";
	case SRC_OUT_OF_BOUNDS: return "The source square is outside the chessboard.";
	case DST_OUT_OF_BOUNDS: return "The destination square is outside the chessboard.";
	case STATIONARY_MOTION: return "The source and destination squares are the same.";
	case TURN_TEAM_MISMATCH: return "Attempted to move a piece belonging to the other side.";
	case MOVING_EMPTY_SQUARE: return "Attempted to move empty square.";
	case MOVING_OBSTRUCTED_BY_ALLY: return "Destination square is occupied by an allied piece.";
	case TRYING_TO_CAPTURE_KING: return "A king cannot be captured.";

	case CASTLING_CHECK_ON_NONCASTLING_MOVE: return "Castling validation was requested for a non-castling move.";
	case CASTLING_RIGHTS_ABSENT: return "The corresponding team has no castling right for this castle.";
	case CASTLING_NOT_KING: return "The expected king square does not contain a king.";
	case CASTLING_NOT_ROOK: return "The expected rook square does not contain a rook.";
	case CASTLING_KING_MOVED: return "The king has already moved, so castling is not allowed.";
	case CASTLING_ROOK_MOVED: return "The rook has already moved, so castling is not allowed.";
	case CASTLING_KING_ROOK_TEAM_MISMATCH: return "The castling king and rook belong to different sides.";
	case CASTLING_TEAM_MISMATCHED_KING_TEAM: return "The king does not belong to the side attempting to castle.";
	case CASTLING_ATTEMPTED_ON_OPPONENTS_TURN: return "Castling attempted on opponent's turn.";
	case CASTLING_PATH_OBSTRUCTED: return "Castling path is obstructed by another piece.";
	case CASTLING_PATH_PASSES_THROUGH_CHECK: return "Castling path passes out of, through, or into check.";

	case MOVING_BAD_KNIGHT_JUMP: return "A knight must move in an L-shape: two squares in one direction and one perpendicular.";
	case PROMOTION_OF_NON_PAWN_PIECE: return "Attempted to promote non-pawn piece.";
	case PAWN_PROMOTION_ON_NONLAST_RANK: return "A pawn can only promote when reaching the last rank.";
	case PAWN_PROMOTION_ELIDED: return "A pawn reaching the last rank must promote.";
	case PAWN_BACKWARDS_MOVE: return "A pawn must move forward toward the opponent's side.";
	case MOVED_PAWN_STEP_TOO_FAR: return "A pawn that has already moved can only advance one square.";
	case NOT_MOVED_PAWN_STEP_TOO_FAR: return "A pawn on its starting square can advance at most two squares.";
	case PAWN_MORE_THAN_ONE_FILE: return "A pawn cannot move more than one file sideways.";
	case PAWN_DIAGONAL_MOVE_ON_FIRST_2MOVE: return "An pawn cannot move to a side when moving forward by 2 steps on it's first move.";
	case PAWN_WRONG_ENPASSANT: return "Invalid en passant capture. An en passant capture must be a one-square diagonal move.";
	case PAWN_WRONG_ENPASSANT_2: return "The en passant target square must be reached with a one-square diagonal pawn move.";
	case PAWN_FILE_MOVE_ON_NON_CAPTURE: return "A pawn cannot move diagonally to an empty square unless capturing en passant.";
	case PAWN_NO_FILE_MOVE_ON_CAPTURE: return "A pawn must capture diagonally.";
	case PAWN_GENERAL_MOVEMENT_ILLEGAL: return "Pawn movement fits none of these rules prior to pathing check: one step forward, two steps forward for unmoved, diagonal capture, diagonal en passant capture.";

	case KING_TOO_FAR: return "A king can only move one square in any direction, except when castling.";
	case KING_RUNS_INTO_ANOTHER_KING: return "A king can't move into squares attacked by another king.";
	case MOVING_SLIDING_PATHS_EXIST_BUT_NONE_MATCH: return "Sliding piece path exists but none match (this usually means that chosen square is on the right path, but obstructed).";
	case MOVING_TEAMS_KING_IN_CHECK: return "Move puts or leaves moving team's king in check.";
	}

	return "Unknown move-check status: " + std::to_string(int(status));
}

std::string StringifyEnum(GameStatus status)
{
	using enum GameStatus;
	switch (status)
	{
	case GameStatus::ONGOING: return "Game in-progress.";
	case GameStatus::WHITE_WON_BY_TIME: return "White wins: black ran out of time.";
	case GameStatus::BLACK_WON_BY_TIME: return "Black wins: white ran out of time.";
	case GameStatus::WHITE_WON_BY_CHECKMATE: return "White won by checkmate.";
	case GameStatus::BLACK_WON_BY_CHECKMATE: return "Black won by checkmate.";
	case GameStatus::WHITE_WON_ENEMY_RESIGNED: return "White wins: black resigned.";
	case GameStatus::BLACK_WON_ENEMY_RESIGNED: return "Black wins: white resigned.";
	case GameStatus::DRAW_ACCEPTED: return "Sides agreed to a draw.";
	case GameStatus::DRAW_STALEMATE: return "Stalemate.";
	case GameStatus::DRAW_THREEFOLD_REPETITION: return "Draw by threefold repetition.";
	case GameStatus::DRAW_FIFTY_MOVE_RULE: return "Draw by 50-move rule.";
	default: return "Unknown GameStatus: " + std::to_string(int(status));
	}
}

void UCI()
{
	GameState gameState;
	std::string idString = "id name Bondcat v0.3: Bee\nid author jaguapard\nuciok\n";
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
			else if (acc.substr(0, 3) == "fen") gameState = GameState::fromFEN(acc);

			std::string moveStr;
			while (ss >> moveStr)
			{
				std::string src = moveStr.substr(0, 2);
				std::string dst = moveStr.substr(2, 2);

				Move move(Square(src), Square(dst), MoveType::MOVE_TO_SQUARE);
				Piece& srcPiece = gameState.getPieceAt(move.from);
				Piece& dstPiece = gameState.getPieceAt(move.to);
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

				auto [applied, status] = gameState.tryApplyMove(move);
				if (!applied) throw BondcatException("UCI illegal move: " + StringifyEnum(status));
				gameState.updateStatus();
			}
		}

		if (cmd.substr(0, 2) == "go")
		{
			SearchParams params;
			params.maxDepth = 3;

			std::string token = "";
			std::stringstream ss;
			ss.str(cmd.substr(3));
			while (ss >> token) if (token == "depth") ss >> params.maxDepth;

			SearchReturn mv = gameState.findBestMove(params);

			int evalScore = mv.evaluation * 100;
			int reportedScore = gameState.whitesTurn ? evalScore : -evalScore;
			std::cout << "info score cp " << reportedScore << " nodes " << mv.nodesSearched << " nps " << uint64_t(mv.nodesPerSecond) << "\n";
			std::cout << "bestmove " << mv.bestMove.toUCI() << "\n";
		}
		if (cmd == "quit") return;
	}
}
int main(int argc, char** argv)
{
	Globals::pawnAttacks.resize(64);
	Globals::knightAttacks.resize(64);
	Globals::bishopAttacks.resize(64);
	Globals::rookAttacks.resize(64);
	Globals::queenAttacks.resize(64);
	Globals::kingAttacks.resize(64);
	for (Square from : Square::all_inBounds())
	{
		for (Square to : Square::all_inBounds())
		{
			int dx = to.x - from.x;
			int dy = to.y - from.y;
			int i = from.toIndex();
			uint64_t dstBit = 1ull << (to.toIndex());

			if (from == to) continue;

			Move move{ from,to,MoveType::MOVE_TO_SQUARE };
			if ((std::abs(dx) == 1 && std::abs(dy) == 2) || (std::abs(dx) == 2 && std::abs(dy) == 1)) Globals::knightAttacks[i].push_back(move);
			if (abs(dx) == abs(dy))
			{
				Globals::bishopAttacks[i].push_back(move);
				Globals::queenAttacks[i].push_back(move);
			}
			if (dx == 0 || dy == 0)
			{
				Globals::rookAttacks[i].push_back(move);
				Globals::queenAttacks[i].push_back(move);
			}
			if (abs(dx) <= 1 && abs(dy) <= 1) Globals::kingAttacks[i].push_back(move);

			if (dx == 0 && abs(dy) == 1) Globals::pawnAttacks[i].push_back(move);
			if (dx == 0 && abs(dy) == 2) Globals::pawnAttacks[i].push_back(move);
			if (abs(dx) == 1 && abs(dy) == 1) Globals::pawnAttacks[i].push_back(move);
		}
	}

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
		std::cout << "Select mode: gui or uci: ";
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
		SDL_CreateWindowAndRenderer("Chess engine", w, h, 0, &wnd, &rend);
		SDL_SetRenderVSync(rend, 1);

		auto state = GameState::newGame();
		//state = GameState::fromFEN("rnb1kbnr/pppp1ppp/4p3/8/3Pq3/4B1P1/PPP2P1P/RN1QKBNR b KQkq - 1 4"); //queen blundering
		//state = GameState::fromFEN("rnbqkbnr/pppp3p/4p3/5pp1/5P2/5NPB/PPPPP2P/RNBQK2R w KQkq - 0 5"); //castling test
		//state = GameState::fromFEN("4k3/2p3P1/p2pb3/5p2/1P2p3/B1N3P1/P2P4/R3K2R w KQkq - 0 27");
		//state = GameState::fromFEN("8/2p5/pk1p2R1/Q7/1P6/B1N1P1P1/R7/4K3 b KQkq - 2 35");
		//state = GameState::fromFEN("1nbk2nr/prppqppp/8/1p1N4/3PP2P/P4P2/1P1Q2P1/2KR1BNR b kq - 4 14");
		//state = GameState::fromFEN("1k4n1/6PP/4R3/1p6/1P6/8/P1K5/8 b kq - 0 56");
		//state = GameState::fromFEN("8/8/8/2K2k2/8/8/8/8 w - - 0 1"); //king vs king test
		//state = GameState::fromFEN("8/8/1k6/5N2/1r6/6B1/5K2/8 b - - 0 1"); //5-piece endgame test

		float minDim = std::min(w, h);
		BoardView boardView({ 0,0,minDim,minDim }, 8, 8);
		std::vector<PieceTexture> pieceTextures;
		{
			std::pair<PieceType, std::string> paths[] = {
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
				t.tex = SDL_CreateTextureFromSurface(rend, s1);
				t.w = s1->w;
				t.h = s1->h;
				pieceTextures.emplace_back(t);
				t.team = Team::BLACK;
				t.tex = SDL_CreateTextureFromSurface(rend, s2);
				t.w = s2->w;
				t.h = s2->h;
				pieceTextures.emplace_back(t);
			}
		}

		BoardDrawCtx boardDrawCtx;
		boardDrawCtx.gameState = &state;
		boardDrawCtx.textures = &pieceTextures;
		SearchParams params;
		params.maxDepth = 4;
		while (true)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == 1 && state.whitesTurn)
				{
					SDL_Point p;
					p.x = event.motion.x;
					p.y = event.motion.y;
					auto clickedAtSquare = boardView.squareAt(p);
					if (!boardDrawCtx.activeSquare)
					{
						boardDrawCtx.activeSquare = clickedAtSquare;
						boardDrawCtx.circlesAt.clear();
						MoveList legalMoves;
						state.getAllLegalMovesForPieceAt(*boardDrawCtx.activeSquare, legalMoves);
						for (auto& move : legalMoves)
						{
							boardDrawCtx.circlesAt.push_back({ move.to.x, move.to.y });
						}
					}
					else
					{
						Move m;
						m.type = MoveType::MOVE_TO_SQUARE;
						m.from.x = boardDrawCtx.activeSquare->x;
						m.from.y = boardDrawCtx.activeSquare->y;
						m.to.x = clickedAtSquare.x;
						m.to.y = clickedAtSquare.y;

						Piece& movingPiece = state.getPieceAt(m.from);
						Piece& targetPiece = state.getPieceAt(m.to);
						bool onLastRank = m.to.y == (state.whitesTurn ? 0 : 7);
						bool isPawn = movingPiece.type == PieceType::PAWN;
						if (isPawn && onLastRank) {
							m.type = MoveType::MOVE_AND_PROMOTE_TO_QUEEN;
						}

						using enum MoveType;
						std::pair<bool, MoveCheckStatus> application;
						if ((movingPiece.type == PieceType::KING || movingPiece.type == PieceType::ROOK) && (movingPiece.team == targetPiece.team))
						{
							Move copy = m;
							std::array<MoveType, 2> castles = { BLACK_KINGSIDE_CASTLE, BLACK_QUEENSIDE_CASTLE };
							if (movingPiece.team == Team::WHITE) castles = { WHITE_KINGSIDE_CASTLE, WHITE_QUEENSIDE_CASTLE };
							for (auto ct : castles)
							{
								m.type = ct;
								application = state.tryApplyMove(m, false);
								if (application.first) break;
							}
							m = copy;
						}

						if (!application.first) application = state.tryApplyMove(m, false);
						if (!application.first) std::cout << StringifyEnum(application.second) << "\n";
						else std::cout << state.toFEN() << "\n";

						boardDrawCtx.activeSquare = std::nullopt;
						boardDrawCtx.circlesAt.clear();
						state.updateStatus();
						if (state.gameStatus != GameStatus::ONGOING) std::cout << "GAME OVER: " << StringifyEnum(*state.gameStatus) << "\n";
					}
				}
			}

			SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
			SDL_RenderClear(rend);

			boardView.draw(rend, boardDrawCtx);
			SDL_RenderPresent(rend);

			if (!state.whitesTurn)
			{
				SearchReturn search = state.findBestMove(params);
				state.tryApplyMove(search.bestMove);
				state.updateStatus();
				std::cout << "Eval: " << search.evaluation << ", " << search.nodesSearched << " nodes searched in " << search.searchSeconds << " s (" << search.nodesPerSecond << " nps)\n";
			}
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

	tb_free();
}