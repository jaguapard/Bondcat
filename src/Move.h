#pragma once
#include "enums.h"

struct SimpleMove
{
	int srcX, srcY, dstX, dstY;
};

struct PromotionMove : SimpleMove
{
	PieceType promotionTo;
};