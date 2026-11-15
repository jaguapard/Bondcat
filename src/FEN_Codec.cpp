
#include "FEN_Codec.h"
#include <sstream>
#include "BondcatException.h"
#include "Square.h"

FEN_Codec::FEN_Codec(const std::string& fen)
{
	std::stringstream ss(fen);

	std::string boardStr, turnStr, castling, enpassantStr;
	ss >> boardStr >> turnStr >> castling >> enpassantStr >> this->halfmoves >> this->fullmoves;

	if (turnStr != "w" && turnStr != "b") throw BondcatException("Invalid FEN string: " + fen + "\ninvalid turn string : " + turnStr);
	this->whitesTurn = (turnStr == "w");
	using enum TeamedPieceType;

	int boardPos = 0;
	for (auto c : boardStr)
	{
		if (c == '/') continue;
		if (c >= '1' && c <= '8')
		{
			for (int j = 0; j < c - '0'; ++j) board[boardPos++] = EMPTY;
		}
		else switch (c)
		{
		case 'K': board[boardPos++] = WHITE_KING; break; 
		case 'Q': board[boardPos++] = WHITE_QUEEN; break; 
		case 'R': board[boardPos++] = WHITE_ROOK; break; 
		case 'B': board[boardPos++] = WHITE_BISHOP; break; 
		case 'N': board[boardPos++] = WHITE_KNIGHT; break; 
		case 'P': board[boardPos++] = WHITE_PAWN; break; 
		
		case 'k': board[boardPos++] = BLACK_KING; break;
		case 'q': board[boardPos++] = BLACK_QUEEN; break;
		case 'r': board[boardPos++] = BLACK_ROOK; break;
		case 'b': board[boardPos++] = BLACK_BISHOP; break;
		case 'n': board[boardPos++] = BLACK_KNIGHT; break;
		case 'p': board[boardPos++] = BLACK_PAWN; break;
		default: throw BondcatException((std::stringstream() << "Invalid FEN string: " << fen <<
			"\ninvalid piece char at board square " << Square(boardPos).toAlgebraic() << " (index " << boardPos << ") : " << c).str());
		}
	}
	if (boardPos != 64) throw BondcatException((std::stringstream() << "Invalid FEN string: " << fen << ": board string has " << boardPos << " squares instead of 64").str());

	for (auto c : castling)
	{
		switch (c)
		{
		case 'K': this->castlingRights.whiteKingside = true; break;
		case 'Q': this->castlingRights.whiteQueenside = true; break;
		case 'k': this->castlingRights.blackKingside = true; break;
		case 'q': this->castlingRights.blackQueenside = true; break;
		case '-':
			if (castling.length() > 1) throw BondcatException("Invalid FEN string: " + fen + "\ninvalid castling string : " + castling);
			break;
		default: throw BondcatException("Invalid FEN string: " + fen + "\ninvalid castling char : " + c); break;
		}
	}

	if (enpassantStr.length() != 1 && enpassantStr.length() != 2) throw BondcatException("Invalid FEN string: " + fen + "\ninvalid enpassant string : " + enpassantStr);
	if (enpassantStr == "-") this->enpassant = std::nullopt;
	else this->enpassant = Square(enpassantStr);
}

std::string FEN_Codec::toFEN() const
{
	using enum TeamedPieceType;
	std::stringstream ss;
	for (int y = 0; y < 8; ++y)
	{
		int emptySquares = 0;
		for (int x = 0; x < 8; ++x)
		{
			const Piece& p = board[y * 8 + x];
			if (p.type == PieceType::EMPTY) ++emptySquares;
			else
			{
				if (emptySquares != 0) ss << emptySquares;
				emptySquares = 0;
				switch (p.teamedType)
				{
				case WHITE_KING: ss << 'K';  break;
				case WHITE_QUEEN: ss << 'Q';  break;
				case WHITE_ROOK: ss << 'R';  break;
				case WHITE_BISHOP: ss << 'B';  break;
				case WHITE_KNIGHT: ss << 'N';  break;
				case WHITE_PAWN: ss << 'P';  break;
				case BLACK_KING: ss << 'k';  break;
				case BLACK_QUEEN: ss << 'q';  break;
				case BLACK_ROOK: ss << 'r';  break;
				case BLACK_BISHOP: ss << 'b';  break;
				case BLACK_KNIGHT: ss << 'n';  break;
				case BLACK_PAWN: ss << 'p';  break;
				default: throw BondcatException((std::stringstream() << "Invalid FEN_Codec board representation: value " << int(p.teamedType) + " at index " << (y * 8 + x)).str());
				}
			}
		}
		if (emptySquares != 0) ss << emptySquares;
		if (y != 7) ss << '/';
	}

	ss << ' ' << (whitesTurn ? 'w' : 'b') << ' ';

	if (castlingRights.whiteKingside) ss << 'K';
	if (castlingRights.whiteQueenside) ss << 'Q';
	if (castlingRights.blackKingside) ss << 'k';
	if (castlingRights.blackQueenside) ss << 'q';
	if (castlingRights.nonePresent()) ss << '-';
	ss << ' ';

	if (enpassant) ss << enpassant->toAlgebraic();
	else ss << '-';
	ss << ' ';

	ss << halfmoves << ' ' << fullmoves;
	return ss.str();
}