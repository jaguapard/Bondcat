#include <iostream>
#include <string>
#include <sstream>
#include <array>
#include <assert.h>
#include <vector>
#include <optional>

enum Team
{
	WHITE,
	BLACK,
};

enum class PieceType
{
	PAWN,
	ROOK,
	QUEEN,
	KING,
	BISHOP,
	KNIGHT,
	EMPTY,
	UNINITIALIZED,
};
struct Piece
{
	PieceType type = PieceType::UNINITIALIZED;
	Team team;
	bool hasMoved = false;
};

enum class MoveType
{
	MOVE_TO_SQUARE, //simple move to square and capture an enemy piece located there
	MOVE_AND_PROMOTE_TO_BISHOP,
	MOVE_AND_PROMOTE_TO_KNIGHT,
	MOVE_AND_PROMOTE_TO_ROOK,
	MOVE_AND_PROMOTE_TO_QUEEN,
	WHITE_KINGSIDE_CASTLE,
	BLACK_KINGSIDE_CASTLE,
	WHITE_QUEENSIDE_CASTLE,
	BLACK_QUEENSIDE_CASTLE,
};

//Describes a move attempted by the player. type describes the type of the move, square indices are ignored for castling (use hardcoded ones)
struct Move
{
	int srcX, srcY, dstX, dstY;
	MoveType type;

	//Returns a promotion target stored or nullopt if there is none. Does not check if it is legal
	std::optional<PieceType> getPromotionTarget() const
	{
		switch (type)
		{
		case MoveType::MOVE_AND_PROMOTE_TO_BISHOP:
			return PieceType::BISHOP;
		case MoveType::MOVE_AND_PROMOTE_TO_KNIGHT:
			return PieceType::KNIGHT;
		case MoveType::MOVE_AND_PROMOTE_TO_ROOK:
			return PieceType::ROOK;
		case MoveType::MOVE_AND_PROMOTE_TO_QUEEN:
			return PieceType::QUEEN;
		default:
			return std::nullopt;
		}
	}

	bool isCastlingMove()
	{
		switch (type)
		{
		case MoveType::WHITE_KINGSIDE_CASTLE:
		case MoveType::BLACK_KINGSIDE_CASTLE:
		case MoveType::WHITE_QUEENSIDE_CASTLE:
		case MoveType::BLACK_QUEENSIDE_CASTLE:
			return true;
		default: return false;
		}
	}
};

//Represents all legal moves that can be made on a board by using bit arrays.
//arr[i] shows the allowed squares for this piece.
//arr[6] == 10 means that piece at index 6 is allowed to move only to index 1 and 3 (10 == 2 | 8 == (1<<1) | (1<<3))
//values at and after index `size` are garbage
typedef std::array<uint64_t, 64> LegalMoves;
/*
struct LegalMoveArray
{
	std::array<uint64_t, 64> arr;
	int size = 0;
};*/
struct GameState
{
	std::array<Piece, 64> board;
	bool blackCanQueensideCastle = false, blackCanKingsideCastle = false;
	bool whiteCanQueensideCastle = false, whiteCanKingsideCastle = false;
	bool whitesTurn;
	int enpassantX = -1, enpassantY = -1;
	int halfmoves, fullmoves;
	//bool blackIsChecked = false, whiteIsChecked = false;

	//TODO: implement these
	bool blackIsChecked();
	bool whiteIsChecked();

	//Converts algebraic notation of a square to it's XY coordinated (i.e "a8" -> (0,0), "h1" -> (7,7))
	void rankFileToXY(const char* rankAndFile, int& retX, int& retY)
	{
		assert(rankAndFile[0] >= 'a' && rankAndFile[0] <= 'h');
		assert(rankAndFile[1] >= '1' && rankAndFile[0] <= '8');
		retX = rankAndFile[0] - 'a';
		retY = 7 - (rankAndFile[1] - '1');
	}

	Piece& getPieceAt(int x, int y)
	{
		return board[x + y * 8];
	}

	//
	template<int N>
	bool anySquareAttackedFor(const std::array<const char*, N>& algebraic, Team team)
	{

	}

	Piece& getPieceAt(const char* algebraic)
	{
		int x, y;
		rankFileToXY(algebraic, x, y);
		return getPieceAt(x, y);
	}

	//returns true if the square (x,y) is attacked by enemies of the input team 
	//(i.e. true for white means that black pieces attack this square), or false otherwise
	bool squareIsAttackedFor(int x, int y, Team team)
	{
		//TODO: implement this
	}

	//Returns pair of references to pieces at locations hardcoded based on type of the move supplied.
	//For castling moves, first element of the returned pair is the reference to starting king square
	//Second - target rook square
	//Undefined behavior for non-castling inputs
	//Note that it returns only the references to pieces at target squares, they are not guaranteed to be occupied, or be kings or rooks, castling legality has to be checked separately.
	std::pair<Piece&, Piece&> getPiecesAtCaslingSquares(Move move)
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

	//Checks if a move is legal, considering input Move adversarial (checking the validity of inputs, legality of the move, king checks). There're no requirements for src, dst squares or movement types, but hasMoved flag is trusted as-is (since it's filled out by our engine)
	bool isMoveLegal(Move move)
	{
		bool isCastling = move.isCastlingMove();
		std::optional<std::pair<Piece&, Piece&>> castlingPieces;
		if (!isCastling) //castling locations are hardcoded, thus, these checks have to be omitted for them
		{
			if (move.srcX < 0 || move.srcX >= 8 ||
				move.srcY < 0 || move.srcY >= 8 ||
				move.dstX < 0 || move.dstX >= 8 ||
				move.dstY < 0 || move.dstY >= 8) return false;

			if (move.dstX == move.srcX && move.dstY == move.srcY) return false; //nothing burger
		}
		else
		{
			castlingPieces = getPiecesAtCaslingSquares(move);
		}

		const Piece& movingPiece = getPieceAt(move.srcX, move.srcY);
		const Piece& targetPiece = getPieceAt(move.dstX, move.dstY);
		assert(movingPiece.type != PieceType::UNINITIALIZED);
		assert(targetPiece.type != PieceType::UNINITIALIZED);

		//redundant simplifications
		int dx = move.dstX - move.srcX;
		int dy = move.dstY - move.srcY;
		auto targetPair = std::make_pair(move.dstX, move.dstY);
		if (movingPiece.type == PieceType::EMPTY) return false; //voids can't move

		//filter out non-pawn promotions (can be encoded, but is never legal)
		auto promotionTarget = move.getPromotionTarget();
		if (promotionTarget && movingPiece.type != PieceType::PAWN) return false;

		//GameState stateWithMoveApplied = *this; //will be modified by 
		//checks the legality of the move WITHOUT verifying if the kings are checked (thus, a move that leaves allied king checked will pass this for instance). That verification is ran later
		bool isLegalWithUncheckedKing = [&]() {
			if (isCastling)
			{
				if (
					(!whiteCanKingsideCastle && move.type == MoveType::WHITE_KINGSIDE_CASTLE) ||
					(!blackCanKingsideCastle && move.type == MoveType::BLACK_KINGSIDE_CASTLE) ||
					(!blackCanQueensideCastle && move.type == MoveType::BLACK_QUEENSIDE_CASTLE) ||
					(!whiteCanQueensideCastle && move.type == MoveType::WHITE_QUEENSIDE_CASTLE)) return false;
				Piece& king = castlingPieces->first;
				Piece& rook = castlingPieces->second;
				if (king.type != PieceType::KING || rook.type != PieceType::ROOK || king.hasMoved || rook.hasMoved || king.team != rook.team) return false;

				Team castleTeam = (move.type == MoveType::WHITE_KINGSIDE_CASTLE || move.type == MoveType::WHITE_QUEENSIDE_CASTLE) ? Team::WHITE : Team::BLACK;
				bool kingsideCastle = (move.type == MoveType::WHITE_KINGSIDE_CASTLE || move.type == MoveType::BLACK_KINGSIDE_CASTLE);
				if (castleTeam != king.team) return false;

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

			else //promotion or simple movements // (move.type != MoveType::CASTLE) 
			{
				if (targetPiece.team == movingPiece.team) return false; //obstructed by ally
				if (targetPiece.type == PieceType::KING) return false; //can't capture king

				std::array<std::optional<std::pair<int, int>>, 8> pathingLimits;
				int limitsLen = 0;
				//basic movement
				if (movingPiece.type == PieceType::KNIGHT)
				{
					//only 8 jump destinations available
					bool goodJump = (std::abs(dx) == 1 && std::abs(dy) == 2) || (std::abs(dx) == 2 && std::abs(dy) == 1);
					if (!goodJump) return false;
				}
				if (movingPiece.type == PieceType::ROOK || movingPiece.type == PieceType::QUEEN)
				{
					pathingLimits[limitsLen++] = this->getPathingLimits(move, -1, 0); //left
					pathingLimits[limitsLen++] = this->getPathingLimits(move, 1, 0); //right
					pathingLimits[limitsLen++] = this->getPathingLimits(move, 0, -1); //up
					pathingLimits[limitsLen++] = this->getPathingLimits(move, 0, 1); //down
				}
				if (movingPiece.type == PieceType::BISHOP || movingPiece.type == PieceType::QUEEN)
				{
					pathingLimits[limitsLen++] = this->getPathingLimits(move, -1, -1); //left-up diagonal
					pathingLimits[limitsLen++] = this->getPathingLimits(move, 1, -1); //right-up
					pathingLimits[limitsLen++] = this->getPathingLimits(move, -1, 1); //left-down
					pathingLimits[limitsLen++] = this->getPathingLimits(move, 1, 1); //right-down
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
					if (normDy < 1 || normDy > 2) return false; //pawns can't stay in place, go backwards or go more than 2 ranks at a time
					if (normDy == 2 && movingPiece.hasMoved) return false; //pawns move by 2 ranks only on first move

					if (abs(dx) > 1) return false; //pawns can't move more that 1 file to the sides

					if (enpassantX == move.dstX && enpassantY == move.dstY)	return (abs(dx) == 1 && abs(dy) == 1);

					if (targetPiece.type == PieceType::EMPTY)
					{
						if (dx != 0) return false; //pawns can only move diagonally if capturing
					}
					else
					{
						if (abs(dx) != 1) return false; //can't capture without diagonal move
						return true;
					}

					pathingLimits[limitsLen++] = this->getPathingLimits(move, 0, normStep);
				}

				if (movingPiece.type == PieceType::KING)
				{
					if (abs(dx) > 1 || abs(dy) > 1) return false;
					return true;
				}

				if (limitsLen != 0) //scan pathingLimits only if piece is a type that uses that movement (not Knight). Otherwise, it's empty, and if (!pathFound) would return false denials
				{
					assert(movingPiece.type != PieceType::KNIGHT);
					bool pathFound = false;
					for (int i = 0; i < limitsLen; ++i)
					{
						if (pathingLimits[i] && pathingLimits[i].value() == targetPair)
						{
							pathFound = true; break;
						}
					}
					if (!pathFound) return false;
				}
			}

			return true;
			}();

		if (!isLegalWithUncheckedKing) return false;
		if (isCastling) return isLegalWithUncheckedKing; //castling check already self-sufficient

		//For movements and promotions, copy the game state, alter it with the attempted move and see if that leaves/puts moving side's king in check
		auto cpy = *this;
		auto& dstPiece = cpy.getPieceAt(move.dstX, move.dstY);
		if (promotionTarget)
		{
			dstPiece.type = *promotionTarget;
			dstPiece.hasMoved = true;
			dstPiece.team = movingPiece.team;
		}
		else dstPiece = movingPiece;
		cpy.getPieceAt(move.srcX, move.srcY).type = PieceType::EMPTY;

		if (cpy.whitesTurn && cpy.whiteIsChecked()) return false;
		if (!cpy.whitesTurn && cpy.blackIsChecked()) return false;

		return true;
	}

	//Attempts to move from srcX,srcY to dstX,dstY using discrette steps of size stepX,stepY until encountering allied piece or capturable enemy piece. Returns the furthest possible square (as pair of X and Y positions), or nullopt if direction is immediately obstructed and no step can be made. Move must contain in-bounds src and dst coordinates
	std::optional<std::pair<int, int>> getPathingLimits(const Move& move, int stepX, int stepY)
	{
		assert(move.srcX >= 0 && move.srcX < 8);
		assert(move.srcY >= 0 && move.srcY < 8);
		std::optional<std::pair<int, int>> ret;
		const Piece& srcPiece = getPieceAt(move.srcX, move.srcY);

		for (
			int x = move.srcX + stepX,
			int y = move.srcY + stepY;
			y >= 0 && y < 8 && x >= 0 && x < 8;
			y += stepY, x += stepX)
		{
			const Piece& dstPiece = getPieceAt(x, y);
			bool enemyMet = false;
			if (dstPiece.type != PieceType::EMPTY)
			{
				if (dstPiece.team == srcPiece.team) break;
				enemyMet = true;
			}
			ret = std::make_pair(x, y);
			if (enemyMet) break;
		}
		return ret;
	}

	void lastNonObstructed(int srcX, int srcY, int dstX, int dstY, int stepX, int stepY, int& retX, int& retY)
	{
		for (int y = srcY; y >= 0 && y < 8; y += stepY)
		{
			for (int x = srcX; x >= 0 && x < 8; x += stepX)
			{

			}
		}
	}

	/*
	LegalMoves getAllLegalMoves()
	{
		LegalMoves ret = { 0 };
		for (int i = 0; i < 64; ++i)
		{
			if (board[i].type == PieceType::EMPTY) continue;
			if (board[i].type == PieceType::PAWN)
			{
				if ()
			}
		}
	}*/
	//static std::unorder

	//returns true if the move is legal, false otherwise
	//@param square index of the square that piece performing the move resides in, 0 to 63 inclusive.
	//@param type type of movement. For castling, the king performs the move with targetSquare being allied rook
	//@param targetSquare index of the destination square. Ignored for promotions

	//Returns a vector of moves that satisfy the basic movement requirements for existing pieces. Does not run obstruction checks
	std::vector<Move> getAllLegalMoves()
	{
		std::vector<Move> ret;

		//First, populate with basic movements
		for (int srcY = 0; srcY < 8; ++srcY)
		{
			for (int srcX = 0; srcX < 8; ++srcX)
			{
				const Piece& piece = getPieceAt(srcX, srcY);
				if (piece.type == PieceType::EMPTY) continue;

				Move move;
				move.srcX = srcX;
				move.srcY = srcY;
				move.type = MoveType::MOVE_TO_SQUARE;

				if (piece.type == PieceType::KING)
				{
					for (int dy = -1; dy <= 1; ++dy)
					{
						for (int dx = -1; dx <= 1; ++dx)
						{
							//if (dx == 0 && dy == 0) continue;
							move.dstX = srcX + dx;
							move.dstY = srcY + dy;
							//if (getPieceAt(move.dstX,move.dstY).type == PieceType::EMPTY)
							ret.push_back(move);
						}
					}
				}

				if (piece.type == PieceType::ROOK || piece.type == PieceType::QUEEN)
				{
					//left
					for (int x = move.srcX - 1; x >= 0; --x)
					{
						bool enemyMet = false;
						auto& p = getPieceAt(x, move.srcY);
						if (p.type != PieceType::EMPTY)
						{
							if (p.team != piece.team) enemyMet = true;
							else break;
						}
						move.dstX = x;
						move.dstY = move.srcY;
						ret.push_back(move);
						if (enemyMet) break;
					}
					//right
					for (int x = move.srcX + 1; x < 8; ++x)
					{
						bool enemyMet = false;
						auto& p = getPieceAt(x, move.srcY);
						if (p.type != PieceType::EMPTY)
						{
							if (p.team != piece.team) enemyMet = true;
							else break;
						}
						move.dstX = x;
						move.dstY = move.srcY;
						ret.push_back(move);
					}
					//up
					for (int y = move.srcY - 1; y >= 0; --y)
					{
						if (getPieceAt(move.srcX, y).type != PieceType::EMPTY) break;
						move.dstX = move.srcX;
						move.dstY = y;
						ret.push_back(move);
					}
					//down
					for (int y = move.srcY + 1; y < 8; ++y)
					{
						if (getPieceAt(move.srcX, y).type != PieceType::EMPTY) break;
						move.dstX = move.srcX;
						move.dstY = y;
						ret.push_back(move);
					}
				}

				if (piece.type == PieceType::BISHOP || piece.type == PieceType::QUEEN)
				{

				}
			}
		}
		
		//remove the moves that end up out of bounds, not moving the original piece or destination square occupied by ally (actual obstruction check runs later)
		for (size_t i = 0; i < ret.size(); ++i)
		{
			const Move& move = ret[i];
			const Piece& movingPiece = getPieceAt(move.srcX, move.srcY);
			const Piece& targetPiece = getPieceAt(move.dstX, move.dstY);
			if (movingPiece.team==targetPiece.team || )
		}

	}

	/*
	bool isMoveLegal(Move move)
	{
		const Piece& movingPiece = board[move.pieceIndex];
		const Piece& targetPiece = board[move.targetIndex];
		if (movingPiece.type == PieceType::EMPTY) return false;
		if (move.pieceIndex < 0 || move.pieceIndex >= 64) throw std::runtime_error("Invalid piece index");
		if (move.targetIndex < 0 || move.targetIndex >= 64) throw std::runtime_error("Invalid target index");

		//int fromRank = 8 - move.pieceIndex / 8;
		//int fromFile = move.pieceIndex % 8;
		int fromRow = move.pieceIndex % 8;
		int fromCol = move.pieceIndex / 8;
		int dstX = move.targetIndex % 8;
		int dstY = move.targetIndex / 8;
		
		if (move.type == MoveType::CASTLE)
		{
			if (movingPiece.type != PieceType::KING) return false;
			if (movingPiece.hasMoved || targetPiece.type != PieceType::ROOK || targetPiece.hasMoved) return false;
			if (movingPiece.team == BLACK && !blackCanKingsideCastle && move.targetIndex == 7) return false;
			if (movingPiece.team == BLACK && !blackCanQueensideCastle && move.targetIndex == 3) return false;
			if (movingPiece.team == WHITE && !whiteCanQueensideCastle && move.targetIndex == 56) return false;
			if (movingPiece.team == WHITE && !whiteCanKingsideCastle && move.targetIndex == 63) return false;
		}

		if (move.type == MoveType::MOVE_TO_SQUARE)
		{
			if (movingPiece.team == targetPiece.team) return false; //target square is obstructed by allied piece

			//if (movingPiece.team == BLACK && blackIsChecked)
			if (movingPiece.type == PieceType::PAWN)
			{
				//TODO: enpassant
				if (targetPiece.type == PieceType::EMPTY)
				{
					//pawns can only move forward if destination is empty
					if (fromCol != dstY) return false;
					if (movingPiece.hasMoved)
					{
						//only 1 step forward allowed if already moved
						if (movingPiece.team == BLACK && (fromRow - dstX != 1)) return false;
						if (movingPiece.team == WHITE && (fromRow - dstX != -1)) return false;
					}
					else
					{
						//allow two steps if hasn't yet moved. Obstruction checks will go later
						if (movingPiece.team == BLACK && (fromRow - dstX != 1) && (fromRow - dstX != 2)) return false;
						if (movingPiece.team == WHITE && (fromRow - dstX != -1) && (fromRow - dstX != -2)) return false;
					}
				}
				else
				{

				}
			}

			if (movingPiece.type == PieceType::BISHOP)
			{

			}
		}

		

		return true;
	}
	*/
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
		ret.enpassantX = file;
		ret.enpassantY = 7 - rank;
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