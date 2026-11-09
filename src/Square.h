#pragma once
#include <array>

#include <stdint.h>
#include <string>

class Square
{
public:
	Square() = default;
	Square(int8_t x, int8_t y);
	Square(const char* algebraic);
	Square(const std::string& algebraic);

	int8_t x, y;

	bool inBounds() const;
	bool outOfBounds() const;
	bool operator==(const Square& other) const;

	std::string toAlgebraic() const;

	//returns all squares that are in-bounds of conventional 8x8 chessboard (mostly used to make iterating easier)
	static constexpr std::array<Square, 64> all_inBounds();
private:
};