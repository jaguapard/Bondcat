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

		int x = i % 8;
		int y = i / 8;
		SimpleMove m;
		m.srcX = x;
		m.srcY = y;
		
		switch (movingPiece.type)
		{
		case PieceType::PAWN:
		{
			int forward = movingPiece.team == Team::BLACK ? 1 : -1;
			m.dstX = x;
			m.dstY = y + forward;
			ret.simple.emplace_back(m);
			if (!movingPiece.hasMoved)
			{
				m.dstY += forward;
				ret.simple.emplace_back(m);
			}
		}
		break;
		default:
			break;
		}
	}

	for (int i = 0; i < ret.simple.size(); ++i)
	{
		bool passesCleaning = [&]() {
			auto& currMove = ret.simple[i];
			if (currMove.dstX < 0 || currMove.dstX >= 8 || currMove.dstY < 0 || currMove.dstY >= 8)
			{
				return false;
			}
			
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
