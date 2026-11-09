#include "GameState.h"
#include <sstream>
#include <assert.h>
#include <unordered_map>

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
SearchReturn GameState::findBestMove(const SearchParams& params, int currDepth, uint64_t* nodesVisited)
{
	//if (currDepth >= params.maxDepth) //TODO: check if depth is off by one (i think it is)
	auto moves = getAllLegalMoves();
	SearchReturn ret;
	if (whitesTurn) ret.evaluation = -INFINITY;
	else ret.evaluation = INFINITY;

	bool first = true;
	Team currTeamsTurn = whitesTurn ? WHITE : BLACK;
	for (auto& move : moves)
	{
		GameState copy = *this;
		copy.tryApplyMove(move, true);
		copy.updateStatus();
		if (nodesVisited) ++*nodesVisited;

		float eval = 0;
		//if (fullmoves < 15 && getPieceAt(move.srcX, move.srcY).type == PieceType::ROOK && getPieceAt(move.srcX, move.srcY).team == currTeamsTurn) eval += whitesTurn ? 4.5 : -4.5;

		if (copy.status == GameStatus::ONGOING)
		{
			int i = -1;
			if (currDepth < params.maxDepth) eval = copy.findBestMove(params, currDepth + 1, nodesVisited).evaluation;
			else
			{
				float whiteScore = 0, blackScore = 0;

				/*
				if (copy.whiteCanKingsideCastle) whiteScore += 0.5;
				else if (copy.whiteCanQueensideCastle) whiteScore += 0.33;
				if (copy.blackCanKingsideCastle) blackScore += 0.5;
				else if (copy.blackCanQueensideCastle) blackScore += 0.33;*/

				for (auto& it : copy.board)
				{
					++i;
					if (it.type == PieceType::EMPTY) continue;
					float& score = (it.team == Team::WHITE) ? whiteScore : blackScore;
					if (it.type == PieceType::PAWN) score += 1;
					if (it.type == PieceType::KNIGHT) score += 3; //* knightMult[i];
					if (it.type == PieceType::BISHOP) score += 3.1;// * bishopMult[i];
					if (it.type == PieceType::ROOK) score += 5;
					if (it.type == PieceType::QUEEN) score += 9;
				}
				eval += whiteScore - blackScore;
			}
		}
		else
		{
			switch (copy.status)
			{
			case GameStatus::WHITE_WON_BY_TIME:
			case GameStatus::WHITE_WON_BY_CHECKMATE:
			case GameStatus::WHITE_WON_ENEMY_RESIGNED:
				eval = INFINITY; break;
			case GameStatus::BLACK_WON_BY_TIME:
			case GameStatus::BLACK_WON_BY_CHECKMATE:
			case GameStatus::BLACK_WON_ENEMY_RESIGNED:
				eval = -INFINITY; break;
			case GameStatus::DRAW_ACCEPTED:
			case GameStatus::DRAW_STALEMATE:
			case GameStatus::DRAW_THREEFOLD_REPETITION:
			case GameStatus::DRAW_INSUFFICIENT_MATERIAL:
				eval = 0; break;
			default:
				break;
			}
		}
		
		if (first || (whitesTurn && eval > ret.evaluation) || (!whitesTurn && eval < ret.evaluation))
		{
			ret.evaluation = eval;
			ret.bestMove = move;
			first = false;
		}
	}
	return ret;
}

GameState GameState::newGame()
{
	return fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void GameState::updateStatus()
{
	this->statusUpToDate = true;
	//TODO: set game's status flags (draw, victory, etc)

	Team nextTeamToMove = whitesTurn ? Team::WHITE : Team::BLACK;
	bool kingChecked = isTeamsKingChecked(nextTeamToMove);
	if (kingChecked)
	{
		for (int y = 0; y < 8; ++y)
		{
			for (int x = 0; x < 8; ++x)
			{
				Piece& p = getPieceAt(x, y);
				if (p.team != nextTeamToMove || p.type != PieceType::KING) continue;
				auto legalKingMoves = getAllLegalMovesForPieceAt(x, y);
				if (legalKingMoves.size() == 0)
				{
					this->status = whitesTurn ? GameStatus::BLACK_WON_BY_CHECKMATE : GameStatus::WHITE_WON_BY_CHECKMATE;
				}
			}
		}
	}
	else if (getAllLegalMoves().size() == 0) this->status = GameStatus::DRAW_STALEMATE;
	else this->status = GameStatus::ONGOING;

	if (this->halfmoves >= 100) this->status = GameStatus::DRAW_FIFTY_MOVE_RULE;
	//TODO: 3-fold repetition draws, insufficient material

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
}

void GameState::reset()
{
	*this = newGame();
}

std::vector<Move> GameState::getAllLegalMoves()
{
	std::vector<Move> ret;

	for (int y = 0; y < 8; ++y)
	{
		for (int x = 0; x < 8; ++x)
		{
			auto moves = getAllLegalMovesForPieceAt(x, y);
			for (auto& it : moves) ret.push_back(it);
		}
	}

	for (auto ct : { MoveType::WHITE_KINGSIDE_CASTLE, MoveType::WHITE_QUEENSIDE_CASTLE, MoveType::BLACK_KINGSIDE_CASTLE, MoveType::BLACK_QUEENSIDE_CASTLE })
	{
		Move m;
		m.type = ct;
		if (isLegalMove(m) == MoveCheckStatus::LEGAL) ret.push_back(m);
	}
	return ret;
}
std::vector<Move> GameState::getAllLegalMovesForPieceAt(int x, int y)
{
	if (!statusUpToDate) throw std::runtime_error("Status out of date");
	std::vector<Move> ret;
	for (int dstY = 0; dstY < 8; ++dstY)
	{
		for (int dstX = 0; dstX < 8; ++dstX)
		{
			Move move;
			move.type = MoveType::MOVE_TO_SQUARE;
			move.srcX = x;
			move.srcY = y;
			move.dstX = dstX;
			move.dstY = dstY;
			if (isLegalMove(move) == MoveCheckStatus::LEGAL) ret.push_back(move);
			if (getPieceAt(x, y).type == PieceType::PAWN)
			{
				for (auto t : { MoveType::MOVE_AND_PROMOTE_TO_KNIGHT, MoveType::MOVE_AND_PROMOTE_TO_BISHOP, MoveType::MOVE_AND_PROMOTE_TO_ROOK, MoveType::MOVE_AND_PROMOTE_TO_QUEEN })
				{
					move.type = t;
					if (isLegalMove(move) == MoveCheckStatus::LEGAL) ret.push_back(move);
				}
			}
		}
	}
	return ret;
}
/*
LegalMoves GameState::getAllLegalMoves()
{
	LegalMoves ret;
	//first, fill out basic movements, with very basic filtering
	for (CastlingType ct : {CastlingType::WHITE_KINGSIDE, CastlingType::BLACK_KINGSIDE, CastlingType::WHITE_QUEENSIDE, CastlingType::BLACK_QUEENSIDE })
	{
		if (allowedCastlings[(int)ct]) ret.castlings.push_back(ct);
	}

	for (int i = 0; i < 64; ++i)
	{
		const Piece& movingPiece = board[i];
		if (movingPiece.type == PieceType::EMPTY) continue;
		if ((movingPiece.team == Team::WHITE) != whitesTurn) continue; //white/black pieces move only on their team's moves

		SimpleMove m;
		m.srcX = i % 8;
		m.srcY = i / 8;
		
		if (movingPiece.type == PieceType::PAWN)
		{
			int forward = movingPiece.team == Team::BLACK ? 1 : -1;
			m.dstY = m.srcY + forward;
			for (int dx = -1; dx <= 1; ++dx)
			{
				m.dstX = m.srcX + dx;
				if (dx != 0 && getPieceAt(m.dstX, m.dstY).type != PieceType::EMPTY) ret.simple.emplace_back(m);
			}
			if (!movingPiece.hasMoved)
			{
				m.dstX = m.srcX;
				m.dstY = m.srcY + 2 * forward;
				ret.simple.emplace_back(m);
			}
		}
		if (movingPiece.type == PieceType::ROOK || movingPiece.type == PieceType::QUEEN)
		{
			for (int k = 0; k < 8; ++k)
			{
				m.dstX = m.srcX;
				m.dstY = k;
				ret.simple.emplace_back(m);
				m.dstX = k;
				m.dstY = m.srcY;
				ret.simple.emplace_back(m);
			}			
		}
	}

	for (int i = 0; i < ret.simple.size(); ++i)
	{
		bool passesCleaning = [&]() {
			auto& currMove = ret.simple[i];
			if (currMove.dstX < 0 || currMove.dstX >= 8 || currMove.dstY < 0 || currMove.dstY >= 8) return false;
			if (currMove.dstX == currMove.srcX && currMove.dstY == currMove.srcY) return false;
			
			Piece& targetPiece = getPieceAt(currMove.dstX, currMove.dstY);
			Piece& movingPiece = getPieceAt(currMove.srcX, currMove.srcY);
			//FIXME: yes, empty squares may have a team.
			if (targetPiece.team == movingPiece.team && targetPiece.type != PieceType::EMPTY) return false;
			
			return true;
			}();
		if (!passesCleaning)
		{
			ret.simple[i--] = ret.simple.back();
			ret.simple.pop_back();
		}
	}

	return ret;
}
*/
Piece& GameState::getPieceAt(int x, int y)
{
	return board[y * 8 + x];
}

std::pair<Piece&, Piece&> GameState::getPiecesAtCaslingSquares(Move move)
{
	assert(move.isCastlingMove());
	switch (move.type)
	{
	case MoveType::WHITE_KINGSIDE_CASTLE: return { getPieceAt("e1"), getPieceAt("h1") };
	case MoveType::BLACK_KINGSIDE_CASTLE: return { getPieceAt("e8"), getPieceAt("h8") };
	case MoveType::WHITE_QUEENSIDE_CASTLE: return { getPieceAt("e1"), getPieceAt("a1") };
	case MoveType::BLACK_QUEENSIDE_CASTLE: return { getPieceAt("e8"), getPieceAt("a8") };
	default: throw std::runtime_error("Invalid input for getPiecesAtCaslingSquares (move type is not one of the castling moves).");
	}
}

MoveCheckStatus GameState::isLegalCastlingMove(Move move)
{
	using enum MoveCheckStatus;
	if (!move.isCastlingMove()) return CASTLING_CHECK_ON_NONCASTLING_MOVE;

	if ((!whiteCanKingsideCastle && move.type == MoveType::WHITE_KINGSIDE_CASTLE) ||
		(!blackCanKingsideCastle && move.type == MoveType::BLACK_KINGSIDE_CASTLE) ||
		(!blackCanQueensideCastle && move.type == MoveType::BLACK_QUEENSIDE_CASTLE) ||
		(!whiteCanQueensideCastle && move.type == MoveType::WHITE_QUEENSIDE_CASTLE)) return CASTLING_RIGHTS_ABSENT;

	//castling locations are hardcoded
	auto [king, rook] = getPiecesAtCaslingSquares(move);
	if (king.type != PieceType::KING) return CASTLING_NOT_KING;
	if (rook.type != PieceType::ROOK) return CASTLING_NOT_ROOK;
	if (king.hasMoved) return CASTLING_KING_MOVED;
	if (rook.hasMoved) return CASTLING_ROOK_MOVED;
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
		if (getPieceAt(obstructionCheckX[i], kingY).type != PieceType::EMPTY) return CASTLING_PATH_OBSTRUCTED;
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
	if (move.srcX < 0 || move.srcX >= 8 || move.srcY < 0 || move.srcY >= 8) return MoveCheckStatus::SRC_OUT_OF_BOUNDS;
	if (move.dstX < 0 || move.dstX >= 8 || move.dstY < 0 || move.dstY >= 8) return MoveCheckStatus::DST_OUT_OF_BOUNDS;
	if (move.dstX == move.srcX && move.dstY == move.srcY) return MoveCheckStatus::STATIONARY_MOTION; //nothing burger

	const Piece& movingPiece = getPieceAt(move.srcX, move.srcY);
	const Piece& targetPiece = getPieceAt(move.dstX, move.dstY);
	assert(movingPiece.type != PieceType::UNINITIALIZED);
	assert(targetPiece.type != PieceType::UNINITIALIZED);
	if ((movingPiece.team == Team::WHITE) != whitesTurn) return MoveCheckStatus::TURN_TEAM_MISMATCH; //can't move other side's pieces

	//redundant simplifications
	int dx = move.dstX - move.srcX;
	int dy = move.dstY - move.srcY;
	auto targetPair = std::make_pair(move.dstX, move.dstY);
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
			slidingPaths[pathCount++] = this->getPath(move.srcX, move.srcY, -1, 0); //left
			slidingPaths[pathCount++] = this->getPath(move.srcX, move.srcY, 1, 0); //right
			slidingPaths[pathCount++] = this->getPath(move.srcX, move.srcY, 0, -1); //up
			slidingPaths[pathCount++] = this->getPath(move.srcX, move.srcY, 0, 1); //down
		}
		if (movingPiece.type == PieceType::BISHOP || movingPiece.type == PieceType::QUEEN)
		{
			slidingPaths[pathCount++] = this->getPath(move.srcX, move.srcY, -1, -1); //left-up diagonal
			slidingPaths[pathCount++] = this->getPath(move.srcX, move.srcY, 1, -1); //right-up
			slidingPaths[pathCount++] = this->getPath(move.srcX, move.srcY, -1, 1); //left-down
			slidingPaths[pathCount++] = this->getPath(move.srcX, move.srcY, 1, 1); //right-down
		}

		if (movingPiece.type == PieceType::PAWN)
		{
			auto promotionTarget = move.getPromotionTarget();
			bool onLastRank = move.dstY == (whitesTurn ? 0 : 7);
			if (promotionTarget && !onLastRank) return MoveCheckStatus::PAWN_PROMOTION_ON_NONLAST_RANK; //can promote only if on last rank
			if (!promotionTarget && onLastRank) return MoveCheckStatus::PAWN_PROMOTION_ELIDED; //cannot stay as pawn on last rank, promotion required

			int forward = movingPiece.team == Team::WHITE ? -1 : 1; //direction of forward movement in board's Y for current turn's team
			bool moveToEnpassantSquare = move.dstX == enpassantX && move.dstY == enpassantY;
			bool trulyEmpty = !moveToEnpassantSquare && targetPiece.type == PieceType::EMPTY;
			bool oneStep = dx == 0 && dy == forward && trulyEmpty;
			bool twoSteps = dx == 0 && dy == forward * 2 && !movingPiece.hasMoved && trulyEmpty;
			bool capture = abs(dx) == 1 && dy == forward && ((targetPiece.type != PieceType::EMPTY && targetPiece.team != movingPiece.team) || moveToEnpassantSquare);
			if (capture) return LEGAL;
			if (!oneStep && !twoSteps && !capture) return PAWN_GENERAL_MOVEMENT_ILLEGAL;
			//after these checks, path can still be obstructed, so a check for that is still needed
			slidingPaths[pathCount++] = this->getPath(move.srcX, move.srcY, 0, forward);
		}

		if (movingPiece.type == PieceType::KING)
		{
			if (abs(dx) > 1 || abs(dy) > 1) return MoveCheckStatus::KING_TOO_FAR;

			for (int y = 0; y < 8; ++y)
				for (int x = 0; x < 8; ++x)
				{
					Piece& p = getPieceAt(x, y);
					if (p.type == PieceType::KING && p.team != movingPiece.team)
					{
						if (abs(move.dstX - x) <= 1 && abs(move.dstY - y) <= 1) return MoveCheckStatus::KING_RUNS_INTO_ANOTHER_KING;
						else break;
					}
				}
			//TODO: king can't move into other king's squares
			return MoveCheckStatus::LEGAL;
		}

		if (pathCount != 0) //sliding pieces paths can be blocked by other pieces, so scan the valid paths for whether they contain the target square
		{
			assert(movingPiece.type != PieceType::KNIGHT);
			bool pathFound = false;
			for (int i = 0; i < pathCount; ++i)
			{
				pathFound = slidingPaths[i].isPointOnPath(move.dstX, move.dstY);
				if (pathFound) break;
			}
			if (!pathFound) return MOVING_SLIDING_PATHS_EXIST_BUT_NONE_MATCH;
		}

		return LEGAL;
		}();

	if (isLegalWithUncheckedKing != MoveCheckStatus::LEGAL) return isLegalWithUncheckedKing;

	auto copy = *this;
	copy.tryApplyMove(move, true);
	//NOTE: should use CURRENT game state's turn, not advanced one's!
	if (copy.isTeamsKingChecked(whitesTurn ? Team::WHITE : Team::BLACK)) return MoveCheckStatus::MOVING_TEAMS_KING_IN_CHECK; //move leaves or puts moving team's king in check
	return MoveCheckStatus::LEGAL;
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

std::pair<bool, MoveCheckStatus> GameState::tryApplyMove(Move move, bool skipValidation)
{
	using enum MoveCheckStatus;
	MoveCheckStatus status = LEGALITY_CHECK_SKIPPED;
	if (!skipValidation)
	{
		status = isLegalMove(move);
		if (status != MoveCheckStatus::LEGAL) return { false,status };
	}
	this->halfmoves++;
	this->whitesTurn ^= 1;
	if (this->whitesTurn) this->fullmoves++;

	bool eraseEnpassant = true;
	if (move.isCastlingMove())
	{
		auto [king, rook] = getPiecesAtCaslingSquares(move);
		const char *rookSq = nullptr, *kingSq = nullptr;
		if (move.type == MoveType::WHITE_KINGSIDE_CASTLE) { rookSq = "f1"; kingSq = "g1"; whiteCanKingsideCastle = false; whiteCanQueensideCastle = false; }
		if (move.type == MoveType::BLACK_KINGSIDE_CASTLE) { rookSq = "f8"; kingSq = "g8"; blackCanKingsideCastle = false; blackCanQueensideCastle = false; }
		if (move.type == MoveType::WHITE_QUEENSIDE_CASTLE) { rookSq = "d1"; kingSq = "c1"; whiteCanQueensideCastle = false; whiteCanKingsideCastle = false; }
		if (move.type == MoveType::BLACK_QUEENSIDE_CASTLE) { rookSq = "d8"; kingSq = "c8"; blackCanQueensideCastle = false; blackCanKingsideCastle = false; }
		getPieceAt(rookSq) = rook;
		getPieceAt(kingSq) = king;
		king.type = rook.type = PieceType::EMPTY;
	}
	else
	{
		Piece& movingPiece = getPieceAt(move.srcX, move.srcY);
		Piece& dstPiece = getPieceAt(move.dstX, move.dstY);
		if (dstPiece.type != PieceType::EMPTY && dstPiece.team != movingPiece.team) this->halfmoves = 0;
		auto promotionTarget = move.getPromotionTarget();
		if (promotionTarget)
		{
			dstPiece.type = *promotionTarget;
			dstPiece.team = movingPiece.team;
		}
		else dstPiece = movingPiece;
		dstPiece.hasMoved = true;
		
		if (movingPiece.type == PieceType::PAWN)
		{
			this->halfmoves = 0;
			int forwardForMovingPawn = move.dstY > move.srcY ? 1 : -1;
			if (abs(move.dstY - move.srcY) == 2)
			{
				enpassantX = move.srcX;
				enpassantY = move.srcY + forwardForMovingPawn;
				eraseEnpassant = false;
			}
			else if (move.dstX == enpassantX && move.dstY == enpassantY)
			{
				getPieceAt(enpassantX, enpassantY - forwardForMovingPawn).type = PieceType::EMPTY;
			}
		}
		movingPiece.type = PieceType::EMPTY;
	}
	
	if (eraseEnpassant) enpassantX = enpassantY = -1;
	statusUpToDate = false;
	return { true, status };
}

GameState GameState::fromFEN(const std::string& s)
{
	GameState ret;
	int boardIndex = 0;
	std::stringstream ss(s);
	std::string board, turn, castling, enpassant, halfmove, fullmove;
	ss >> board >> turn >> castling >> enpassant >> ret.halfmoves >> ret.fullmoves;

	ret.whitesTurn = turn[0] == 'w';
	ret.enpassantX = ret.enpassantY = -1;
	for (auto it : board)
	{
		if (it >= '1' && it <= '8')
		{
			for (int j = 0; j <= it - '1'; ++j)
			{
				ret.board[boardIndex++].type = PieceType::EMPTY;
			}
		}
		else
		{
			switch (it)
			{
				//case '/': break;
			case 'P': ret.board[boardIndex].team = Team::WHITE; ret.board[boardIndex++].type = PieceType::PAWN; break;
			case 'N': ret.board[boardIndex].team = Team::WHITE; ret.board[boardIndex++].type = PieceType::KNIGHT; break;
			case 'B': ret.board[boardIndex].team = Team::WHITE; ret.board[boardIndex++].type = PieceType::BISHOP; break;
			case 'R': ret.board[boardIndex].team = Team::WHITE; ret.board[boardIndex++].type = PieceType::ROOK; break;
			case 'Q': ret.board[boardIndex].team = Team::WHITE; ret.board[boardIndex++].type = PieceType::QUEEN; break;
			case 'K': ret.board[boardIndex].team = Team::WHITE; ret.board[boardIndex++].type = PieceType::KING; break;

			case 'p': ret.board[boardIndex].team = Team::BLACK; ret.board[boardIndex++].type = PieceType::PAWN; break;
			case 'n': ret.board[boardIndex].team = Team::BLACK; ret.board[boardIndex++].type = PieceType::KNIGHT; break;
			case 'b': ret.board[boardIndex].team = Team::BLACK; ret.board[boardIndex++].type = PieceType::BISHOP; break;
			case 'r': ret.board[boardIndex].team = Team::BLACK; ret.board[boardIndex++].type = PieceType::ROOK; break;
			case 'q': ret.board[boardIndex].team = Team::BLACK; ret.board[boardIndex++].type = PieceType::QUEEN; break;
			case 'k': ret.board[boardIndex].team = Team::BLACK; ret.board[boardIndex++].type = PieceType::KING; break;
			default: break;
			}
		}
	}

	for (int y = 0; y < 8; ++y)
	{
		for (int x = 0; x < 8; ++x)
		{
			Piece& p = ret.getPieceAt(x, y);
			if (p.type == PieceType::PAWN)
			{
				int startingY = p.team == Team::BLACK ? 1 : 6;
				p.hasMoved = y != startingY;
			}
		}
	}
	for (auto it : castling)
	{
		switch (it)
		{
		case 'K': ret.whiteCanKingsideCastle = true; break;
		case 'Q': ret.whiteCanQueensideCastle = true; break;
		case 'k': ret.blackCanKingsideCastle = true; break;
		case 'q': ret.blackCanQueensideCastle = true; break;
		default: break;
		}
	}
	if (enpassant[0] != '-')
	{
		ret.rankAndFileToXY(enpassant.c_str(), ret.enpassantX, ret.enpassantY); //TODO: check if this works!
	}

	for (auto& it : ret.board) assert(it.type != PieceType::UNINITIALIZED);
	assert(enpassant.size() == 1 || enpassant.size() == 2);
	ret.updateStatus();
	return ret;
}

std::string GameState::xyToAlgebraic(int x, int y)
{
	std::string s = "na";
	s[0] = 'a' + x;
	s[1] = '8' - y;
	return s;
}

std::string GameState::toFEN()
{
	//static std::unordered_map<PieceType, char> chr = {}
	std::stringstream ss;
	/*
	int emptySquares = 0;
	auto dump_empty = [&]() {
		ss << emptySquares;
		emptySquares = 0;
		};
		*/

	for (int y = 0; y < 8; ++y)
	{
		int emptySquares = 0;
		for (int x = 0; x < 8; ++x)
		{
			Piece& piece = getPieceAt(x, y);
			if (piece.type == PieceType::EMPTY) ++emptySquares;
			else
			{
				if (emptySquares != 0) ss << emptySquares;
				emptySquares = 0;
				bool w = piece.team == Team::WHITE;
				switch (piece.type)
				{
				case PieceType::PAWN: ss << (w ? 'P' : 'p'); break;
				case PieceType::KNIGHT: ss << (w ? 'N' : 'n'); break;
				case PieceType::BISHOP: ss << (w ? 'B' : 'b'); break;
				case PieceType::ROOK: ss << (w ? 'R' : 'r'); break;
				case PieceType::QUEEN: ss << (w ? 'Q' : 'q'); break;
				case PieceType::KING: ss << (w ? 'K' : 'k'); break;
				default: break;
				}
			}
		}
		if (emptySquares != 0) ss << emptySquares;
		if (y != 7) ss << '/';
	}
	
	ss << ' ' << (whitesTurn ? 'w' : 'b') << ' ';

	if (whiteCanKingsideCastle) ss << 'K';
	if (whiteCanQueensideCastle) ss << 'Q';
	if (blackCanKingsideCastle) ss << 'k';
	if (blackCanQueensideCastle) ss << 'q';
	if (!blackCanKingsideCastle && !whiteCanKingsideCastle && !blackCanQueensideCastle && !whiteCanQueensideCastle) ss << '-';
	ss << ' ';
	
	if (enpassantX != -1 && enpassantY != -1)
	{
		ss << xyToAlgebraic(enpassantX, enpassantY);
	}
	else ss << '-';

	ss << ' ' << halfmoves << ' ' << fullmoves;
	return ss.str();
}

void GameState::rankAndFileToXY(const char* rankAndFile, int& retX, int& retY) const
{
	assert(rankAndFile[0] >= 'a' && rankAndFile[0] <= 'h');
	assert(rankAndFile[1] >= '1' && rankAndFile[1] <= '8');
	retX = rankAndFile[0] - 'a';
	retY = 7 - (rankAndFile[1] - '1');
}

Path GameState::getPath(int startX, int startY, int stepX, int stepY)
{
	Path ret;
	ret.stepCount = 0;
	ret.startX = startX;
	ret.startY = startY;
	ret.stepX = stepX;
	ret.stepY = stepY;

	int x = startX, y = startY;
	Piece& movingPiece = getPieceAt(x, y);
	assert(movingPiece.type != PieceType::EMPTY);
	while (true)
	{
		x += stepX;
		y += stepY;
		if (x < 0 || x > 7 || y < 0 || y > 7) return ret;

		Piece& targetPiece = getPieceAt(x, y);
		bool shouldReturn = false;
		if (targetPiece.type != PieceType::EMPTY)
		{
			if (targetPiece.team == movingPiece.team) return ret; //return without increment, can't make that move
			else shouldReturn = true; //can capture enemy
		}
		ret.stepCount++;
		if (shouldReturn) return ret; //thus, this flag has to control the return
	}
}

bool GameState::squareIsAttackedFor(int x, int y, Team team)
{
	//TODO: is this correct?
	if (team == Team::NONE) throw std::runtime_error("Square attack checked for NONE team");

	int enemyPawnForward = team == Team::WHITE ? 1 : -1;
	for (int y2 = 0; y2 < 8; ++y2)
	{
		for (int x2 = 0; x2 < 8; ++x2)
		{
			
			Piece& attacker = getPieceAt(x2, y2);
			if (attacker.type == PieceType::EMPTY || attacker.team == team) continue;

			int dx = x - x2, dy = y - y2; //direction flipped since perspective is too (x2,y2 is the source of the threat, (x,y) is destination of it)
			if (attacker.type == PieceType::KNIGHT && ((std::abs(dx) == 1 && std::abs(dy) == 2) || (std::abs(dx) == 2 && std::abs(dy) == 1))) return true;
			if (attacker.type == PieceType::KING && abs(dx) < 1 && abs(dy) < 1) return true;
			if (attacker.type == PieceType::PAWN)
				if (abs(dx) == 1 && dy == enemyPawnForward) return true;

			std::array<Path, 8> slidingPaths;
			int pathCount = 0;

			if (attacker.type == PieceType::ROOK || attacker.type == PieceType::QUEEN)
			{
				slidingPaths[pathCount++] = this->getPath(x2,y2, -1, 0); //left
				slidingPaths[pathCount++] = this->getPath(x2, y2, 1, 0); //right
				slidingPaths[pathCount++] = this->getPath(x2, y2, 0, -1); //up
				slidingPaths[pathCount++] = this->getPath(x2, y2, 0, 1); //down
			}
			if (attacker.type == PieceType::BISHOP || attacker.type == PieceType::QUEEN)
			{
				slidingPaths[pathCount++] = this->getPath(x2, y2, -1, -1); //left-up diagonal
				slidingPaths[pathCount++] = this->getPath(x2, y2, 1, -1); //right-up
				slidingPaths[pathCount++] = this->getPath(x2, y2, -1, 1); //left-down
				slidingPaths[pathCount++] = this->getPath(x2, y2, 1, 1); //right-down
			}
			for (int i = 0; i < pathCount; ++i)
			{
				if (slidingPaths[i].isPointOnPath(x, y) && slidingPaths[i].stepCount != 0) return true;
			}
		}
	}
	return false;
}

Piece& GameState::getPieceAt(const char* algebraic)
{
	int x, y;
	rankAndFileToXY(algebraic, x, y);
	return getPieceAt(x, y);
}

bool Path::isPointOnPath(int x, int y)
{
	assert(stepX != 0 || stepY != 0);
	int nStepsX, nStepsY;
	if (stepX == 0)
	{
		if (x != startX) return false;
		nStepsY = (y - startY) / stepY;
		return nStepsY >= 0 && nStepsY <= stepCount;
	}
	if (stepY == 0)
	{
		if (y != startY) return false;
		nStepsX = (x - startX) / stepX;
		return nStepsX >= 0 && nStepsX <= stepCount;
	}

	nStepsX = (x - startX) / stepX;
	nStepsY = (y - startY) / stepY;
	return nStepsX >= 0 && nStepsY >= 0 && nStepsX == nStepsY && nStepsX <= stepCount && nStepsY <= stepCount;
}
