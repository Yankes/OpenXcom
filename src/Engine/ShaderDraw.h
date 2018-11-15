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
#include "ShaderDrawHelper.h"
#include "HelperMeta.h"
#include <tuple>

namespace OpenXcom
{

/**
 * Universal blit function implementation.
 * @param f called function.
 * @param src source surfaces control objects.
 */
template<typename Func, typename... SrcType>
static inline void ShaderDrawImpl(Func&& f, helper::controler<SrcType>... src)
{
	//get basic draw range in 2d space
	GraphSubset end_temp = std::get<0>(std::tie(src...)).get_range();

	//intersections with src ranges
	(void)helper::DummySeq
	{
		(src.mod_range(end_temp), 0)...
	};

	const GraphSubset end = end_temp;
	if (end.size_x() == 0 || end.size_y() == 0)
		return;
	//set final draw range in 2d space
	(void)helper::DummySeq
	{
		(src.set_range(end), 0)...
	};


	int begin_y = 0, end_y = end.size_y();
	//determining iteration range in y-axis
	(void)helper::DummySeq
	{
		(src.mod_y(begin_y, end_y), 0)...
	};
	if(begin_y>=end_y)
		return;
	//set final iteration range
	(void)helper::DummySeq
	{
		(src.set_y(begin_y, end_y), 0)...
	};

	//iteration on y-axis
	for (int y = end_y-begin_y; y>0; --y, (void)helper::DummySeq{ (src.inc_y(), 0)... })
	{
		int begin_x = 0, end_x = end.size_x();
		//determining iteration range in x-axis
		(void)helper::DummySeq
		{
			(src.mod_x(begin_x, end_x), 0)...
		};
		if (begin_x>=end_x)
			continue;
		//set final iteration range
		(void)helper::DummySeq
		{
			(src.set_x(begin_x, end_x), 0)...
		};

		int size_x = end_x-begin_x;
		//iteration on x-axis
		for (int x = size_x / 4; x>0; --x)
		{
			f(src.get_ref()...); (void)helper::DummySeq{ (src.inc_x(), 0)... };
			f(src.get_ref()...); (void)helper::DummySeq{ (src.inc_x(), 0)... };
			f(src.get_ref()...); (void)helper::DummySeq{ (src.inc_x(), 0)... };
			f(src.get_ref()...); (void)helper::DummySeq{ (src.inc_x(), 0)... };
		}
		if (size_x & 2)
		{
			f(src.get_ref()...); (void)helper::DummySeq{ (src.inc_x(), 0)... };
			f(src.get_ref()...); (void)helper::DummySeq{ (src.inc_x(), 0)... };
		}
		if (size_x & 1)
		{
			f(src.get_ref()...); (void)helper::DummySeq{ (src.inc_x(), 0)... };
		}
	}

};

/**
 * Universal blit function.
 * @tparam ColorFunc class that contains static function `func`.
 * function is used to modify these arguments.
 * @param src_frame destination and source surfaces modified by function.
 */
template<typename ColorFunc, typename... SrcType>
static inline void ShaderDraw(const SrcType&... src_frame)
{
	ShaderDrawImpl(ColorFunc::func, helper::controler<SrcType>(src_frame)...);
}

/**
 * Universal blit function.
 * @param f function that modify other arguments.
 * @param src_frame destination and source surfaces modified by function.
 */
template<typename Func, typename... SrcType>
static inline void ShaderDrawFunc(Func&& f, const SrcType&... src_frame)
{
	ShaderDrawImpl(std::forward<Func>(f), helper::controler<SrcType>(src_frame)...);
}

namespace helper
{

const Uint8 ColorGroup = 0xF0;
const Uint8 ColorShade = 0x0F;

/**
 * help class used for Surface::blitNShade
 */
struct ColorReplace
{
	/**
	* Function used by ShaderDraw in Surface::blitNShade
	* set shade and replace color in that surface
	* @param dest destination pixel
	* @param src source pixel
	* @param shade value of shade of this surface
	* @param newColor new color to set (it should be offseted by 4)
	*/
	static inline void func(Uint8& dest, const Uint8& src, const int& shade, const int& newColor)
	{
		if (src)
		{
			const Uint8 newShade = (src & ColorShade) + shade;
			if (newShade & ColorGroup)
				// so dark it would flip over to another color - make it black instead
				dest = ColorShade;
			else
				dest = newColor | newShade;
		}
	}

};

/**
 * help class used for Surface::blitNShade
 */
struct StandardShade
{
	/**
	* Function used by ShaderDraw in Surface::blitNShade
	* set shade
	* @param dest destination pixel
	* @param src source pixel
	* @param shade value of shade of this surface
	* @param notused
	* @param notused
	*/
	static inline void func(Uint8& dest, const Uint8& src, const int& shade)
	{
		if (src)
		{
			const Uint8 newShade = src + shade;
			if ((newShade ^ src) & ColorGroup)
				// so dark it would flip over to another color - make it black instead
				dest = ColorShade;
			else
				dest = newShade;
		}
	}

};
/**
 * helper class used for bliting dying unit with overkill
 */
struct BurnShade
{
	static inline void func(Uint8& dest, const Uint8& src, const int& burn, const int& shade)
	{
		if (src)
		{
			if (burn)
			{
				const Uint8 tempBurn = (src & ColorShade) + burn;
				if (tempBurn > 26)
				{
					//nothing
				}
				else if (tempBurn > 15)
				{
					StandardShade::func(dest, ColorShade, shade);
				}
				else
				{
					StandardShade::func(dest, (src & ColorGroup) + tempBurn, shade);
				}
			}
			else
			{
				StandardShade::func(dest, src, shade);
			}
		}
	}
};

}//namespace helper

template<typename T>
static inline helper::Scalar<T> ShaderScalar(T& t)
{
	return helper::Scalar<T>(t);
}
template<typename T>
static inline helper::Scalar<const T> ShaderScalar(const T& t)
{
	return helper::Scalar<const T>(t);
}

/**
 * Create warper from vector
 * @param s vector
 * @return
 */
template<typename Pixel>
inline helper::ShaderBase<Pixel> ShaderSurface(std::vector<Pixel>& s, int max_x, int max_y)
{
	return helper::ShaderBase<Pixel>(s, max_x, max_y);
}

/**
 * Create warper from array
 * @param s array
 * @return
 */
template<typename Pixel, int Size>
inline helper::ShaderBase<Pixel> ShaderSurface(Pixel(&s)[Size], int max_x, int max_y)
{
	return helper::ShaderBase<Pixel>(s, max_x, max_y, max_x*sizeof(Pixel));
}

}//namespace OpenXcom
