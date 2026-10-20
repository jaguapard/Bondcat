#include <iostream>
#include <string>
#include <sstream>
#include <array>
#include <assert.h>

enum Piece
{
	EMPTY,
	WHITE_PAWN,
	WHITE_ROOK,
	WHITE_QUEEN,
	WHITE_KING,
	WHITE_BISHOP,
	WHITE_KNIGHT,

	BLACK_PAWN,
	BLACK_ROOK,
	BLACK_QUEEN,
	BLACK_KING,
	BLACK_BISHOP,
	BLACK_KNIGHT,

	UNINITIALIZED,
};

struct GameState
{
	std::array<Piece, 64> board = { UNINITIALIZED };
	bool blackCanQueensideCastle = false, blackCanKingsideCastle = false;
	bool whiteCanQueensideCastle = false, whiteCanKingsideCastle = false;
	bool whitesTurn;
	int enpassant = -1;
	int halfmoves, fullmoves;
	//static std::unorder
};
typedef std::array<Piece, 64> board_t;

GameState decodeFEN(const std::string& s)
{
	GameState ret;
	int boardIndex = 0;
	std::stringstream ss(s);
	std::string board, turn, castling, enpassant, halfmove, fullmove;
	ss >> board >> turn >> castling >> enpassant >> ret.halfmoves >> ret.fullmoves;

	ret.whitesTurn = turn[0] == 'w';
	for (auto it : board)
	{
		if (it >= '1' && it <= '8')
		{
			for (int j = 0; j <= it - '1'; ++j)
			{
				ret.board[boardIndex++] = EMPTY;
			}
		}
		else
		{
			switch (it)
			{
				//case '/': break;
			case 'P': ret.board[boardIndex++] = WHITE_PAWN; break;
			case 'N': ret.board[boardIndex++] = WHITE_KNIGHT; break;
			case 'B': ret.board[boardIndex++] = WHITE_BISHOP; break;
			case 'R': ret.board[boardIndex++] = WHITE_ROOK; break;
			case 'Q': ret.board[boardIndex++] = WHITE_QUEEN; break;
			case 'K': ret.board[boardIndex++] = WHITE_KING; break;
			case 'p': ret.board[boardIndex++] = BLACK_PAWN; break;
			case 'n': ret.board[boardIndex++] = BLACK_KNIGHT; break;
			case 'b': ret.board[boardIndex++] = BLACK_BISHOP; break;
			case 'r': ret.board[boardIndex++] = BLACK_ROOK; break;
			case 'q': ret.board[boardIndex++] = BLACK_QUEEN; break;
			case 'k': ret.board[boardIndex++] = BLACK_KING; break;
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
		int rank = enpassant[1] - '1'; //0 indexed
		int file = enpassant[0] - 'a'; //0 indexed;
		ret.enpassant = (7 - rank) * 8 + file;
	}

	for (auto& it : ret.board) assert(it != UNINITIALIZED);
	assert(enpassant.size() == 1 || enpassant.size() == 2);
	return ret;
}
int main()
{
	std::string fenStr;
	std::getline(std::cin, fenStr);
	auto s = decodeFEN(fenStr);
	return 0;
}