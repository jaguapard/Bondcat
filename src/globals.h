#pragma once
#include <mutex>

struct Globals
{
	static std::mutex tablebase_mutex;
};