#include "GameState.h"
#include <sstream>
#include <assert.h>

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

Piece& GameState::getPieceAt(int x, int y)
{
	return board[y * 8 + x];
}

std::optional<std::pair<int, int>> GameState::getFurthestReachableSquare(const SimpleMove& move, int stepX, int stepY, int maxSteps)
{
	assert(move.srcX >= 0 && move.srcX < 8);
	assert(move.srcY >= 0 && move.srcY < 8);
	std::optional<std::pair<int, int>> lastSquare;
	const Piece& srcPiece = getPieceAt(move.srcX, move.srcY);

	int x = move.srcX + stepX;
	int y = move.srcY + stepY;
	int stepCounter = 0;
	for (;
		y >= 0 && y < 8 && x >= 0 && x < 8;
		y += stepY, x += stepX)
	{
		if (x == move.srcX && y == move.srcY) throw std::runtime_error("Self");
		const Piece& dstPiece = getPieceAt(x, y);
		if (dstPiece.type != PieceType::EMPTY)
		{
			if (dstPiece.team == srcPiece.team) return lastSquare;
			return std::make_pair(x, y);
		}
		lastSquare = std::make_pair(x, y);
		if (maxSteps > 0 && ++stepCounter >= maxSteps) return lastSquare;
	}
	return std::nullopt;
}

bool GameState::pieceCanPathTo(const SimpleMove& move, int stepX, int stepY)
{
	if (move.dstX < 0 || move.dstX >= 8 || move.srcX < 0 || move.srcX >= 8 || move.dstY < 0 || move.dstY >= 8 || move.srcX < 0 || move.srcX >= 8) return false;
	if (move.dstX == move.srcX && move.dstY == move.srcY) return false;

	Piece& target = getPieceAt(move.dstX, move.dstY);
	Piece& moving = getPieceAt(move.srcX, move.srcY);
	if (moving.type == PieceType::EMPTY) return false;
	if (target.team == moving.team && target.type != PieceType::EMPTY) return false;

	int x = move.srcX + stepX;
	int y = move.srcY + stepY;
	for (int i = 0; 
		x >= 0 && x < 8 && y >= 0 && y < 8; 
		++i, x += stepX, y += stepY)
	{
		Piece& t = getPieceAt(x, y);
		if (t.type != PieceType::EMPTY)
		{
			return (t.team != moving.team) && (x == move.dstX && y == move.dstY);
		}
		else if (x == move.dstX && y == move.dstY) return true;
	}
	return false;
}

/*
int GameState::countStepsToPathTo(const SimpleMove& move, int stepX, int stepY, int maxSteps) const
{
	int x = move.srcX;
	int y = move.srcY;
	for (int i = 0; i <= maxSteps; ++i, x += stepX, y += stepY)
	{
		if 
		Piece& currTarget = getPieceAt()
		if (x == move.dstX && y == move.dstY) return i;
	}
	return -1;
}*/

GameState GameState::fromFEN(const std::string& s)
{
	GameState ret;
	int boardIndex = 0;
	std::stringstream ss(s);
	std::string board, turn, castling, enpassant, halfmove, fullmove;
	ss >> board >> turn >> castling >> enpassant >> ret.halfmoves >> ret.fullmoves;

	ret.whitesTurn = turn[0] == 'w';
	ret.enpassant = -1;
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
		case 'K': ret.allowedCastlings[int(CastlingType::WHITE_KINGSIDE)] = true; break;
		case 'Q': ret.allowedCastlings[int(CastlingType::WHITE_QUEENSIDE)] = true; break;
		case 'k': ret.allowedCastlings[int(CastlingType::BLACK_KINGSIDE)] = true; break;
		case 'q': ret.allowedCastlings[int(CastlingType::BLACK_QUEENSIDE)] = true; break;
		default: break;
		}
	}
	if (enpassant[0] != '-')
	{
		int rank = enpassant[1] - '1'; //0 indexed
		int file = enpassant[0] - 'a'; //0 indexed;
		ret.enpassant = (7 - rank) * 8 + file;
	}

	for (auto& it : ret.board) assert(it.type != PieceType::UNINITIALIZED);
	assert(enpassant.size() == 1 || enpassant.size() == 2);
	return ret;
}
