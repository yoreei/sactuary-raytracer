#pragma once
#include <vector>
#include <stdexcept>

class IndexMap {
public:
	IndexMap(size_t size)
	{
		map.resize(size, false);
	}
	void add(size_t index, std::vector<size_t>& output)
	{
		if (map.size() <= index) {
			throw std::out_of_range("out of range");
		}

		if (!map[index]) {
			output.push_back(index);
			map[index] = true;
		}
	}

	std::vector<bool> map{};
};

class IndexCounter {
public:
	IndexCounter(size_t size)
	{
		map.resize(size, -1);
	}
	int add(size_t index)
	{
		if (map.size() <= index) {
			throw std::out_of_range("out of range");
		}

		if (map[index] != -1) {
			return map[index];
		}
		else {
			map[index] = counter;
			++counter;
			return -1;
		}
	}
	int counter = 0;
	std::vector<int> map{};
};
