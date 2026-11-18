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
			auto rollback = this->makeMove(move);
			this->updateStatus();

			SearchReturn subsearch = findBestMoveInner(remainingDepth - 1, -beta, -alpha, params);
			ret.nodesSearched += subsearch.nodesSearched;
			//in main search (not game-ended), the algorithm immediately returns current player's advantage, so no need to change it
			if (-subsearch.evaluation > ret.evaluation) ret.bestMove = move;
			ret.evaluation = std::max(-subsearch.evaluation, ret.evaluation);
			alpha = std::max(alpha, ret.evaluation);

			this->rollbackMove(rollback);
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
			/*
			if (copy.whiteCanKingsideCastle) whiteScore += 0.5;
			else if (copy.whiteCanQueensideCastle) whiteScore += 0.33;
			if (copy.blackCanKingsideCastle) blackScore += 0.5;
			else if (copy.blackCanQueensideCastle) blackScore += 0.33;*/
			/*
			for (int i = 0; i < 64; ++i)
			{
				if (board[i].type == PieceType::EMPTY) continue;
				Team pieceTeam = board[i].team;
				float& score = pieceTeam == Team::WHITE ? whiteScore : blackScore;
				switch (board[i].type)
				{
				case PieceType::PAWN: score += 1; break;
				case PieceType::KNIGHT: score += 3; break;
				case PieceType::BISHOP: score += 3.1; break;
				case PieceType::ROOK: score += 5; break;
				case PieceType::QUEEN: score += 9; break;
				default: break;
				}
			}*/
			return whiteScore - blackScore;
		}();
	}

	//override whatever lies in ret.evaluation if we have override for that (terminal nodes)
	if (terminalNodeEvalFromWhitesPerspective) ret.evaluation = st.whitesTurn ? *terminalNodeEvalFromWhitesPerspective : -*terminalNodeEvalFromWhitesPerspective;
	return ret;
}

GameState GameState::newGame()
{
	return fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

Piece GameState::getPieceAt(Square s) const
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
		std::optional<MoveRollbackData> rollback;
		for (auto& move : dd.legalMoveList)
		{
			rollback = this->makeMove(move);
			DerivedData dd2 = deriveStateData();
			if (!(st.whitesTurn ? dd2.whiteChecked : dd2.blackChecked))
			{
				checkmate = false;
				break;
			}
			this->rollbackMove(*rollback);
		}

		if (rollback) this->rollbackMove(*rollback);
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

	/* //unsure about this
	bool insufficiet = [&]() {
		bool wBishopLightSq = false, wBishopDarkSq = false, bBishopLightSq = false, bBishopDarkSq = false;
		int wKnightCount = 0, bKnightCount = 0;
		bool wLoneKing = true, bLoneKing = true;
		for (int y = 0; y < 8; ++y)
		{
			for (int x = 0; x < 8; ++x)
			{
				Piece& p = getPieceAt(x, y);
				if (p.type != PieceType::KING && p.type != PieceType::EMPTY) p.team == WHITE ? (wLoneKing = false) : (bLoneKing = false);
				if (p.type == PieceType::QUEEN || p.type == PieceType::ROOK || p.type == PieceType::PAWN) return false;
				if (p.type == PieceType::KNIGHT) p.team == WHITE ? wKnightCount++ : bKnightCount++;
				if (p.type == PieceType::BISHOP)
				{
					bool darkSq = ((x ^ y) % 2);
					if (p.team == WHITE)
					{
						if (darkSq) wBishopLightSq = true;
						else wBishopDarkSq = true;
					}
					if (p.team == BLACK)
					{
						if (darkSq) bBishopLightSq = true;
						else bBishopDarkSq = true;
					}
				}
			}
		}
		if ((wBishopDarkSq && bBishopLightSq) || (wBishopLightSq && bBishopDarkSq)) return false;
		if (wLoneKing )
		return true;
		}();
	if (insufficiet) this->status = GameStatus::DRAW_INSUFFICIENT_MATERIAL;
	*/
	if (this->seenPositions[st.board] >= 3) { this->gameStatus = GameStatus::DRAW_THREEFOLD_REPETITION; return; }

	//if (!this->gameStatus) __debugbreak()
}

void GameState::reset()
{
	*this = newGame();
}

/*
void GameState::getAllLegalMoves(MoveList& ret)
{
	for (Square sq : Square::all_inBounds())
	{
		getAllLegalMovesForPieceAt(sq, ret);
	}

	for (auto ct : { MoveType::WHITE_KINGSIDE_CASTLE, MoveType::WHITE_QUEENSIDE_CASTLE, MoveType::BLACK_KINGSIDE_CASTLE, MoveType::BLACK_QUEENSIDE_CASTLE })
	{
		Move m;
		m.type = ct;
		if (isLegalMove(m) == MoveCheckStatus::LEGAL) ret.push_back(m);
	}
}
void GameState::getAllLegalMovesForPieceAt(Square sq, MoveList& ret)
{
	if (!sq.inBounds()) return;

	Piece& movingPiece = getPieceAt(sq);
	for (auto move : Globals::getPieceMoveCandidates(sq, movingPiece.type))
	{
		if (isLegalMove(move) == MoveCheckStatus::LEGAL) ret.push_back(move);
		if (movingPiece.type == PieceType::PAWN)
		{
			for (auto t : { MoveType::MOVE_AND_PROMOTE_TO_QUEEN, MoveType::MOVE_AND_PROMOTE_TO_ROOK, MoveType::MOVE_AND_PROMOTE_TO_BISHOP, MoveType::MOVE_AND_PROMOTE_TO_KNIGHT })
			{
				move.type = t;
				if (isLegalMove(move) == MoveCheckStatus::LEGAL) ret.push_back(move);
			}
		}
	}
}

Piece& GameState::getPieceAt(Square s)
{
	return board[s.y * 8 + s.x];
}

MoveCheckStatus GameState::isLegalCastlingMove(Move move)
{
	using enum MoveCheckStatus;
	if (!move.isCastlingMove()) return CASTLING_CHECK_ON_NONCASTLING_MOVE;
	if (!castlingRights.corresponding(move.type)) return CASTLING_RIGHTS_ABSENT;

	//castling locations are hardcoded
	auto [king, rook] = getPiecesAtCaslingSquares(move);
	if (king.type != PieceType::KING) return CASTLING_NOT_KING;
	if (rook.type != PieceType::ROOK) return CASTLING_NOT_ROOK;
	//if (king.hasMoved) return CASTLING_KING_MOVED;
	//if (rook.hasMoved) return CASTLING_ROOK_MOVED;
	if (king.team != rook.team) return CASTLING_KING_ROOK_TEAM_MISMATCH;

	Team castleTeam = (move.type == MoveType::WHITE_KINGSIDE_CASTLE || move.type == MoveType::WHITE_QUEENSIDE_CASTLE) ? Team::WHITE : Team::BLACK;
	bool kingsideCastle = (move.type == MoveType::WHITE_KINGSIDE_CASTLE || move.type == MoveType::BLACK_KINGSIDE_CASTLE);
	if (castleTeam != king.team) return CASTLING_TEAM_MISMATCHED_KING_TEAM;
	if ((castleTeam == Team::WHITE) != whitesTurn) return CASTLING_ATTEMPTED_ON_OPPONENTS_TURN; //can't castle on enemies turn!

	std::array<int, 3> attackedCheckX, obstructionCheckX;
	int kingY = castleTeam == Team::BLACK ? 0 : 7;
	if (kingsideCastle)
	{
		attackedCheckX = { 4,5,6 };
		obstructionCheckX = { 5,6,6 }; //duplicate last one as a hacky way to simplify next code (so length always stays at 3)
	}
	else
	{
		attackedCheckX = { 4,3,2 };
		obstructionCheckX = { 3,2,1 };
	}

	for (int i = 0; i < 3; ++i)
	{
		if (getPieceAt(Square(obstructionCheckX[i], kingY)).type != PieceType::EMPTY) return CASTLING_PATH_OBSTRUCTED;
		if (squareIsAttackedFor(attackedCheckX[i], kingY, castleTeam)) return CASTLING_PATH_PASSES_THROUGH_CHECK;
	}
	return LEGAL;
}

MoveCheckStatus GameState::isLegalMove(Move move)
{
	using enum MoveCheckStatus;
	bool isCastling = move.isCastlingMove();
	if (isCastling)
	{
		return isLegalCastlingMove(move);
	}

	//Non-castling, i.e. move or move+promote
	if (move.from.outOfBounds()) return MoveCheckStatus::SRC_OUT_OF_BOUNDS;
	if (move.to.outOfBounds()) return MoveCheckStatus::DST_OUT_OF_BOUNDS;
	if (move.from == move.to) return MoveCheckStatus::STATIONARY_MOTION; //nothing burger

	const Piece& movingPiece = getPieceAt(move.from);
	const Piece& targetPiece = getPieceAt(move.to);
	if ((movingPiece.team == Team::WHITE) != whitesTurn) return MoveCheckStatus::TURN_TEAM_MISMATCH; //can't move other side's pieces

	//redundant simplifications
	int dx = move.to.x - move.from.x;
	int dy = move.to.y - move.from.y;
	if (movingPiece.type == PieceType::EMPTY) return MoveCheckStatus::MOVING_EMPTY_SQUARE; //voids can't move

	//filter out non-pawn promotions (can be encoded, but are never legal)
	auto promotionTarget = move.getPromotionTarget();
	if (promotionTarget && movingPiece.type != PieceType::PAWN) return MoveCheckStatus::PROMOTION_OF_NON_PAWN_PIECE;

	//checks the legality of the move WITHOUT verifying if the kings are checked (thus, a move that leaves allied king checked will pass this for instance). That verification is ran later.
	//Due to previous checks passing, we can assume that move is relatively clean here (doesn't try to go out of bounds, correct team, moving not empty square, etc).
	//But it still can fail due to other things, so a preliminary check is isolated to the lambda.
	MoveCheckStatus isLegalWithUncheckedKing = [&]() {
		if (targetPiece.type != PieceType::EMPTY && targetPiece.team == movingPiece.team) return MoveCheckStatus::MOVING_OBSTRUCTED_BY_ALLY; //obstructed by ally
		if (targetPiece.type == PieceType::KING) return MoveCheckStatus::TRYING_TO_CAPTURE_KING; //can't capture king

		std::array<Path, 8> slidingPaths; //paths for sliding movements (not king or knight)
		int pathCount = 0;
		//basic movement
		if (movingPiece.type == PieceType::KNIGHT)
		{
			return (std::abs(dx) == 1 && std::abs(dy) == 2) || (std::abs(dx) == 2 && std::abs(dy) == 1) ? LEGAL : MOVING_BAD_KNIGHT_JUMP;
		}
		if (movingPiece.type == PieceType::ROOK || movingPiece.type == PieceType::QUEEN)
		{
			slidingPaths[pathCount++] = this->getPath(move.from.x, move.from.y, -1, 0); //left
			slidingPaths[pathCount++] = this->getPath(move.from.x, move.from.y, 1, 0); //right
			slidingPaths[pathCount++] = this->getPath(move.from.x, move.from.y, 0, -1); //up
			slidingPaths[pathCount++] = this->getPath(move.from.x, move.from.y, 0, 1); //down
		}
		if (movingPiece.type == PieceType::BISHOP || movingPiece.type == PieceType::QUEEN)
		{
			slidingPaths[pathCount++] = this->getPath(move.from.x, move.from.y, -1, -1); //left-up diagonal
			slidingPaths[pathCount++] = this->getPath(move.from.x, move.from.y, 1, -1); //right-up
			slidingPaths[pathCount++] = this->getPath(move.from.x, move.from.y, -1, 1); //left-down
			slidingPaths[pathCount++] = this->getPath(move.from.x, move.from.y, 1, 1); //right-down
		}

		if (movingPiece.type == PieceType::PAWN)
		{
			auto promotionTarget = move.getPromotionTarget();
			bool onLastRank = move.to.y == (movingPiece.team == Team::WHITE ? 0 : 7);
			bool onStartingRank = move.from.y == (movingPiece.team == Team::WHITE ? 6 : 1);
			if (promotionTarget && !onLastRank) return MoveCheckStatus::PAWN_PROMOTION_ON_NONLAST_RANK; //can promote only if on last rank
			if (!promotionTarget && onLastRank) return MoveCheckStatus::PAWN_PROMOTION_ELIDED; //cannot stay as pawn on last rank, promotion required

			int forward = movingPiece.team == Team::WHITE ? -1 : 1; //direction of forward movement in board's Y for current turn's team
			bool moveToEnpassantSquare = enpassant && move.to == *enpassant;
			bool trulyEmpty = !moveToEnpassantSquare && targetPiece.type == PieceType::EMPTY;
			bool oneStep = dx == 0 && dy == forward && trulyEmpty;
			bool twoSteps = dx == 0 && dy == forward * 2 && onStartingRank && trulyEmpty;
			bool capture = abs(dx) == 1 && dy == forward && ((targetPiece.type != PieceType::EMPTY && targetPiece.team != movingPiece.team) || moveToEnpassantSquare);
			if (capture) return LEGAL;
			if (!oneStep && !twoSteps && !capture) return PAWN_GENERAL_MOVEMENT_ILLEGAL;
			//after these checks, path can still be obstructed, so a check for that is still needed
			slidingPaths[pathCount++] = this->getPath(move.from.x, move.from.y, 0, forward);
		}

		if (movingPiece.type == PieceType::KING)
		{
			if (abs(dx) > 1 || abs(dy) > 1) return MoveCheckStatus::KING_TOO_FAR;

			for (auto kingCheckSq : Square::all_inBounds())
			{
				Piece& p = getPieceAt(kingCheckSq);
				if (p.type == PieceType::KING && p.team != movingPiece.team)
				{
					if (abs(move.to.x - kingCheckSq.x) <= 1 && abs(move.to.y - kingCheckSq.y) <= 1) return MoveCheckStatus::KING_RUNS_INTO_ANOTHER_KING;
					else break;
				}
			}
			return MoveCheckStatus::LEGAL;
		}

		if (pathCount != 0) //sliding pieces paths can be blocked by other pieces, so scan the valid paths for whether they contain the target square
		{
			assert(movingPiece.type != PieceType::KNIGHT);
			bool pathFound = false;
			for (int i = 0; i < pathCount; ++i)
			{
				pathFound = slidingPaths[i].isPointOnPath(move.to.x, move.to.y);
				if (pathFound) break;
			}
			if (!pathFound) return MOVING_SLIDING_PATHS_EXIST_BUT_NONE_MATCH;
		}

		return LEGAL;
		}();

	if (isLegalWithUncheckedKing != MoveCheckStatus::LEGAL) return isLegalWithUncheckedKing;

	auto rollback = this->makeMove(move);
	bool stillChecked = isTeamsKingChecked(whitesTurn ? Team::BLACK : Team::WHITE); //TODO: check this
	this->rollbackMove(rollback);
	return stillChecked ? MoveCheckStatus::MOVING_TEAMS_KING_IN_CHECK : MoveCheckStatus::LEGAL;
}

bool GameState::isTeamsKingChecked(Team team)
{
	if (team == Team::NONE) throw std::runtime_error("Attempted to check checked status for NONE team");
	int kingX = -1, kingY = -1;
	int i = 0;
	for (const auto& it : board)
	{
		if (it.team == team && it.type == PieceType::KING)
		{
			kingX = i % 8;
			kingY = i / 8;
			break;
		}
		++i;
	}

	return squareIsAttackedFor(kingX, kingY, team);
}
*/
MoveResult GameState::tryMakeMove(const Move& move)
{
	DerivedData dd = deriveStateData();
	MoveResult ret;
	ret.applied = false;
	ret.checkStatus = MoveCheckStatus::ILLEGAL_REASON_NOT_SPECIFIED;
	if (std::find(dd.legalMoveList.begin(), dd.legalMoveList.end(), move) != dd.legalMoveList.end())
	{
		ret.applied = true;
		ret.rollbackData = this->makeMove(move);
		ret.checkStatus = MoveCheckStatus::LEGAL;
	}
	return ret;
}

MoveRollbackData GameState::makeMove(const Move& move)
{
	MoveRollbackData rollback = st;
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
			//st.board.removePiece(move.from.toIndex());
			st.board.pawns &= ~srcBit;
			st.board.white &= ~srcBit;
			st.board.black &= ~srcBit;
			if (st.whitesTurn) st.board.white |= dstBit;
			else st.board.black |= dstBit;

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
	this->seenPositions[st.board]++;

	return rollback;
}

void GameState::rollbackMove(const MoveRollbackData& rollbackData)
{
	assert(seenPositions[st.board] >= 1);
	if (--seenPositions[st.board] == 0) seenPositions.erase(st.board);
	this->st = rollbackData;
}

DerivedData GameState::deriveStateData(DeriveOptions opts)
{
	DerivedData ret;
	ret.motionMatrix = { 0 };
	uint64_t allies = st.whitesTurn ? st.board.white : st.board.black;
	uint64_t enemies = st.whitesTurn ? st.board.black : st.board.white;
	uint64_t notEmpty = st.board.black | st.board.white;
	uint64_t empty = ~notEmpty;

	uint64_t squaresAttackedByWhite = 0, squaresAttackedByBlack = 0;
	for (int i = 0; i < 64; ++i)
	{
		uint64_t bit = 1ull << i;
		if (bit & empty) continue; //TODO: can instead start and increase i by lzcnt?
		uint64_t teamedBit = bit & allies;
		uint64_t otherTeamBit = bit & enemies;

		int fromX = i % 8;
		int fromY = i / 8;
		//put all these bits into LSB
		//bit >>= 63 - i;
		//teamedBit >>= 63 - i;
		//otherTeamBit >>= 63 - i;

		uint64_t basicMovement = 0;
		//TODO: ifs for now, optimize later
		if (bit & st.board.kings) basicMovement = Globals::kingMoves[i];
		if (bit & st.board.knights) basicMovement = Globals::knightMoves[i];

		//Sliding pieces (rooks, bishops, queens and pawn forward moves) currently use "blacklist":
		//Start will all squares blacklisted, whitelist until first occluder. 
		//But still whitelist the occluder square, since we can capture enemy piece, and allies are filtered out by final &= allies
		uint64_t rookMoves = 0, bishopMoves = 0;
		if (bit & (st.board.rooks | st.board.queens | st.board.pawns))
		{
			uint64_t rookOccluders = -1ll;
			for (int dstX = fromX - 1; dstX >= 0; --dstX)
			{
				uint64_t k = 1ull << (fromY * 8 + dstX);
				rookOccluders &= ~k;
				if (notEmpty & k) break;
			}
			for (int dstX = fromX + 1; dstX < 8; ++dstX)
			{
				uint64_t k = 1ull << (fromY * 8 + dstX);
				rookOccluders &= ~k;
				if (notEmpty & k) break;
			}
			for (int dstY = fromY + 1; dstY < 8; ++dstY)
			{
				uint64_t k = 1ull << (dstY * 8 + fromX);
				rookOccluders &= ~k;
				if (notEmpty & k) break;
			}
			for (int dstY = fromY - 1; dstY >= 0; --dstY)
			{
				uint64_t k = 1ull << (dstY * 8 + fromX);
				rookOccluders &= ~k;
				if (notEmpty & k) break;
			}
			rookMoves = Globals::rookMoves[i] & ~rookOccluders;
		}
		if (bit & (st.board.bishops | st.board.queens))
		{
			uint64_t bishopOccluders = -1ll;
			int dstX = fromX - 1, dstY = fromY - 1;
			for (; dstX >= 0 && dstY >= 0; --dstX, --dstY) //left-up diagonal
			{
				uint64_t k = 1ull << (dstY * 8 + dstX);
				bishopOccluders &= ~k;
				if (notEmpty & k) break;
			}
			for (dstX = fromX + 1, dstY = fromY - 1; dstX < 8 && dstY >= 0; ++dstX, --dstY) //right-up diagonal
			{
				uint64_t k = 1ull << (dstY * 8 + dstX);
				bishopOccluders &= ~k;
				if (notEmpty & k) break;
			}
			for (dstX = fromX - 1, dstY = fromY + 1; dstX >= 0 && dstY < 8; --dstX, ++dstY) //left-down diagonal
			{
				uint64_t k = 1ull << (dstY * 8 + dstX);
				bishopOccluders &= ~k;
				if (notEmpty & k) break;
			}
			for (dstX = fromX + 1, dstY = fromY + 1; dstX < 8 && dstY < 8; ++dstX, ++dstY) //right-down diagonal
			{
				uint64_t k = 1ull << (dstY * 8 + dstX);
				bishopOccluders &= ~k;
				if (notEmpty & k) break;
			}
			bishopMoves = Globals::bishopMoves[i] & ~bishopOccluders;
		}

		uint64_t pawnMoves = 0, pawnCaptures = 0;
		if (bit & st.board.pawns & st.board.black)
		{
			pawnMoves = (Globals::blackPawnMoves[i] & rookMoves & empty);
			pawnCaptures = (Globals::blackPawnCaptures[i] & (enemies | st.enpassant));
			basicMovement = pawnMoves | pawnCaptures;
		}
		if (bit & st.board.pawns & st.board.white)
		{
			pawnMoves = (Globals::whitePawnMoves[i] & rookMoves & empty);
			pawnCaptures = (Globals::whitePawnCaptures[i] & (enemies | st.enpassant));
			basicMovement = pawnMoves | pawnCaptures;
		}
		if (bit & (st.board.bishops | st.board.queens)) basicMovement |= bishopMoves;
		if (bit & (st.board.rooks | st.board.queens)) basicMovement |= rookMoves;

		//squares attacked are movements for all pieces, except kings (kinda) and pawns. King "attacks" are still "real" for check calculations,
		//but pawns always require separate care: en passant doesn't count for attacks (?) but normal diagonal captures do
		//basicMovement &= ~allies;
		if (bit & ~st.board.pawns)
		{
			if (bit & st.board.white) squaresAttackedByWhite |= basicMovement & ~st.board.white;
			if (bit & st.board.black) squaresAttackedByBlack |= basicMovement & ~st.board.black;
		}
		if (bit & st.board.pawns)
		{
			if (bit & st.board.white) squaresAttackedByWhite |= Globals::whitePawnCaptures[i] & ~st.board.white;
			if (bit & st.board.black) squaresAttackedByBlack |= Globals::blackPawnCaptures[i] & ~st.board.black;
		}
		if (!(bit & allies)) continue; //can't move empty or enemy pieces on our turn, but need that data for attacks calculation, so just don't allow writing out

		ret.motionMatrix[i] = basicMovement & ~allies & ~st.board.kings; //preliminary list, need to then filter out king moves into squares attacked by enemies
	}

	uint64_t squaresAttackedByEnemy = st.whitesTurn ? squaresAttackedByBlack : squaresAttackedByWhite;
	int kingIndex = _tzcnt_u64(allies & st.board.kings);
	ret.whiteKingIndex = _tzcnt_u64(st.board.kings & st.board.white);
	ret.blackKingIndex = _tzcnt_u64(st.board.kings & st.board.black);
	ret.motionMatrix[kingIndex] &= ~squaresAttackedByEnemy;
	ret.squaresAttackedByBlack = squaresAttackedByBlack;
	ret.squaresAttackedByWhite = squaresAttackedByWhite;

	ret.whiteChecked = st.board.kings & st.board.white & squaresAttackedByBlack;
	ret.blackChecked = st.board.kings & st.board.black & squaresAttackedByWhite;
	/*for (int i = 0; i < 64; ++i)
	{
		uint64_t bit = 1ull << i;
		if (board.kings & bit) continue; //TODO: lz/tzcnt for king index: kingIndex = tzcnt(allies & board.kings)

	}*/

	if (opts.makeLegalMoveList)
	{
		uint64_t lastRankForPawnsMask = st.whitesTurn ? 0xFFull : (0xFFull << 56);
		for (int srcInd = 0; srcInd < 64; ++srcInd)
		{
			uint64_t srcBit = 1ull << srcInd;
			if (ret.motionMatrix[srcInd] == 0) continue;
			for (int dstInd = 0; dstInd < 64; ++dstInd)
			{
				uint64_t dstBit = 1ull << dstInd;
				if (!(ret.motionMatrix[srcInd] & (1ull << dstInd))) continue;

				Move m(Square(srcInd), Square(dstInd), MoveType::MOVE_TO_SQUARE);

				bool promotion = (st.board.pawns & srcBit) && (lastRankForPawnsMask & dstBit);
				if (!promotion) ret.legalMoveList.push_back(m);
				else
				{
					for (auto it : { MoveType::MOVE_AND_PROMOTE_TO_QUEEN, MoveType::MOVE_AND_PROMOTE_TO_ROOK, MoveType::MOVE_AND_PROMOTE_TO_BISHOP, MoveType::MOVE_AND_PROMOTE_TO_KNIGHT })
					{
						m.type = it;
						ret.legalMoveList.push_back(m);
					}
				}
			}
		}

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
	}

	//TODO: pin checks rely on move lists, should force that flag on?
	if (!opts.skipPinChecks)
	{
		//final filter: disallow moves that put or leave our king in check
		DeriveOptions opts2;
		opts2.makeLegalMoveList = false;
		opts2.skipPinChecks = true;
		bool savedWhitesTurn = st.whitesTurn;
		for (int i = 0; i < ret.legalMoveList.size(); ++i)
		{
			MoveRollbackData rollback = this->makeMove(ret.legalMoveList[i]);
			DerivedData dd = deriveStateData(opts2);
			if ((savedWhitesTurn ? dd.whiteChecked : dd.blackChecked))
			{
				ret.legalMoveList[i--] = ret.legalMoveList.back();
				ret.legalMoveList.pop_back();
			}
			this->rollbackMove(rollback);
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