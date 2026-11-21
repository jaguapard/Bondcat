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

	void pop_back()
	{
		assert(count > 0);
		--count;
	}

	//Returns an index of first entrance of value in the array or nullopt if the value is not in the array
	std::optional<uint32_t> find(const T& value)
	{
		auto f = std::find(arr.begin(), arr.end(), value);
		if (f != arr.end()) return f - arr.begin();
		return std::nullopt;
	}

	T& back() { return arr[count - 1]; }
	const T& back() const { return arr[count - 1]; }


	auto begin() { return arr.begin(); }
	auto end() { return arr.begin() + count; }

	size_t size() { return count; }
	size_t capacity() { return N; }
};