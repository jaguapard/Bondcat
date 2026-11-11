#pragma once
#include <mutex>

struct Globals
{
	static int tablebase_max_pieces;
	static std::mutex tablebase_mutex;
};