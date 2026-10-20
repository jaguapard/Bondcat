#include <iostream>
#include <string>
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
	int fenFieldNumber = 0;

	std::array<char, 8> halfmoveCtrBuf = { 0 }, fullmoveCtrBuf = { 0 }, enpassantBuf = { 0 };
	int enpassantBufPos = 0, halfmoveCtrBufPos = 0, fullmoveCtrBufPos = 0;
	for (int i = 0; i < s.length(); ++i)
	{
		if (s[i] == ' ')
		{
			fenFieldNumber++;
			continue;
		}
		if (fenFieldNumber == 0)
		{
			if (s[i] >= '1' && s[i] <= '8')
			{
				for (int j = 0; j <= s[i] - '1'; ++j)
				{
					ret.board[boardIndex++] = EMPTY;
				}
			}
			else
			{
				switch (s[i])
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
		else if (fenFieldNumber == 1) ret.whitesTurn = s[i] == 'w';
		else if (fenFieldNumber == 2)
		{
			switch (s[i])
			{
			case 'K': ret.whiteCanKingsideCastle = true; break;
			case 'Q': ret.whiteCanQueensideCastle = true; break;
			case 'k': ret.blackCanKingsideCastle = true; break;
			case 'q': ret.blackCanQueensideCastle = true; break;
			default: break;
			}
		}
		else if (fenFieldNumber == 3)
		{
			if (enpassantBufPos >= enpassantBuf.size()) throw std::runtime_error("Enpassant overflow");
			enpassantBuf[enpassantBufPos++] = s[i];
		}
		else if (fenFieldNumber == 4)
		{
			if (halfmoveCtrBufPos >= halfmoveCtrBuf.size()) throw std::runtime_error("Halfmove counter overflow");
			halfmoveCtrBuf[halfmoveCtrBufPos++] = s[i];
		}
		else if (fenFieldNumber == 5)
		{
			if (fullmoveCtrBufPos >= fullmoveCtrBuf.size()) throw std::runtime_error("Fullmove counter overflow");
			fullmoveCtrBuf[fullmoveCtrBufPos++] = s[i];
		}
		else std::cerr << "FEN field counter: " << fenFieldNumber << " out of range for char at index " << i << ", skipping.\n";
	}

	for (auto& it : ret.board) assert(it != UNINITIALIZED);
	assert(enpassantBufPos == 1 || enpassantBufPos == 2);
	if (enpassantBuf[0] == '-') ret.enpassant = -1;
	else
	{
		int rank = enpassantBuf[1] - '1'; //0 indexed
		int file = enpassantBuf[0] - 'a'; //0 indexed;
		ret.enpassant = (7 - rank) * 8 + file;
	}
	ret.halfmoves = atol(halfmoveCtrBuf.data());
	ret.fullmoves = atol(fullmoveCtrBuf.data());

	return ret;
}
int main()
{
	std::string fenStr;
	std::getline(std::cin, fenStr);
	auto s = decodeFEN(fenStr);
	return 0;
}