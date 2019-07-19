#pragma once
/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <vector>
#include <map>
#include <list>
#include "Exception.h"

namespace OpenXcom
{

/**
 * Helper class for managing object colections
 */
class Collections
{
public:
	/**
	 * Delete can be only used on owning pointers, to make clear diffrence to removeAll we reject case when it is used on colection without pointers.
	 * @param p
	 */
	template<typename T>
	static void deleteAll(const T& p)
	{
		static_assert(sizeof(T) == 0, "deleteAll can be only used on pointers and colection of pointers");
	}
	template<typename T>
	static void deleteAll(T* p)
	{
		delete p;
	}
	template<typename K, typename V>
	static void deleteAll(std::pair<const K, V>& p)
	{
		deleteAll(p.second);
	}

	/**
	 * Delete all pointers from container, it can be nested.
	 * SFINAE, valid only if type look like container.
	 */
	template<typename C, typename = decltype(std::declval<C>().begin()), typename = decltype(std::declval<C>().end())>
	static void deleteAll(C& colection)
	{
		for (auto p : colection)
		{
			deleteAll(p);
		}
		removeAll(colection);
	}

	/**
	 * Remove and delete (if pointer) items from colection with limit.
	 * @param colection Ccolection from witch remove items
	 * @param numberToRemove Limit of removal
	 * @param func Test what should be removed, can modyfy everyting except this colection
	 * @return Number of values left to remove
	 */
	template<typename C, typename F>
	static int deleteIf(C& colection, int numberToRemove, F&& func)
	{
		return removeIf(colection, numberToRemove,
			[&](typename C::reference t)
			{
				if (func(t))
				{
					deleteAll(t);
					return true;
				}
				else
				{
					return false;
				}
			}
		);
	}

	/**
	 * Remove and delete (if pointer) items from colection.
	 * @param colection Ccolection from witch remove items
	 * @param func Test what should be removed, can modyfy everyting except this colection
	 * @return Number of values left in colection
	 */
	template<typename C, typename F>
	static int deleteIf(C& colection, F&& func)
	{
		return deleteIf(colection, colection.size(), std::forward<F>(func));
	}

	/**
	 * Clear vector. It set capacity to zero too.
	 * @param vec
	 */
	template<typename T>
	static void removeAll(std::vector<T>& vec)
	{
		std::vector<T>{}.swap(vec);
	}

	/**
	 * Clear container.
	 * @param colection
	 */
	template<typename C>
	static void removeAll(C& colection)
	{
		colection.clear();
	}

	/**
	 * Remove items from vector with limit.
	 * @param vec Vector from witch remove items
	 * @param numberToRemove Limit of removal
	 * @param func Test what should be removed, can modyfy everyting except this vector
	 * @return Number of values left to remove
	 */
	template<typename T, typename F>
	static int removeIf(std::vector<T>& vec, int numberToRemove, F&& func)
	{
		if (numberToRemove <= 0)
		{
			return 0;
		}
		auto begin = vec.begin();
		auto newEnd = vec.begin();
		//similar to `std::remove_if` but it do not allow modyfy anything in `func`
		for (auto it = begin; it != vec.end(); ++it)
		{
			auto& value = *it;
			if (numberToRemove > 0 && func(value))
			{
				--numberToRemove;
			}
			else
			{
				*newEnd = std::move(value);
				++newEnd;
			}
		}
		vec.erase(newEnd, vec.end());
		return numberToRemove;
	}

	/**
	 * Remove items from colection with limit.
	 * @param list List from witch remove items
	 * @param numberToRemove Limit of removal
	 * @param func Test what should be removed, can modyfy everyting except this colection
	 * @return Number of values left to remove
	 */
	template<typename C, typename F>
	static int removeIf(C& colection, int numberToRemove, F&& func)
	{
		if (numberToRemove <= 0)
		{
			return 0;
		}
		for (auto it = colection.begin(); it != colection.end(); )
		{
			auto& value = *it;
			if (func(value))
			{
				it = colection.erase(it);
				if (--numberToRemove == 0)
				{
					return 0;
				}
			}
			else
			{
				++it;
			}
		}
		return numberToRemove;
	}

	/**
	 * Remove items from colection.
	 * @param list List from witch remove items
	 * @param func Test what should be removed, can modyfy everyting except this colection
	 * @return Number of values left in colection
	 */
	template<typename C, typename F>
	static int removeIf(C& colection, F&& func)
	{
		return removeIf(colection, colection.size(), std::forward<F>(func));
	}

	template<typename C, typename Predicate, typename Callback>
	static void untilLastIf(C& colection, Predicate&& p, Callback&& f)
	{
		int countLimit = 0;
		int curr = 0;
		for (auto &v : colection)
		{
			++curr;
			if (p(v)) countLimit = curr;
		}
		for (auto &v : colection)
		{
			if (countLimit)
			{
				f(v);
				--countLimit;
			}
			else
			{
				break;
			}
		}
	}
};

}
