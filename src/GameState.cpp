#include "GameState.h"
#include <sstream>
#include <assert.h>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <tbprobe.h>
#include "globals.h"
#include "BondcatException.h"

#pragma warning(push)
#pragma warning(disable : 6262) //"This function uses X bytes of stack, consider moving to heap"

constexpr std::array<float, 64> knightMult = {
	0.3, 0.4, 0.5, 0.6, 0.6, 0.5, 0.4, 0.3,
	0.5, 0.6, 0.6, 0.7, 0.7, 0.6, 0.5, 0.4,
	0.6, 0.7, 0.75, 0.75, 0.75, 0.7, 0.65, 0.5,
	0.6, 0.7, 0.9, 0.9, 0.9, 0.9, 0.8, 0.7,
	0.6, 0.7, 0.9, 0.9, 0.9, 0.9, 0.8, 0.7,
	0.6, 0.7, 0.75, 0.75, 0.75, 0.7, 0.65, 0.5,
	0.5, 0.6, 0.6, 0.7, 0.7, 0.6, 0.5, 0.4,
};
constexpr std::array<float, 64> bishopMult = {
	1, 1, 1, 1, 1, 1, 1, 1,
	0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95,
	0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9,
	0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85,
	0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85, 0.85,
	0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9,
	0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95,
	1, 1, 1, 1, 1, 1, 1, 1,
};
GameState::GameState()
{
	this->stateStack.push_back({});
	//this->stateStack.reserve(1024);
}
SearchReturn GameState::findBestMove(SearchParams params)
{
	assert(params.maxDepth >= 1);
	SearchReturn ret;
	uint64_t searchStartNs = SDL_GetTicksNS();
	bool shouldReturn = false;
	
	bool endgameTablebaseHit = [&]() {
		//The root node sets up the search start time and checks for tablebase hits, if any.
		//DO NOT pass st.board stuff here without change, since Fathom uses different convention!!!!
		//Ours: a8=0, h8=7, ..., a1=56,h1=63
		//Fathom: a1=0 (!), h1 = 7, a8 = 56, h8 = 63
		int pieceCount = 0;
		uint64_t whitePieces = 0, blackPieces = 0, kings = 0, queens = 0, rooks = 0, bishops = 0, knights = 0, pawns = 0;
		int i = 0;
		for (char rank = '1'; rank <= '8'; ++rank)
		{
			for (char file = 'a'; file <= 'h'; ++file, ++i)
			{
				std::string s;
				s.push_back(file);
				s.push_back(rank);
				Piece p = getPieceAt(Square(s));

				if (p.type == PieceType::EMPTY) continue;
				++pieceCount;
				bool black = (p.team == Team::BLACK);
				bool white = (p.team == Team::WHITE);

				uint64_t pieceMask = 1ull << i;
				switch (p.type)
				{
				case PieceType::KING: kings |= pieceMask; break;
				case PieceType::QUEEN: queens |= pieceMask; break;
				case PieceType::ROOK: rooks |= pieceMask; break;
				case PieceType::BISHOP: bishops |= pieceMask; break;
				case PieceType::KNIGHT: knights |= pieceMask; break;
				case PieceType::PAWN: pawns |= pieceMask; break;
				default:
					break;
				}
				if (black) blackPieces |= pieceMask;
				if (white) whitePieces |= pieceMask;
			}
			if (pieceCount > TB_LARGEST || !st.castlingRights.nonePresent()) return false;
		}

		unsigned int tb_ret;
		{
			int enpassantIndex = 0;
			if (st.enpassant) enpassantIndex = Square(_tzcnt_u64(st.enpassant)).toFathomIndex();
			std::lock_guard lck(Globals::tablebase_mutex);
			tb_ret = tb_probe_root(whitePieces, blackPieces, kings, queens, rooks, bishops, knights, pawns, st.halfmoves, false, enpassantIndex, st.whitesTurn, nullptr);
		}

		if (tb_ret == TB_RESULT_FAILED) return false;
		if (tb_ret == TB_RESULT_CHECKMATE)
		{
			ret.evaluation = st.whitesTurn ? 10000 : -10000;
			return true;
		}
		else if (tb_ret == TB_RESULT_STALEMATE)
		{
			ret.evaluation = 0;
			return true;
		}
		else ret.evaluation = TB_GET_WDL(tb_ret);
		//TB_GET_FROM, TB_GET_TO, TB_GET_PROMOTES, TB_GET_EP
		int sqFrom = TB_GET_FROM(tb_ret);
		int sqTo = TB_GET_TO(tb_ret);
		

		Square from(sqFrom % 8, 7 - sqFrom / 8);
		Square to(sqTo % 8, 7 - sqTo / 8);
		ret.bestMove.from = from;
		ret.bestMove.to = to;
		ret.bestMove.type = MoveType::MOVE_TO_SQUARE;
		if (TB_GET_PROMOTES(tb_ret) == TB_PROMOTES_QUEEN) ret.bestMove.type = MoveType::MOVE_AND_PROMOTE_TO_QUEEN;
		if (TB_GET_PROMOTES(tb_ret) == TB_PROMOTES_ROOK) ret.bestMove.type = MoveType::MOVE_AND_PROMOTE_TO_ROOK;
		if (TB_GET_PROMOTES(tb_ret) == TB_PROMOTES_BISHOP) ret.bestMove.type = MoveType::MOVE_AND_PROMOTE_TO_BISHOP;
		if (TB_GET_PROMOTES(tb_ret) == TB_PROMOTES_KNIGHT) ret.bestMove.type = MoveType::MOVE_AND_PROMOTE_TO_KNIGHT;
		return true;
		}();
	
	if (!endgameTablebaseHit)
	{
		//Yes, maxDepth shouldn't be decremented, since we didn't perform any search here yet. 
		//This function only checks endgame tables, sets up initial search and manages the feedback to UCI/GUI
		SearchReturn search = findBestMoveInner(params.maxDepth, -INFINITY, INFINITY, params);
		ret.bestMove = search.bestMove;
		ret.evaluation = search.evaluation;
		ret.nodesSearched += search.nodesSearched;
	}

	uint64_t searchEndNs = SDL_GetTicksNS();
	ret.searchSeconds = (searchEndNs - searchStartNs) / 1e9;
	ret.nodesPerSecond = ret.nodesSearched / ret.searchSeconds;
	ret.depth = params.maxDepth;
	ret.endgameTablebaseHit = endgameTablebaseHit;

	if (abs(ret.evaluation) >= 1000)
	{
		int matePlies = 10000 - abs(ret.evaluation);
		int mateMoves = (matePlies + 1) / 2;
		ret.mateIn = ret.evaluation > 0 ? mateMoves : -mateMoves;
	}

	return ret;
}

SearchReturn GameState::findBestMoveInner(int remainingDepth, float alpha, float beta, const SearchParams& params)
{
	SearchReturn ret;
	ret.nodesSearched++;

	std::optional<float> terminalNodeEvalFromWhitesPerspective; //this will be used for game-ended and search out of depth nodes
	//First, see if we're already done before any search
	if (*this->gameStatus != GameStatus::ONGOING)
	{
		//if (remainingDepth > 0) __debugbreak();
		float mateScore = 10000 - params.maxDepth + remainingDepth; //TODO: can change it to fullmove clock?
		switch (*this->gameStatus)
		{
		case GameStatus::WHITE_WON_BY_CHECKMATE:
			terminalNodeEvalFromWhitesPerspective = mateScore; break;
		case GameStatus::BLACK_WON_BY_CHECKMATE:
			terminalNodeEvalFromWhitesPerspective = -mateScore; break;
		case GameStatus::DRAW_STALEMATE:
		case GameStatus::DRAW_THREEFOLD_REPETITION:
		case GameStatus::DRAW_INSUFFICIENT_MATERIAL:
			terminalNodeEvalFromWhitesPerspective = 0.f; break;
		default: break;
		}
	}
	else if (remainingDepth > 0) //else if there's depth remaining, search recursively
	{
		DerivedData dd = deriveStateData();
		ret.evaluation = -INFINITY;
		for (auto& move : dd.legalMoveList)
		{
			//if (move.from == Square("f3") && move.to == Square("g2")) __debugbreak();
			this->makeMove(move);
			this->updateStatus();

			SearchReturn subsearch = findBestMoveInner(remainingDepth - 1, -beta, -alpha, params);
			ret.nodesSearched += subsearch.nodesSearched;
			//in main search (not game-ended), the algorithm immediately returns current player's advantage, so no need to change it
			if (-subsearch.evaluation > ret.evaluation) ret.bestMove = move;
			ret.evaluation = std::max(-subsearch.evaluation, ret.evaluation);
			alpha = std::max(alpha, ret.evaluation);

			this->rollbackMove();
			if (alpha >= beta) break;
		}
	}
	else //out of depth AND not game-ended, evaluate position
	{
		//negative for black advantage, positive for white advantage. Negamax expects it to be positive for current side advantaged, so it'll be adjusted later
		terminalNodeEvalFromWhitesPerspective = [&]() -> float {
			
			float whiteScore =
				std::popcount(st.board.white & st.board.queens) * 9 +
				std::popcount(st.board.white & st.board.rooks) * 5 +
				std::popcount(st.board.white & st.board.bishops) * 3.1 +
				std::popcount(st.board.white & st.board.knights) * 3 +
				std::popcount(st.board.white & st.board.pawns) * 1;
			float blackScore =
				std::popcount(st.board.black & st.board.queens) * 9 +
				std::popcount(st.board.black & st.board.rooks) * 5 +
				std::popcount(st.board.black & st.board.bishops) * 3.1 +
				std::popcount(st.board.black & st.board.knights) * 3 +
				std::popcount(st.board.black & st.board.pawns) * 1;

			DerivedData dd = deriveStateData();
			float mobilityBonus = dd.legalMoveList.size() * 0.02;

			if (st.whitesTurn) whiteScore += mobilityBonus;
			else blackScore += mobilityBonus;
			
			return whiteScore - blackScore;
		}();
	}

	//override whatever lies in ret.evaluation if we have override for that (terminal nodes)
	if (terminalNodeEvalFromWhitesPerspective) ret.evaluation = st.whitesTurn ? *terminalNodeEvalFromWhitesPerspective : -*terminalNodeEvalFromWhitesPerspective;
	return ret;
}

uint64_t GameState::getSquaresAttackedByRookAt(uint64_t ind, uint64_t rookAllies)
{
	using enum Direction;
	const auto& rays = Globals::directionRaysByOrigin;
	uint64_t paths = 0, ray, occludersMask, firstOccluderInd, notEmpty = st.board.white | st.board.black;

	//TODO: lzcnt/tzcnt return 64 for all zeroes, and shifting by that is UB (in fact, it breaks on my system by rejecting paths that are correct). 
	//Thus, branches for now. Think about ways of removing branches
	ray = rays[ind][int(LEFT)];
	occludersMask = ray & notEmpty;
	if (occludersMask)
	{
		firstOccluderInd = 63 - _lzcnt_u64(ray & notEmpty); //left path: find first occluder to the left, starting from bottom-right to left to top
		paths |= ray & ~((1ull << firstOccluderInd) - 1); //this bitwise magic does everything to include first occluder into pathable squares. Allies will be filtered later
	}
	else paths |= ray;

	ray = rays[ind][int(RIGHT)];
	occludersMask = ray & notEmpty;
	if (occludersMask)
	{
		firstOccluderInd = _tzcnt_u64(occludersMask); //right path: scan from top-left to right to bottom
		uint64_t shiftMsk = 1ull << firstOccluderInd; //shift by variable 64 leaves input untouched!
		paths |= ray & ((shiftMsk << 1) - 1);
	}
	else paths |= ray;

	ray = rays[ind][int(UP)];
	occludersMask = ray & notEmpty;
	if (occludersMask)
	{
		firstOccluderInd = 63 - _lzcnt_u64(occludersMask);
		paths |= ray & ~((1ull << firstOccluderInd) - 1);
	}
	else paths |= ray;

	ray = rays[ind][int(DOWN)];
	occludersMask = ray & notEmpty;
	if (occludersMask)
	{
		firstOccluderInd = _tzcnt_u64(occludersMask);
		uint64_t shiftMsk = 1ull << firstOccluderInd; //shift by variable 64 leaves input untouched!
		paths |= ray & ((shiftMsk << 1) - 1);
	}
	else paths |= ray;

	return paths & ~rookAllies;
}

uint64_t GameState::getSquaresAttackedByBishopAt(uint64_t ind, uint64_t bishopAllies)
{
	using enum Direction;
	const auto& rays = Globals::directionRaysByOrigin;
	uint64_t paths = 0, ray, occludersMask, firstOccluderInd, notEmpty = st.board.white | st.board.black;

	ray = rays[ind][int(LEFT_UP)];
	occludersMask = ray & notEmpty;
	if (occludersMask)
	{
		firstOccluderInd = 63-_lzcnt_u64(occludersMask);
		paths |= ray & ~((1ull << firstOccluderInd) - 1);
	}
	else paths |= ray;

	ray = rays[ind][int(RIGHT_UP)];
	occludersMask = ray & notEmpty;
	if (occludersMask)
	{
		firstOccluderInd = 63 - _lzcnt_u64(occludersMask);
		paths |= ray & ~((1ull << firstOccluderInd) - 1);
	}
	else paths |= ray;

	ray = rays[ind][int(LEFT_DOWN)];
	occludersMask = ray & notEmpty;
	if (occludersMask)
	{
		firstOccluderInd = _tzcnt_u64(occludersMask);
		uint64_t shiftMsk = 1ull << firstOccluderInd;
		paths |= ray & ((shiftMsk << 1) - 1);
	}
	else paths |= ray;

	ray = rays[ind][int(RIGHT_DOWN)];
	occludersMask = ray & notEmpty;
	if (occludersMask)
	{
		firstOccluderInd = _tzcnt_u64(occludersMask);
		uint64_t shiftMsk = 1ull << firstOccluderInd;
		paths |= ray & ((shiftMsk << 1) - 1);
	}
	else paths |= ray;

	return paths & ~bishopAllies;
}

bool GameState::isSquareAttacked(uint64_t sqInd, bool forWhite)
{
	//abusing the symmetry of attacks:
	//If a square is attacked by knight/king, then knight/king at this square can also attack the attacker
	uint64_t enemies = forWhite ? st.board.black : st.board.white;
	uint64_t allies = forWhite ? st.board.white: st.board.black;
	if (st.board.knights & enemies & Globals::knightMoves[sqInd]) return true;
	if (st.board.kings & enemies & Globals::kingMoves[sqInd]) return true;

	//Same for sliders. If enemy slider is attacked by us, then we are also attacked
	uint64_t sqBit = 1ull << sqInd;
	if (getSquaresAttackedByBishopAt(sqInd, allies) & enemies & (st.board.bishops | st.board.queens)) return true;
	if (getSquaresAttackedByRookAt(sqInd, allies) & enemies & (st.board.rooks | st.board.queens)) return true;

	if (forWhite) return Globals::whitePawnCaptures[sqInd] & enemies & st.board.pawns;
	else return Globals::blackPawnCaptures[sqInd] & enemies & st.board.pawns;
	return false;
}

GameState GameState::newGame()
{
	return fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

StateStackEntry& GameState::getTopStateStackEntry()
{
	return stateStack.back();
}

Piece GameState::getPieceAt(Square s)
{
	using enum TeamedPieceType;
	assert(s.inBounds());
	uint64_t bit = 1ull << s.toIndex();
	if (bit & st.board.white & st.board.kings) return WHITE_KING;
	if (bit & st.board.white & st.board.queens) return WHITE_QUEEN;
	if (bit & st.board.white & st.board.rooks) return WHITE_ROOK;
	if (bit & st.board.white & st.board.bishops) return WHITE_BISHOP;
	if (bit & st.board.white & st.board.knights) return WHITE_KNIGHT;
	if (bit & st.board.white & st.board.pawns) return WHITE_PAWN;
	if (bit & st.board.black & st.board.kings) return BLACK_KING;
	if (bit & st.board.black & st.board.queens) return BLACK_QUEEN;
	if (bit & st.board.black & st.board.rooks) return BLACK_ROOK;
	if (bit & st.board.black & st.board.bishops) return BLACK_BISHOP;
	if (bit & st.board.black & st.board.knights) return BLACK_KNIGHT;
	if (bit & st.board.black & st.board.pawns) return BLACK_PAWN;
	return EMPTY;
}

void GameState::updateStatus()
{
	//Team nextTeamToMove = st.whitesTurn ? Team::WHITE : Team::BLACK;
	DerivedData dd = deriveStateData();
	bool kingChecked = st.whitesTurn ? dd.whiteChecked : dd.blackChecked;
	
	if (kingChecked)
	{
		bool checkmate = true;
		bool rollback = false;
		bool white = st.whitesTurn;
		for (auto& move : dd.legalMoveList)
		{
			this->makeMove(move, false);
			rollback = true;
			
			uint64_t kingIndex = _tzcnt_u64(st.board.kings & (white ? st.board.white : st.board.black));
			bool kingChecked2 = isSquareAttacked(kingIndex, white);
			if (!kingChecked2)
			{
				checkmate = false;
				break;
			}
			this->rollbackMove(false);
			rollback = false;
		}

		if (rollback) this->rollbackMove(false);
		if (checkmate)
		{
			this->gameStatus = st.whitesTurn ? GameStatus::BLACK_WON_BY_CHECKMATE : GameStatus::WHITE_WON_BY_CHECKMATE;
			return;
		}
	}
	else if (dd.legalMoveList.size() == 0) { this->gameStatus = GameStatus::DRAW_STALEMATE; return; };
	this->gameStatus = GameStatus::ONGOING;

	if (st.halfmoves >= 100) { this->gameStatus = GameStatus::DRAW_FIFTY_MOVE_RULE; return; };
	//TODO: insufficient material

	//if (insufficiet) this->status = GameStatus::DRAW_INSUFFICIENT_MATERIAL;
	int repetitions = 0;
	if (st.halfmoves > 0)
	{
		for (int i = this->stateStack.size() - 1; i >= 0; i -= 2)
		{
			//if (this->stateStack[i].halfmoves == 0) break;
			if (this->stateStack[i] == st) ++repetitions;
			if (repetitions >= 3)
			{
				this->gameStatus = GameStatus::DRAW_THREEFOLD_REPETITION;
				break;
			}
		}
	}
}

void GameState::reset()
{
	*this = newGame();
}

MoveResult GameState::tryMakeMove(const Move& move)
{
	DerivedData dd = deriveStateData();
	MoveResult ret;
	ret.applied = false;
	ret.checkStatus = MoveCheckStatus::ILLEGAL_REASON_NOT_SPECIFIED;
	if (std::find(dd.legalMoveList.begin(), dd.legalMoveList.end(), move) != dd.legalMoveList.end())
	{
		ret.applied = true;
		this->makeMove(move);
		ret.checkStatus = MoveCheckStatus::LEGAL;
	}
	return ret;
}

void GameState::makeMove(const Move& move, bool rememberPosition)
{
	assert(std::popcount(st.board.kings) == 2);
	StateStackEntry cp = st;
	this->stateStack.push_back(cp);
	this->st.halfmoves++;
	this->st.whitesTurn ^= 1;
	if (this->st.whitesTurn) this->st.fullmoves++;

	using enum MoveType;
	bool eraseEnpassant = true;
	if (move.isCastlingMove())
	{
		CastlingSquares cs(move.type);
		st.board.movePiece(cs.kingStart.toIndex(), cs.kingEnd.toIndex());
		st.board.movePiece(cs.rookStart.toIndex(), cs.rookEnd.toIndex());
		if (move.type == MoveType::WHITE_KINGSIDE_CASTLE) {st.castlingRights.whiteKingside = false; st.castlingRights.whiteQueenside = false; }
		else if (move.type == MoveType::BLACK_KINGSIDE_CASTLE) { st.castlingRights.blackKingside = false; st.castlingRights.blackQueenside = false; }
		else if (move.type == MoveType::WHITE_QUEENSIDE_CASTLE) { st.castlingRights.whiteKingside = false; st.castlingRights.whiteQueenside = false; }
		else if (move.type == MoveType::BLACK_QUEENSIDE_CASTLE) { st.castlingRights.blackKingside = false; st.castlingRights.blackQueenside = false; }
	}
	else
	{
		uint64_t srcBit = 1ull << move.from.toIndex();
		uint64_t dstBit = 1ull << move.to.toIndex();
		//a piece was captured or pawn was moved, reset halfmove clock
		if ((dstBit & (st.board.black | st.board.white)) || (srcBit & st.board.pawns)) st.halfmoves = 0;

		if (st.board.rooks & (srcBit | dstBit)) //if rook moves or captured, remove corresponding castling rights 
		{
			if (move.from == Square(0, 0) || move.to == Square(0, 0)) st.castlingRights.blackQueenside = false;
			if (move.from == Square(7, 0) || move.to == Square(7, 0)) st.castlingRights.blackKingside = false;
			if (move.from == Square(0, 7) || move.to == Square(0, 7)) st.castlingRights.whiteQueenside = false;
			if (move.from == Square(7, 7) || move.to == Square(7, 7)) st.castlingRights.whiteKingside = false;
		}
		if (srcBit & st.board.kings & st.board.white) st.castlingRights.whiteKingside = st.castlingRights.whiteQueenside = false;
		if (srcBit & st.board.kings & st.board.black) st.castlingRights.blackKingside = st.castlingRights.blackQueenside = false;

		auto promotionTarget = move.getPromotionTarget();
		if (!promotionTarget)
		{
			if (st.board.pawns & srcBit)
			{
				int forwardForMovingPawn = move.to.y > move.from.y ? 1 : -1;
				if (abs(move.to.y - move.from.y) == 2)
				{
					st.enpassant = 1ull << Square(move.from.x, move.from.y + forwardForMovingPawn).toIndex();
					eraseEnpassant = false;
				}
				else if (st.enpassant && dstBit == st.enpassant)
				{
					Square enpassantCapturedSq = Square(move.to.x, move.to.y - forwardForMovingPawn);
					st.board.removePiece(enpassantCapturedSq.toIndex());
				}
			}
			st.board.movePiece(move.from.toIndex(), move.to.toIndex());	
		}
		else
		{
			st.board.movePiece(move.from.toIndex(), move.to.toIndex());
			st.board.pawns &= ~dstBit;

			if (*promotionTarget == PieceType::QUEEN) st.board.queens |= dstBit;
			else if (*promotionTarget == PieceType::ROOK) st.board.rooks |= dstBit;
			else if (*promotionTarget == PieceType::BISHOP) st.board.bishops |= dstBit;
			else if (*promotionTarget == PieceType::KNIGHT) st.board.knights |= dstBit;
		}
	}

	if (eraseEnpassant) st.enpassant = 0;
	//sadly, we can't put the check of game status here, since it creates infinite recursion of getting legal moves. 
	//Thus, gameStatus is made optional that will explode on unwrapping, forcing callers to update game status
	this->gameStatus = std::nullopt;

	//if (rememberPosition) this->seenPositions[st.board]++;
}

void GameState::rollbackMove(bool forgetPosition)
{
	if (forgetPosition)
	{
		assert(seenPositions[st.board] >= 1);
		//if (--seenPositions[st.board] == 0) seenPositions.erase(st.board);
	}
	assert(this->stateStack.size() > 0);
	this->stateStack.pop_back();
}

DerivedData GameState::deriveStateData(DeriveOptions opts)
{
	assert(std::popcount(st.board.kings) == 2);
	DerivedData ret;
	//ret.motionMatrix = { 0 };
	uint64_t allies = st.whitesTurn ? st.board.white : st.board.black;
	uint64_t enemies = st.whitesTurn ? st.board.black : st.board.white;
	uint64_t notEmpty = st.board.black | st.board.white;
	uint64_t empty = ~notEmpty;

	uint64_t squaresAttackedByWhite = 0, squaresAttackedByBlack = 0;
	
	const auto& rays = Globals::directionRaysByOrigin;
	uint64_t asdd = st.board.white & st.board.rooks;
	using enum Direction;
	using enum MoveType;

	//unpacks the moves bitmask and pushes it's moves into the move list. 
	//if promotions == false (default), only pushes movements to destination squares
	//otherwise, pushes only movements to destination squares WITH promotion (all 4 possible promotions pushed)
	auto commit_moves = [&](uint64_t moves, uint64_t pieceInd, bool promotions = false) {
		while (moves)
		{
			uint64_t pathDstInd = _tzcnt_u64(moves);
			if (!promotions) ret.legalMoveList.push_back({ Square(pieceInd),Square(pathDstInd), MoveType::MOVE_TO_SQUARE });
			else
			{
				for (auto it : { MOVE_AND_PROMOTE_TO_QUEEN, MOVE_AND_PROMOTE_TO_ROOK, MOVE_AND_PROMOTE_TO_BISHOP, MOVE_AND_PROMOTE_TO_KNIGHT })
					ret.legalMoveList.push_back({ Square(pieceInd),Square(pathDstInd), it });
			}
			moves &= ~(1ull << pathDstInd);
		}
		};
	//on our board, tzcnt basically scans from from top-left to bottom right, lzcnt - from bottom-right to top-left (inverse of lzcnt)
	//Thus, one such instruction can do entire loop's worth of scanning

	uint64_t whiteKnights = st.board.white & st.board.knights;
	uint64_t blackKnights = st.board.black & st.board.knights;
	while (whiteKnights)
	{
		uint64_t ind = _tzcnt_u64(whiteKnights);
		uint64_t moves = Globals::knightMoves[ind];
		whiteKnights &= ~(1ull << ind);
		squaresAttackedByWhite |= moves;
		if (opts.makeLegalMoveList && st.whitesTurn) commit_moves(moves & ~st.board.white & ~st.board.kings, ind);
	}
	while (blackKnights)
	{
		uint64_t ind = _tzcnt_u64(blackKnights);
		uint64_t moves = Globals::knightMoves[ind];
		blackKnights &= ~(1ull << ind);
		squaresAttackedByBlack |= moves;
		if (opts.makeLegalMoveList && !st.whitesTurn) commit_moves(moves & ~st.board.black & ~st.board.kings, ind);
	}

	uint64_t whiteRookLike = st.board.white & (st.board.rooks | st.board.queens);
	uint64_t blackRookLike = st.board.black & (st.board.rooks | st.board.queens);
	while (whiteRookLike)
	{
		uint64_t ind = _tzcnt_u64(whiteRookLike);
		uint64_t paths = this->getSquaresAttackedByRookAt(ind, st.board.white);
		squaresAttackedByWhite |= paths;
		whiteRookLike &= ~(1ull << ind);
		if (opts.makeLegalMoveList && st.whitesTurn) commit_moves(paths & ~st.board.kings, ind);
	}
	while (blackRookLike)
	{
		uint64_t ind = _tzcnt_u64(blackRookLike);
		uint64_t paths = this->getSquaresAttackedByRookAt(ind, st.board.black);
		squaresAttackedByBlack |= paths;
		blackRookLike &= ~(1ull << ind);
		if (opts.makeLegalMoveList && !st.whitesTurn) commit_moves(paths & ~st.board.kings, ind);
	}

	uint64_t whiteBishopLike = st.board.white & (st.board.bishops | st.board.queens);
	uint64_t blackBishopLike = st.board.black & (st.board.bishops | st.board.queens);
	while (whiteBishopLike)
	{
		uint64_t ind = _tzcnt_u64(whiteBishopLike);
		uint64_t paths = this->getSquaresAttackedByBishopAt(ind, st.board.white);
		squaresAttackedByWhite |= paths;
		whiteBishopLike &= ~(1ull << ind);
		if (opts.makeLegalMoveList && st.whitesTurn) commit_moves(paths & ~st.board.kings, ind);
	}
	while (blackBishopLike)
	{
		uint64_t ind = _tzcnt_u64(blackBishopLike);
		uint64_t paths = this->getSquaresAttackedByBishopAt(ind, st.board.black);
		squaresAttackedByBlack |= paths;
		blackBishopLike &= ~(1ull << ind);
		if (opts.makeLegalMoveList && !st.whitesTurn) commit_moves(paths & ~st.board.kings, ind);
	}

	uint64_t whitePawns = st.board.pawns & st.board.white, blackPawns = st.board.pawns & st.board.black;
	while (whitePawns)
	{
		uint64_t ind = _tzcnt_u64(whitePawns);
		uint64_t paths = this->getSquaresAttackedByRookAt(ind, st.board.white) & Globals::whitePawnMoves[ind] & empty;
		whitePawns &= ~(1ull << ind);
		squaresAttackedByWhite |= Globals::whitePawnCaptures[ind];
		if (opts.makeLegalMoveList && st.whitesTurn)
		{
			commit_moves(paths & ~st.board.kings, ind, ind >= 8 && ind < 16); //moves+promotions
			commit_moves(Globals::whitePawnCaptures[ind] & ~st.board.kings & st.board.black, ind, ind >= 8 && ind < 16);
			commit_moves(Globals::whitePawnCaptures[ind] & st.enpassant, ind);
		}
	}
	while (blackPawns)
	{
		uint64_t ind = _tzcnt_u64(blackPawns);
		uint64_t paths = this->getSquaresAttackedByRookAt(ind, st.board.black) & Globals::blackPawnMoves[ind] & empty;
		blackPawns &= ~(1ull << ind);
		squaresAttackedByBlack |= Globals::blackPawnCaptures[ind];
		if (opts.makeLegalMoveList && !st.whitesTurn)
		{
			commit_moves(paths & ~st.board.kings, ind, ind >= 48 && ind < 56); //moves+promotions
			commit_moves(Globals::blackPawnCaptures[ind] & ~st.board.kings & st.board.white, ind, ind >= 48 && ind < 56);
			commit_moves(Globals::blackPawnCaptures[ind] & st.enpassant, ind);
		}
	}
	
	int kingIndex = _tzcnt_u64(allies & st.board.kings);
	ret.whiteKingIndex = _tzcnt_u64(st.board.kings & st.board.white);
	ret.blackKingIndex = _tzcnt_u64(st.board.kings & st.board.black);
	{
		uint64_t wkingMoves = Globals::kingMoves[ret.whiteKingIndex] & ~st.board.white;
		uint64_t bkingMoves = Globals::kingMoves[ret.blackKingIndex] & ~st.board.black;
		squaresAttackedByWhite |= wkingMoves;
		squaresAttackedByBlack |= bkingMoves;
		if (opts.makeLegalMoveList) commit_moves((st.whitesTurn ? wkingMoves : bkingMoves) & ~st.board.kings, st.whitesTurn ? ret.whiteKingIndex : ret.blackKingIndex);
	}
	
	uint64_t squaresAttackedByEnemy = st.whitesTurn ? squaresAttackedByBlack : squaresAttackedByWhite;
	
	//ret.motionMatrix[kingIndex] &= ~squaresAttackedByEnemy;
	ret.squaresAttackedByBlack = squaresAttackedByBlack;
	ret.squaresAttackedByWhite = squaresAttackedByWhite;

	ret.whiteChecked = st.board.kings & st.board.white & squaresAttackedByBlack;
	ret.blackChecked = st.board.kings & st.board.black & squaresAttackedByWhite;

	if (opts.makeLegalMoveList)
	{
		if (st.whitesTurn)
		{
			if (st.castlingRights.whiteKingside)
			{
				uint64_t emptyCheck = (1ull << 61) | (1ull << 62);
				uint64_t checkCheck = (1ull << 60) | (1ull << 61) | (1ull << 62);
				if (!(notEmpty & emptyCheck) && !(squaresAttackedByEnemy & checkCheck)) ret.legalMoveList.push_back(Move({}, {}, MoveType::WHITE_KINGSIDE_CASTLE));
			}
			if (st.castlingRights.whiteQueenside)
			{
				uint64_t emptyCheck = (1ull << 57) | (1ull << 58) | (1ull << 59);
				uint64_t checkCheck = (1ull << 58) | (1ull << 59) | (1ull << 60);
				if (!(notEmpty & emptyCheck) && !(squaresAttackedByEnemy & checkCheck)) ret.legalMoveList.push_back(Move({}, {}, MoveType::WHITE_QUEENSIDE_CASTLE));
			}
		}
		else
		{
			if (st.castlingRights.blackKingside)
			{
				uint64_t emptyCheck = (1ull << 5) | (1ull << 6);
				uint64_t checkCheck = (1ull << 4) | (1ull << 5) | (1ull << 6);
				if (!(notEmpty & emptyCheck) && !(squaresAttackedByEnemy & checkCheck)) ret.legalMoveList.push_back(Move({}, {}, MoveType::BLACK_KINGSIDE_CASTLE));
			}
			if (st.castlingRights.blackQueenside)
			{
				uint64_t emptyCheck = (1ull << 1) | (1ull << 2) | (1ull << 3);
				uint64_t checkCheck = (1ull << 2) | (1ull << 3) | (1ull << 4);
				if (!(notEmpty & emptyCheck) && !(squaresAttackedByEnemy & checkCheck)) ret.legalMoveList.push_back(Move({}, {}, MoveType::BLACK_QUEENSIDE_CASTLE));
			}
		}

		//now all pseudo legal moves are in the list, the list needs to be filtered to remove moves that put/leave the moving team's king in check
		//final filter: disallow moves that put or leave our king in check
		DeriveOptions opts2;
		opts2.makeLegalMoveList = false;
		opts2.skipPinChecks = true;
		bool savedWhitesTurn = st.whitesTurn;
		for (int i = 0; i < ret.legalMoveList.size(); ++i)
		{
			this->makeMove(ret.legalMoveList[i], false);
			uint64_t kingIndex = _tzcnt_u64(st.board.kings & (savedWhitesTurn ? st.board.white : st.board.black));
			if (this->isSquareAttacked(kingIndex, savedWhitesTurn))
			{
				ret.legalMoveList[i--] = ret.legalMoveList.back();
				ret.legalMoveList.pop_back();
			}
			this->rollbackMove(false);
		}
	}

	//TODO: maybe can put game status update here too?
	//TODO: add attackers and defender matrices and check king check avoidance with them instead of emulating move?
	return ret;
}

GameState GameState::fromFEN(const std::string& s)
{
	GameState ret;
	FEN_Codec codec(s);
	ret.st.whitesTurn = codec.whitesTurn;
	ret.st.castlingRights = codec.castlingRights;
	ret.st.halfmoves = codec.halfmoves;
	ret.st.fullmoves = codec.fullmoves;
	ret.st.enpassant = codec.enpassant ? (1ull << codec.enpassant->toIndex()) : 0;

	for (int i = 0; i < 64; ++i)
	{
		uint64_t bit = 1ull << i;
		switch (codec.board[i].teamedType)
		{
		case TeamedPieceType::WHITE_KING: ret.st.board.white |= bit; ret.st.board.kings |= bit; break;
		case TeamedPieceType::WHITE_QUEEN: ret.st.board.white |= bit; ret.st.board.queens |= bit; break;
		case TeamedPieceType::WHITE_ROOK: ret.st.board.white |= bit; ret.st.board.rooks |= bit; break;
		case TeamedPieceType::WHITE_BISHOP: ret.st.board.white |= bit; ret.st.board.bishops |= bit; break;
		case TeamedPieceType::WHITE_KNIGHT: ret.st.board.white |= bit; ret.st.board.knights |= bit; break;
		case TeamedPieceType::WHITE_PAWN: ret.st.board.white |= bit; ret.st.board.pawns |= bit; break;
		case TeamedPieceType::BLACK_KING: ret.st.board.black |= bit; ret.st.board.kings |= bit; break;
		case TeamedPieceType::BLACK_QUEEN: ret.st.board.black |= bit; ret.st.board.queens |= bit; break;
		case TeamedPieceType::BLACK_ROOK: ret.st.board.black |= bit; ret.st.board.rooks |= bit; break;
		case TeamedPieceType::BLACK_BISHOP: ret.st.board.black |= bit; ret.st.board.bishops |= bit; break;
		case TeamedPieceType::BLACK_KNIGHT: ret.st.board.black |= bit; ret.st.board.knights |= bit; break;
		case TeamedPieceType::BLACK_PAWN: ret.st.board.black |= bit; ret.st.board.pawns |= bit; break;
		case TeamedPieceType::EMPTY: break;
		default: throw BondcatException((std::stringstream() << "Unknown teamed piece type: " + int(codec.board[i].teamedType) << " at index " << i).str());
		}
	}
	ret.updateStatus();
	return ret;
}

std::string GameState::toFEN()
{
	FEN_Codec codec;
	codec.whitesTurn = st.whitesTurn;
	codec.castlingRights = st.castlingRights;
	codec.halfmoves = st.halfmoves;
	codec.fullmoves = st.fullmoves;
	if (st.enpassant) codec.enpassant = Square(_tzcnt_u64(st.enpassant));
	
	using enum TeamedPieceType;
	for (int i = 0; i < 64; ++i)
	{
		uint64_t bit = 1ull << i;
		Piece& dst = codec.board[i];
		if (bit & st.board.white)
		{
			if (bit & st.board.kings) dst = WHITE_KING;
			if (bit & st.board.queens) dst = WHITE_QUEEN;
			if (bit & st.board.rooks) dst = WHITE_ROOK;
			if (bit & st.board.bishops) dst = WHITE_BISHOP;
			if (bit & st.board.knights) dst = WHITE_KNIGHT;
			if (bit & st.board.pawns) dst = WHITE_PAWN;
		}
		if (bit & st.board.black)
		{
			if (bit & st.board.kings) dst = BLACK_KING;
			if (bit & st.board.queens) dst = BLACK_QUEEN;
			if (bit & st.board.rooks) dst = BLACK_ROOK;
			if (bit & st.board.bishops) dst = BLACK_BISHOP;
			if (bit & st.board.knights) dst = BLACK_KNIGHT;
			if (bit & st.board.pawns) dst = BLACK_PAWN;
		}
	}
	return codec.toFEN();
}

std::string SearchReturn::toUCI() const
{
	std::stringstream ss;
	int evalScore = evaluation * 100;
	int reportedScore = evalScore;
	ss << "info depth " << depth << " nodes " << nodesSearched << " nps " << uint64_t(nodesPerSecond) << " score ";
	if (mateIn) ss << "mate " << *mateIn;
	else ss << "cp " << reportedScore;

	ss << "\nbestmove " << bestMove.toUCI();
	return ss.str();
}

CastlingSquares::CastlingSquares(MoveType type)
{
	assert(Move({}, {}, type).isCastlingMove());
	switch (type)
	{
	case MoveType::WHITE_KINGSIDE_CASTLE: 
		kingStart = { 4, 7 }; kingEnd = { 6,7 }; rookStart = { 7,7 }; rookEnd = { 5,7 }; break;
	case MoveType::BLACK_KINGSIDE_CASTLE:
		kingStart = { 4, 0 }; kingEnd = { 6,0 }; rookStart = { 7,0 }; rookEnd = { 5,0 }; break;
	case MoveType::WHITE_QUEENSIDE_CASTLE:
		kingStart = { 4,7 }; kingEnd = { 2, 7 }; rookStart = { 0, 7 }; rookEnd = { 3,7 }; break;
	case MoveType::BLACK_QUEENSIDE_CASTLE:
		kingStart = { 4,0 }; kingEnd = { 2, 0 }; rookStart = { 0, 0 }; rookEnd = { 3,0 }; break;
	default:
		break;
	}
}

#pragma warning(pop) //restore too much stack usage warn