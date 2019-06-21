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
#include <algorithm>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <random>

namespace OpenXcom
{

/**
 * Random Number Generator used throughout the game
 * for all your randomness needs. Uses a 64-bit xorshift
 * pseudorandom number generator.
 */
namespace RNG
{
	class RandomState
	{
		 uint64_t _seedState;

	public:
		/// Default constructor intializy seed by time and this type address.
		RandomState();
		/// Constructor from predefined seed.
		RandomState(uint64_t seed);
		/// Get current seed.
		uint64_t getSeed() const;

		/// Get next random number.
		uint64_t next();
		/// Generates a random integer number, inclusive.
		int generate(int min, int max);

		/// Needed by shuffle
		using result_type = uint64_t;
		/// Needed by shuffle
		constexpr static uint64_t max() { return ~uint64_t{}; }
		/// Needed by shuffle
		constexpr static uint64_t min() { return uint64_t{}; }
		/// Needed by shuffle
		uint64_t operator()() { return next(); }
	};

	/// Gets the seed in use.
	uint64_t getSeed();
	/// Sets the seed in use.
	void setSeed(uint64_t n);
	/// Get state.
	RandomState& globalRandomState();
	/// Generates a random integer number, inclusive.
	int generate(int min, int max);
	/// Generates a random floating-point number.
	double generate(double min, double max);
	/// Generates a random integer number, inclusive (non-seed version).
	int seedless(int min, int max);
	/// Get normally distributed value.
	double boxMuller(double m = 0, double s = 1);
	/// Generates a percentage chance.
	bool percent(int value);
	/// Shuffles a list randomly.
	/**
	 * Randomly changes the orders of the elements in a list.
	 * @param list The container to randomize.
	 */
	template <typename T>
	void shuffle(T &list)
	{
		std::shuffle(list.begin(), list.end(), globalRandomState());
	}
}

}
