#pragma once
#include <array>

#include <stdint.h>
#include <string>

class Square
{
public:
	Square() = default;
	Square(int ind);
	Square(int8_t x, int8_t y);
	Square(const char* algebraic);
	Square(const std::string& algebraic);

	int8_t x, y;

	bool inBounds() const;
	bool outOfBounds() const;
	bool operator==(const Square& other) const;
	int toIndex() const;
	int toFathomIndex() const;

	std::string toAlgebraic() const;

	//returns all squares that are in-bounds of conventional 8x8 chessboard (mostly used to make iterating easier)
	static constexpr std::array<Square, 64> all_inBounds()
	{
		return []() {
			std::array<Square, 64> ret;
			for (int y = 0; y < 8; ++y)
				for (int x = 0; x < 8; ++x)
					ret[y * 8 + x] = Square(x, y);
			return ret;
			}();
	}
private:
};