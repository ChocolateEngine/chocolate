#pragma once

#include <vector>
#include <string>
#include <algorithm>


template <class T>
size_t vec_index(std::vector<T> &vec, T item)
{
	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec[i] == item)
			return i;
	}

	return SIZE_MAX;
}


template <class T>
size_t vec_index(const std::vector<T> &vec, T item)
{
	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec[i] == item)
			return i;
	}

	return SIZE_MAX;
}


template <class T>
void vec_remove(std::vector<T> &vec, T item)
{
	vec.erase(vec.begin() + vec_index(vec, item));
}


template <class T>
bool vec_contains(std::vector<T> &vec, T item)
{
	for (T addedItem: vec)
	{
		if (addedItem == item)
			return true;
	}

	return false;
}


template <class T>
bool vec_contains(const std::vector<T> &vec, T item)
{
	for (T addedItem: vec)
	{
		if (addedItem == item)
			return true;
	}

	return false;
}


void str_upper(std::string &string);
void str_lower(std::string &string);

