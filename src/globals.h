#pragma once
#include <mutex>

struct Globals
{
	static inline std::mutex tablebase_mutex;
};