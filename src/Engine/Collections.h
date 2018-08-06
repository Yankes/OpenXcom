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
#include <SDL_stdinc.h>
#include "Exception.h"

namespace OpenXcom
{

/**
 * Helper class for managing object colections
 */
struct Collections
{
	template<typename C>
	class ForwardPosIterator
	{
		C* _colection;
		std::size_t _position;

	public:

		ForwardPosIterator() : _colection{ nullptr }, _position{ 0 }
		{

		}

		ForwardPosIterator(C* c, std::size_t pos) : _colection{ nullptr }, _position{ pos }
		{

		}

		bool operator==(const ForwardPosIterator& other)
		{
			return other._colection == _colection && other._position == _position;
		}

		typename C::reference operator*() const
		{
			return _colection->raw_data(_position);
		}

		ForwardPosIterator operator++()
		{
			*this = _colection->find(_position + 1);
			return this;
		}

		ForwardPosIterator operator++(int)
		{
			auto pos = _position;
			++(*this);
			return { _colection, pos };
		}
	};

	template<typename T>
	class SkipVector
	{
		struct SkipTable
		{
			Uint8 groupSkip;
			Uint8 itemSkip[64];
		};

		std::vector<SkipTable> _skipTable;
		std::vector<T> _items;

	public:

	};

	template<typename T>
	static void deleteAll(const T& p)
	{
		//nothing
	}
	template<typename T>
	static void deleteAll(T* p)
	{
		delete p;
	}
	template<typename T>
	static void deleteAll(std::vector<T>& vec)
	{
		for (auto p : vec)
		{
			deleteAll(p);
		}
		removeAll(vec);
	}
	template<typename T>
	static void deleteAll(std::list<T>& list)
	{
		for (auto p : list)
		{
			deleteAll(p);
		}
		removeAll(list);
	}
	template<typename K, typename V>
	static void deleteAll(std::map<K, V>& map)
	{
		for (auto p : map)
		{
			deleteAll(p.second);
		}
		removeAll(map);
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
			return first != r.first && second != r.second; //zip will stop when one of ranges ends, this is why `&&` isntade of `||`
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
	 * Crate range from `min` to `max` (excluding), if `max > min` it return empty range.
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
	 * Crate range from zero to less than `end`
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
