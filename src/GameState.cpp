#include "GameState.h"
#include <sstream>
#include <assert.h>

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
		if (isLegalMove(m)) ret.push_back(m);
	}
	return ret;
}
std::vector<Move> GameState::getAllLegalMovesForPieceAt(int x, int y)
{
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
			if (isLegalMove(move)) ret.push_back(move);
			if (getPieceAt(x, y).type == PieceType::PAWN)
			{
				for (auto t : { MoveType::MOVE_AND_PROMOTE_TO_KNIGHT, MoveType::MOVE_AND_PROMOTE_TO_BISHOP, MoveType::MOVE_AND_PROMOTE_TO_ROOK, MoveType::MOVE_AND_PROMOTE_TO_QUEEN })
				{
					move.type = t;
					if (isLegalMove(move)) ret.push_back(move);
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

bool GameState::isLegalCastlingMove(Move move)
{
	if (!move.isCastlingMove()) return false;

	if ((!whiteCanKingsideCastle && move.type == MoveType::WHITE_KINGSIDE_CASTLE) ||
		(!blackCanKingsideCastle && move.type == MoveType::BLACK_KINGSIDE_CASTLE) ||
		(!blackCanQueensideCastle && move.type == MoveType::BLACK_QUEENSIDE_CASTLE) ||
		(!whiteCanQueensideCastle && move.type == MoveType::WHITE_QUEENSIDE_CASTLE)) return false;

	//castling locations are hardcoded
	auto [king, rook] = getPiecesAtCaslingSquares(move);
	if (king.type != PieceType::KING || rook.type != PieceType::ROOK || king.hasMoved || rook.hasMoved || king.team != rook.team) return false;

	Team castleTeam = (move.type == MoveType::WHITE_KINGSIDE_CASTLE || move.type == MoveType::WHITE_QUEENSIDE_CASTLE) ? Team::WHITE : Team::BLACK;
	bool kingsideCastle = (move.type == MoveType::WHITE_KINGSIDE_CASTLE || move.type == MoveType::BLACK_KINGSIDE_CASTLE);
	if (castleTeam != king.team) return false;
	if ((castleTeam == Team::WHITE) != whitesTurn) return false; //can't castle on enemies turn!

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
		if (getPieceAt(obstructionCheckX[i], kingY).type != PieceType::EMPTY || squareIsAttackedFor(attackedCheckX[i], kingY, castleTeam)) return false;
	}
	return true;
}

bool GameState::isLegalMove(Move move)
{
	bool isCastling = move.isCastlingMove();
	if (isCastling)
	{
		return isLegalCastlingMove(move);
	}

	//Non-castling, i.e. move or move+promote
	if (move.srcX < 0 || move.srcX >= 8 ||
		move.srcY < 0 || move.srcY >= 8 ||
		move.dstX < 0 || move.dstX >= 8 ||
		move.dstY < 0 || move.dstY >= 8) return false;
	if (move.dstX == move.srcX && move.dstY == move.srcY) return false; //nothing burger

	const Piece& movingPiece = getPieceAt(move.srcX, move.srcY);
	const Piece& targetPiece = getPieceAt(move.dstX, move.dstY);
	assert(movingPiece.type != PieceType::UNINITIALIZED);
	assert(targetPiece.type != PieceType::UNINITIALIZED);
	if ((movingPiece.team == Team::WHITE) != whitesTurn) return false; //can't move other side's pieces

	//redundant simplifications
	int dx = move.dstX - move.srcX;
	int dy = move.dstY - move.srcY;
	auto targetPair = std::make_pair(move.dstX, move.dstY);
	if (movingPiece.type == PieceType::EMPTY) return false; //voids can't move

	//filter out non-pawn promotions (can be encoded, but are never legal)
	auto promotionTarget = move.getPromotionTarget();
	if (promotionTarget && movingPiece.type != PieceType::PAWN) return false;

	//checks the legality of the move WITHOUT verifying if the kings are checked (thus, a move that leaves allied king checked will pass this for instance). That verification is ran later
	bool isLegalWithUncheckedKing = [&]() {
		if (targetPiece.team == movingPiece.team) return false; //obstructed by ally
		if (targetPiece.type == PieceType::KING) return false; //can't capture king

		std::array<Path, 8> paths;
		int pathCount = 0;
		//basic movement
		if (movingPiece.type == PieceType::KNIGHT)
		{
			//only 8 jump destinations available
			bool goodJump = (std::abs(dx) == 1 && std::abs(dy) == 2) || (std::abs(dx) == 2 && std::abs(dy) == 1);
			if (!goodJump) return false;
		}
		if (movingPiece.type == PieceType::ROOK || movingPiece.type == PieceType::QUEEN)
		{
			paths[pathCount++] = this->getPath(move.srcX, move.srcY, -1, 0); //left
			paths[pathCount++] = this->getPath(move.srcX, move.srcY, 1, 0); //right
			paths[pathCount++] = this->getPath(move.srcX, move.srcY, 0, -1); //up
			paths[pathCount++] = this->getPath(move.srcX, move.srcY, 0, 1); //down
		}
		if (movingPiece.type == PieceType::BISHOP || movingPiece.type == PieceType::QUEEN)
		{
			paths[pathCount++] = this->getPath(move.srcX, move.srcY, -1, -1); //left-up diagonal
			paths[pathCount++] = this->getPath(move.srcX, move.srcY, 1, -1); //right-up
			paths[pathCount++] = this->getPath(move.srcX, move.srcY, -1, 1); //left-down
			paths[pathCount++] = this->getPath(move.srcX, move.srcY, 1, 1); //right-down
		}

		if (movingPiece.type == PieceType::PAWN)
		{
			auto promotionTarget = move.getPromotionTarget();
			if (promotionTarget)
			{
				//can promote only if on last rank
				if (movingPiece.team == Team::WHITE && move.dstY != 0) return false;
				if (movingPiece.team == Team::BLACK && move.dstY != 7) return false;
			}
			else
			{
				//cannot stay as pawn on last rank, promotion required
				if (movingPiece.team == Team::WHITE && move.dstY == 0) return false;
				if (movingPiece.team == Team::BLACK && move.dstY == 7) return false;
			}

			int normDy = movingPiece.team == Team::WHITE ? -dy : dy; //dy modified to be positive for respective team's forward direction
			int normStep = movingPiece.team == Team::WHITE ? -1 : 1; //single steps modified to be positive for respective team's forward direction

			if (normDy < 1) return false; //pawns can't stay in place or go backwards
			if (movingPiece.hasMoved && normDy != 1) return false; //already moved pawns can only move by 1 step forward
			if (!movingPiece.hasMoved && normDy > 2) return false; //if pawn hasn't moved, it can't go more than 2 steps forward


			if (abs(dx) > 1) return false; //pawns can't move more that 1 file to the sides
			if (dx != 0 && normDy == 2) return false; //can only move forward 2 steps

			if (enpassantX == move.dstX && enpassantY == move.dstY)	return (abs(dx) == 1 && normDy == 1);

			if (targetPiece.type == PieceType::EMPTY)
			{
				bool capturingEnpassant = enpassantX == move.dstX && enpassantY == move.dstY;
				if (capturingEnpassant && abs(dx) != 1 && normDy != 1) return false;
				if (dx != 0) return false; //pawns can only move diagonally if capturing
			}
			else
			{
				if (abs(dx) != 1) return false; //can't capture without diagonal move
				return true;
			}

			paths[pathCount++] = this->getPath(move.srcX, move.srcY, 0, normStep);
		}

		if (movingPiece.type == PieceType::KING)
		{
			if (abs(dx) > 1 || abs(dy) > 1) return false;
			return true;
		}

		if (pathCount != 0) //scan paths only if piece is a type that uses that movement (not Knight). Otherwise, it's empty, and if (!pathFound) would return false denials
		{
			assert(movingPiece.type != PieceType::KNIGHT);
			bool pathFound = false;
			for (int i = 0; i < pathCount; ++i)
			{
				pathFound = paths[i].isPointOnPath(move.dstX, move.dstY);
				if (pathFound) break;
			}
			if (!pathFound) return false;
		}

		return true;
		}();

	if (!isLegalWithUncheckedKing) return false;

	//For movements and promotions, copy the game state, alter it with the attempted move and see if that leaves/puts moving side's king in check
	auto cpy = *this;
	auto& dstPiece = cpy.getPieceAt(move.dstX, move.dstY);
	if (promotionTarget)
	{
		dstPiece.type = *promotionTarget;
		dstPiece.team = movingPiece.team;
	}
	else dstPiece = movingPiece;
	dstPiece.hasMoved = true;
	cpy.getPieceAt(move.srcX, move.srcY).type = PieceType::EMPTY;

	if (cpy.whitesTurn && cpy.isTeamsKingChecked(Team::WHITE)) return false;
	if (!cpy.whitesTurn && cpy.isTeamsKingChecked(Team::BLACK)) return false;

	return true;
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
	return ret;
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
	//TODO: implement this
	return false;
}

Piece& GameState::getPieceAt(const char* algebraic)
{
	int x, y;
	rankAndFileToXY(algebraic, x, y);
	return getPieceAt(x, y);
}