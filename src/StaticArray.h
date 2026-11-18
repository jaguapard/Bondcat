#pragma once
#include <array>

template <typename T, size_t N>
struct StaticArray
{
private:
	size_t count = 0;
public:
	std::array<T, N> arr;

	void push_back(const T& value)
	{
		//if (count >= N) throw std::out_of_range("StaticArray overflow");
		arr[count++] = value;
	}

	T& operator[](size_t i) { return arr[i]; }
	const T& operator[](size_t i) const { return arr[i]; }

	auto begin() { return arr.begin(); }
	auto end() { return arr.begin() + count; }

	size_t size() { return count; }
	size_t capacity() { return N; }
};