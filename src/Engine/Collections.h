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
#include <algorithm>
#include "Exception.h"

namespace OpenXcom
{

/**
 * Helper class for managing object collections
 */
class Collections
{
public:

	////////////////////////////////////////////////////////////
	//					Helper algorithms
	////////////////////////////////////////////////////////////

	/**
	 * Delete can be only used on owning pointers, to make clear difference to removeAll we reject case when it is used on collection without pointers.
	 * @param p
	 */
	template<typename T>
	static void deleteAll(const T& p)
	{
		static_assert(sizeof(T) == 0, "deleteAll can be only used on pointers and collection of pointers");
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
	 * Remove and delete (if pointer) items from collection with limit.
	 * @param collection Collection from which to remove items
	 * @param numberToRemove Limit of removal
	 * @param func Test what should be removed, can modify everything except this collection
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
	 * Remove and delete (if pointer) items from collection.
	 * @param collection Collection from which to remove items
	 * @param func Test what should be removed, can modify everything except this collection
	 * @return Number of values left in collection
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
	 * @param collection
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
	 * @param func Test what should be removed, can modify everything except this vector
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
		//similar to `std::remove_if` but it do not allow modify anything in `func`
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
	 * Remove items from collection with limit.
	 * @param list List from witch remove items
	 * @param numberToRemove Limit of removal
	 * @param func Test what should be removed, can modify everything except this collection
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
	 * Remove items from collection.
	 * @param list List from witch remove items
	 * @param func Test what should be removed, can modify everything except this collection
	 * @return Number of values left in collection
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

	/**
	 * Sort vector using `std::less`.
	 */
	template<typename T>
	static void sortVector(std::vector<T>& vec)
	{
		std::sort(vec.begin(), vec.end(), std::less<>());
	}

	/**
	 * Check for value in sorted vector by `std::less`.
	 */
	template<typename T>
	static bool sortVectorHave(const std::vector<T>& vec, T v)
	{
		return std::binary_search(vec.begin(), vec.end(), v, std::less<>());
	}

	////////////////////////////////////////////////////////////
	//						Range
	////////////////////////////////////////////////////////////

	template<typename T>
	struct ValueIterator
	{
		T value;

		T operator*()
		{
			return value;
		}

		void operator++()
		{
			++value;
		}

		void operator--()
		{
			--value;
		}

		bool operator!=(const ValueIterator& r)
		{
			return value != r.value;
		}
	};

	template<typename ItA, typename ItB>
	struct ZipIterator
	{
		ItA first;
		ItB second;

		auto operator*() -> decltype(std::make_pair(*first, *second))
		{
			return std::make_pair(*first, *second);
		}

		void operator++()
		{
			++first;
			++second;
		}

		bool operator!=(const ZipIterator& r)
		{
			return first != r.first && second != r.second; //zip will stop when one of ranges ends, this is why `&&` instead of `||`
		}
	};

	template<typename It, typename Filter>
	class FilterIterator
	{
		It _curr;
		It _end;
		Filter _filter;

	public:

		FilterIterator(It curr, It end, Filter filter) :
			_curr { std::move(curr)},
			_end { std::move(end)},
			_filter { std::move(filter)}
		{

		}

		auto operator*() -> decltype(*_curr)
		{
			return *_curr;
		}

		void operator++()
		{
			++_curr;
			while (_curr != _end && !_filter(*_curr))
			{
				++_curr;
			}
		}

		bool operator!=(const FilterIterator& r)
		{
			return _curr != r._curr;
		}
	};

	template<typename It>
	struct ReverseIterator
	{
		It _curr;

	public:

		ReverseIterator(It curr) :
			_curr{ std::move(curr) }
		{

		}

		auto operator*() -> decltype(*_curr)
		{
			auto copy = _curr;
			--copy;
			return *copy;
		}

		void operator++()
		{
			--_curr;
		}

		bool operator!=(const ReverseIterator& r)
		{
			return _curr != r._curr;
		}
	};

	template<typename It>
	class Range
	{
		It _begin;
		It _end;
	public:

		Range(It begin, It end) :
			_begin { std::move(begin) },
			_end { std::move(end) }
		{
		}

		It begin()
		{
			return _begin;
		}
		It end()
		{
			return _end;
		}
	};

	/**
	 * Create range from `min` to `max` (excluding), if `max > min` it return empty range.
	 * @param min minimum value of range
	 * @param max maximum value of range
	 * @return
	 */
	template<typename T>
	static Range<ValueIterator<T>> rangeValue(T begin, T end)
	{
		if (begin < end)
		{
			return { ValueIterator<T>{ begin }, ValueIterator<T>{ end } };
		}
		else
		{
			return { ValueIterator<T>{ end }, ValueIterator<T>{ end } };
		}
	}

	/**
	 * Create range from zero to less than `end`
	 * @param end
	 * @return
	 */
	template<typename T>
	static Range<ValueIterator<T>> rangeValueLess(T end)
	{
		return rangeValue(T{}, end);
	}

	template<typename It>
	static Range<It> range(It begin, It end)
	{
		return { begin, end };
	}

	template<typename T>
	static Range<T*> range(std::vector<T>& v)
	{
		return { v.data(), v.data() + v.size() };
	}

	template<typename T>
	static Range<const T*> range(const std::vector<T>& v)
	{
		return { v.data(), v.data() + v.size() };
	}

	template<typename T, int N>
	static Range<T*> range(T (&a)[N])
	{
		return { std::begin(a), std::end(a) };
	}

	template<typename ItA, typename ItB>
	static Range<ZipIterator<ItA, ItB>> zip(Range<ItA> a, Range<ItB> b)
	{
		return { { a.begin(), b.begin() }, { a.end(), b.end() } };
	}

	template<typename It>
	static Range<ReverseIterator<It>> reverse(Range<It> a)
	{
		return { a.end(), a.begin() };
	}

	template<typename It>
	static Range<It> reverse(Range<ReverseIterator<It>> a)
	{
		return { a.end().curr, a.begin().curr };
	}

	template<typename It, typename Filter>
	static Range<FilterIterator<It, Filter>> filter(Range<It> a, Filter f)
	{
		auto begin = a.begin();
		auto end = a.end();
		if (begin != end)
		{
			auto fbegin = FilterIterator<It, Filter>{ begin, end, f };
			if (!f(*begin))
			{
				++fbegin;
			}
			return { fbegin, { end, end, f } };
		}
		else
		{
			return { { end, end, f }, { end, end, f } };
		}
	}

	template<typename It>
	static Range<ValueIterator<It>> nonDeref(Range<It> a)
	{
		return { { a.begin() }, { a.end() } };
	}
};

}
