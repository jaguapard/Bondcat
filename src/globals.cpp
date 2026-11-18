#include "globals.h"

/*
const std::vector<Move>& Globals::getPieceMoveCandidates(Square from, PieceType type)
{
    typedef std::vector<Move> movelist_t;

	constexpr int N = int(PieceType::EMPTY) + 1;
	typedef std::array<movelist_t, 64 * N> resulting_list_t;
	static const resulting_list_t lut = []()
        {
			resulting_list_t results;

			for (Square from : Square::all_inBounds())
			{
				for (Square to : Square::all_inBounds())
				{
					int dx = to.x - from.x;
					int dy = to.y - from.y;
					int i = from.toIndex();
					uint64_t dstBit = 1ull << (to.toIndex());

					if (from == to) continue;
					int j = i * (int(PieceType::EMPTY) + 1);
					Move move{ from,to,MoveType::MOVE_TO_SQUARE };
					//movelist_t& appendTo = ret[int()]
					if ((std::abs(dx) == 1 && std::abs(dy) == 2) || (std::abs(dx) == 2 && std::abs(dy) == 1)) results[j + int(PieceType::KNIGHT)].push_back(move);
					if (abs(dx) == abs(dy))
					{
						results[j + int(PieceType::BISHOP)].push_back(move);
						results[j + int(PieceType::QUEEN)].push_back(move);
					}
					if (dx == 0 || dy == 0)
					{
						results[j + int(PieceType::QUEEN)].push_back(move);
						results[j + int(PieceType::ROOK)].push_back(move);
					}
					if (abs(dx) <= 1 && abs(dy) <= 1) results[j + int(PieceType::KING)].push_back(move);

					if (dx == 0 && abs(dy) == 1) results[j + int(PieceType::PAWN)].push_back(move);
					if (dx == 0 && abs(dy) == 2 && (from.y == 1 || from.y == 6)) results[j + int(PieceType::PAWN)].push_back(move);
					if (abs(dx) == 1 && abs(dy) == 1) results[j + int(PieceType::PAWN)].push_back(move);
				}
			}

            return results;
        }();
    
	return lut[from.toIndex() * N + int(type)];
}*/

void Globals::init()
{
	for (auto& it : Globals::kingMoves) it = 0;
	for (auto& it : Globals::queenMoves) it = 0;
	for (auto& it : Globals::rookMoves) it = 0;
	for (auto& it : Globals::bishopMoves) it = 0;
	for (auto& it : Globals::knightMoves) it = 0;
	for (auto& it : Globals::blackPawnMoves) it = 0;
	for (auto& it : Globals::whitePawnMoves) it = 0;

	for (Square from : Square::all_inBounds())
	{
		for (Square to : Square::all_inBounds())
		{
			int dx = to.x - from.x;
			int dy = to.y - from.y;
			int i = from.toIndex();
			uint64_t dstBit = 1ull << (to.toIndex());

			if (from == to) continue;
			if ((std::abs(dx) == 1 && std::abs(dy) == 2) || (std::abs(dx) == 2 && std::abs(dy) == 1)) knightMoves[i] |= dstBit;
			if (abs(dx) == abs(dy)) { queenMoves[i] |= dstBit; bishopMoves[i] |= dstBit; }
			if (dx == 0 || dy == 0) { queenMoves[i] |= dstBit; rookMoves[i] |= dstBit; }
			if (abs(dx) <= 1 && abs(dy) <= 1) kingMoves[i] |= dstBit;

			if (dy == 1 && dx == 0) blackPawnMoves[i] |= dstBit;
			if ((dy == 1 || dy == 2) && dx == 0 && from.y == 1) blackPawnMoves[i] |= dstBit;
			if (dy == -1 && dx == 0) whitePawnMoves[i] |= dstBit;
			if ((dy == -1 || dy == -2) && dx == 0 && from.y == 6) whitePawnMoves[i] |= dstBit;

			if (dy == 1 && abs(dx) == 1) blackPawnCaptures[i] |= dstBit;
			if (dy == -1 && abs(dx) == 1) whitePawnCaptures[i] |= dstBit;
		}
	}

	for (Square from : Square::all_inBounds())
	{
		auto slider_run = [&](int sx, int sy, uint64_t& writeBack) {
			int x = from.x + sx; int y = from.y + sy;
			writeBack = 0;
			while (Square(x, y).inBounds())
			{
				writeBack |= 1ull << Square(x, y).toIndex();
				x += sx; y += sy;
			}
			};

		int k = from.toIndex();
		using enum Direction;
		slider_run(-1, -1, directionRaysByOrigin[k][int(LEFT_UP)]);
		slider_run(1, -1, directionRaysByOrigin[k][int(RIGHT_UP)]);
		slider_run(-1, 1, directionRaysByOrigin[k][int(LEFT_DOWN)]);
		slider_run(1, 1, directionRaysByOrigin[k][int(RIGHT_DOWN)]);

		slider_run(-1, 0, directionRaysByOrigin[k][int(LEFT)]);
		slider_run(1, 0, directionRaysByOrigin[k][int(RIGHT)]);
		slider_run(0, -1, directionRaysByOrigin[k][int(UP)]);
		slider_run(0, 1, directionRaysByOrigin[k][int(DOWN)]);
	}
}
