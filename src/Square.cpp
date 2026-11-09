#include "Square.h"
#include <assert.h>

Square::Square(int8_t x, int8_t y)
	:x(x), y(y)
{
}

Square::Square(const char* algebraic)
{
	assert(algebraic[0] >= 'a' && algebraic[0] <= 'h');
	assert(algebraic[1] >= '1' && algebraic[1] <= '8');
	x = algebraic[0] - 'a';
	y = 7 - (algebraic[1] - '1');
}

Square::Square(const std::string& algebraic)
{
	assert(algebraic.length() == 2);
	*this = Square(algebraic.c_str());
}

bool Square::inBounds() const
{
	return x >= 0 && x < 8 && y >= 0 && y < 8;
}

bool Square::outOfBounds() const
{
	return !inBounds();
}

bool Square::operator==(const Square& other) const
{
	return x == other.x && y == other.y;
}

constexpr std::array<Square, 64> Square::all_inBounds()
{
	return []() {
		std::array<Square, 64> ret;
		for (int y = 0; y < 8; ++y)
			for (int x = 0; x < 8; ++x)
				ret[y * 8 + x] = Square(x, y);
		return ret;
		}();
}


