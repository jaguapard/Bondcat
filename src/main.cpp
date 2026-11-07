#include <iostream>
#include <string>
#include <sstream>
#include <array>
#include <assert.h>
#include <vector>
#include <optional>
#include <unordered_map>

#include <SDL3/SDL_image.h>

#include "GameState.h"
#include "Piece.h"

#include <SDL3/SDL.h>

#pragma comment(lib,"SDL3.lib")
#pragma comment(lib,"SDL3_image.lib")
#undef main

/*
class Piece
{
public:
	PieceType __declspec(property(get = get_type, put = set_type)) type;

	PieceType get_type() const
	{
		return static_cast<PieceType>(data & 7);
	}

	void set_type(PieceType type)
	{
		static_assert(static_cast<uint8_t>(PieceType::COUNT) <= 8);
		data &= ~((1 << 4) - 1);
		data |= static_cast<uint8_t>(type);
	}
private:
	uint8_t data;
};*/

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

//Represents all legal moves that can be made on a board by using bit arrays.
//arr[i] shows the allowed squares for this piece.
//arr[6] == 10 means that piece at index 6 is allowed to move only to index 1 and 3 (10 == 2 | 8 == (1<<1) | (1<<3))
//values at and after index `size` are garbage

/*
struct LegalMoveArray
{
	std::array<uint64_t, 64> arr;
	int size = 0;
};*/
struct __GameState
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

	bool isLegalCastlingMove(Move move)
	{
		if (!move.isCastlingMove()) return false;
		std::optional<std::pair<Piece&, Piece&>> castlingPieces = getPiecesAtCaslingSquares(move);

		//castling locations are hardcoded
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


	//Attempts to move from srcX,srcY to dstX,dstY using discrette steps of size stepX,stepY until encountering allied piece or capturable enemy piece. Returns the furthest possible square (as pair of X and Y positions), or nullopt if direction is immediately obstructed and no step can be made. Move must contain in-bounds src and dst coordinates
	std::optional<std::pair<int, int>> getPathingLimits(const Move& move, int stepX, int stepY)
	{
		assert(move.srcX >= 0 && move.srcX < 8);
		assert(move.srcY >= 0 && move.srcY < 8);
		std::optional<std::pair<int, int>> ret;
		const Piece& srcPiece = getPieceAt(move.srcX, move.srcY);

		int x = move.srcX + stepX;
		int y = move.srcY + stepY;
		for (;
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
};
typedef std::array<Piece, 64> board_t;

struct PieceTexture
{
	Team team;
	PieceType type;
	SDL_Texture* tex;
	float w, h;
};
int main()
{
	SDL_Window* wnd;
	SDL_Renderer* rend;
	int w = 1920, h = 1080;
	SDL_CreateWindowAndRenderer("Chess engine", w, h, 0, &wnd, &rend);
	SDL_SetRenderVSync(rend, 1);

	auto state = GameState::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); //starting position
	float side = std::min(w, h) / 8.0;

	std::vector<PieceTexture> pieceTextures;
	{
		std::pair<PieceType,std::string> paths[] = {
			{ PieceType::PAWN, "pawn" },
			{ PieceType::ROOK, "rook" },
			{ PieceType::KING, "king" },
			{ PieceType::QUEEN, "queen" },
			{ PieceType::BISHOP, "bishop" },
			{ PieceType::KNIGHT, "knight" },
		};
		for (auto& it : paths)
		{
			std::string pth_w = "textures/white_" + it.second + ".png";
			std::string pth_b = "textures/black_" + it.second + ".png";
			SDL_Surface* s1 = IMG_Load(pth_w.c_str());
			SDL_Surface* s2 = IMG_Load(pth_b.c_str());
			
			PieceTexture t;
			t.type = it.first;
			t.team = Team::WHITE;
			t.tex = SDL_CreateTextureFromSurface(rend,s1);
			t.w = s1->w;
			t.h = s1->h;
			pieceTextures.emplace_back(t);
			t.team = Team::BLACK;
			t.tex = SDL_CreateTextureFromSurface(rend,s2);
			t.w = s2->w;
			t.h = s2->h;
			pieceTextures.emplace_back(t);
		}
	}

	
	while (true)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{

		}

		SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
		SDL_RenderClear(rend);
		for (int y = 0; y < 8; ++y)
		{
			for (int x = 0; x < 8; ++x)
			{
				std::array<Uint8, 3> dark = { 118,150,86 };
				std::array<Uint8, 3> bright = { 238,238,210  };
				auto c = ((x ^ y) % 2) ? dark : bright;
				SDL_SetRenderDrawColor(rend, c[0], c[1], c[2], 255);

				SDL_FRect rect;
				rect.x = side * x;
				rect.y = side * y;
				rect.w = side;
				rect.h = side;
				SDL_RenderFillRect(rend, &rect);

				Piece piece = state.getPieceAt(x, y);
				if (piece.type != PieceType::EMPTY)
				{
					for (auto& it : pieceTextures)
						if (it.team == piece.team && it.type == piece.type)
						{
							SDL_FRect pieceRect;
							float scale = side * 0.9 / std::max(it.w, it.h);
							float scaledW = it.w * scale;
							float scaledH = it.h * scale;
							pieceRect.x = rect.x + (side - scaledW) / 2;
							pieceRect.y = rect.y + (side - scaledH) / 2;
							pieceRect.w = it.w * scale;
							pieceRect.h = it.h * scale;
							SDL_RenderTexture(rend, it.tex, nullptr, &pieceRect);
						}
				}
			}
		}
		SDL_RenderPresent(rend);
	}
	/*
	std::string fenStr;
	std::getline(std::cin, fenStr);
	auto s = GameState::fromFEN(fenStr);*/

	

	auto legal = state.getAllLegalMoves();
	return 0;
}