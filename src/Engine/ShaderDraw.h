/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#ifndef OPENXCOM_SHADERDRAW_H
#define	OPENXCOM_SHADERDRAW_H

#include "ShaderDrawHelper.h"
	
namespace OpenXcom
{

namespace helper
{

typedef controler<Nothing> ConNone;

/**
 * variadic templates for the poor
 * 8 arguments version
 */
template<typename ColorFunc,
	typename ConArg0, typename ConArg1,
	typename ConArg2, typename ConArg3,
	typename ConArg4, typename ConArg5,
	typename ConArg6, typename ConArg7>
struct ArgReducer
{
	static inline void func(
		ConArg0& arg0, ConArg1& arg1,
		ConArg2& arg2, ConArg3& arg3,
		ConArg4& arg4, ConArg5& arg5,
		ConArg6& arg6, ConArg7& arg7)
	{
		ColorFunc::func(
			arg0.get_ref(), arg1.get_ref(),
			arg2.get_ref(), arg3.get_ref(),
			arg4.get_ref(), arg5.get_ref(),
			arg6.get_ref(), arg7.get_ref());
	}
};

/**
 * variadic templates for the poor
 * 7 arguments version
 */
template<typename ColorFunc,
	typename ConArg0, typename ConArg1,
	typename ConArg2, typename ConArg3,
	typename ConArg4, typename ConArg5,
	typename ConArg6>
struct ArgReducer<ColorFunc,
	ConArg0, ConArg1,
	ConArg2, ConArg3,
	ConArg4, ConArg5,
	ConArg6, ConNone >
{
	static inline void func(
		ConArg0& arg0, ConArg1& arg1,
		ConArg2& arg2, ConArg3& arg3,
		ConArg4& arg4, ConArg5& arg5,
		ConArg6& arg6, ConNone& arg7)
	{
		ColorFunc::func(
			arg0.get_ref(), arg1.get_ref(),
			arg2.get_ref(), arg3.get_ref(),
			arg4.get_ref(), arg5.get_ref(),
			arg6.get_ref());
	}
};

/**
 * variadic templates for the poor
 * 6 arguments version
 */
template<typename ColorFunc,
	typename ConArg0, typename ConArg1,
	typename ConArg2, typename ConArg3,
	typename ConArg4, typename ConArg5>
struct ArgReducer<ColorFunc,
	ConArg0, ConArg1,
	ConArg2, ConArg3,
	ConArg4, ConArg5,
	ConNone, ConNone >
{
	static inline void func(
		ConArg0& arg0, ConArg1& arg1,
		ConArg2& arg2, ConArg3& arg3,
		ConArg4& arg4, ConArg5& arg5,
		ConNone& arg6, ConNone& arg7)
	{
		ColorFunc::func(
			arg0.get_ref(), arg1.get_ref(),
			arg2.get_ref(), arg3.get_ref(),
			arg4.get_ref(), arg5.get_ref());
	}
};

/**
 * variadic templates for the poor
 * 5 arguments version
 */
template<typename ColorFunc,
	typename ConArg0, typename ConArg1,
	typename ConArg2, typename ConArg3,
	typename ConArg4>
struct ArgReducer<ColorFunc,
	ConArg0, ConArg1,
	ConArg2, ConArg3,
	ConArg4, ConNone,
	ConNone, ConNone >
{
	static inline void func(
		ConArg0& arg0, ConArg1& arg1,
		ConArg2& arg2, ConArg3& arg3,
		ConArg4& arg4, ConNone& arg5,
		ConNone& arg6, ConNone& arg7)
	{
		ColorFunc::func(
			arg0.get_ref(), arg1.get_ref(),
			arg2.get_ref(), arg3.get_ref(),
			arg4.get_ref());
	}
};

/**
 * variadic templates for the poor
 * 5 arguments version
 */
template<typename ColorFunc,
	typename ConArg0, typename ConArg1,
	typename ConArg2, typename ConArg3>
struct ArgReducer<ColorFunc,
	ConArg0, ConArg1,
	ConArg2, ConArg3,
	ConNone, ConNone,
	ConNone, ConNone >
{
	static inline void func(
		ConArg0& arg0, ConArg1& arg1,
		ConArg2& arg2, ConArg3& arg3,
		ConNone& arg4, ConNone& arg5,
		ConNone& arg6, ConNone& arg7)
	{
		ColorFunc::func(
			arg0.get_ref(), arg1.get_ref(),
			arg2.get_ref(), arg3.get_ref());
	}
};

/**
 * variadic templates for the poor
 * 3 arguments version
 */
template<typename ColorFunc,
	typename ConArg0, typename ConArg1,
	typename ConArg2>
struct ArgReducer<ColorFunc,
	ConArg0, ConArg1,
	ConArg2, ConNone,
	ConNone, ConNone,
	ConNone, ConNone >
{
	static inline void func(
		ConArg0& arg0, ConArg1& arg1,
		ConArg2& arg2, ConNone& arg3,
		ConNone& arg4, ConNone& arg5,
		ConNone& arg6, ConNone& arg7)
	{
		ColorFunc::func(
			arg0.get_ref(), arg1.get_ref(),
			arg2.get_ref());
	}
};

/**
 * variadic templates for the poor
 * 2 arguments version
 */
template<typename ColorFunc,
	typename ConArg0, typename ConArg1>
struct ArgReducer<ColorFunc,
	ConArg0, ConArg1,
	ConNone, ConNone,
	ConNone, ConNone,
	ConNone, ConNone >
{
	static inline void func(
		ConArg0& arg0, ConArg1& arg1,
		ConNone& arg2, ConNone& arg3,
		ConNone& arg4, ConNone& arg5,
		ConNone& arg6, ConNone& arg7)
	{
		ColorFunc::func(
			arg0.get_ref(), arg1.get_ref());
	}
};

/**
 * variadic templates for the poor
 * 1 argument version
 */
template<typename ColorFunc,
	typename ConArg0>
struct ArgReducer<ColorFunc,
	ConArg0, ConNone,
	ConNone, ConNone,
	ConNone, ConNone,
	ConNone, ConNone >
{
	static inline void func(
		ConArg0& arg0, ConNone& arg1,
		ConNone& arg2, ConNone& arg3,
		ConNone& arg4, ConNone& arg5,
		ConNone& arg6, ConNone& arg7)
	{
		ColorFunc::func(
			arg0.get_ref());
	}
};

}//namespace helper

/**
 * Universal blit function.
 * This function is waiting for salvation through C++11 :>
 * @tparam ColorFunc class that contains static function `func` that get 5 arguments
 * function is used to modify these arguments.
 * @param src0_frame destination surface modified by function.
 * @param src1_frame surface or scalar
 * @param src2_frame surface or scalar
 * @param src3_frame surface or scalar
 * @param src4_frame surface or scalar
 * @param src5_frame surface or scalar
 * @param src6_frame surface or scalar
 * @param src7_frame surface or scalar
 */
template<typename ColorFunc,
	typename Src0Type, typename Src1Type,
	typename Src2Type, typename Src3Type,
	typename Src4Type, typename Src5Type,
	typename Src6Type, typename Src7Type>
static inline void ShaderDraw(
	const Src0Type& src0_frame, const Src1Type& src1_frame,
	const Src2Type& src2_frame, const Src3Type& src3_frame,
	const Src4Type& src4_frame, const Src5Type& src5_frame,
	const Src6Type& src6_frame, const Src7Type& src7_frame)
{
	//creating helper objects
	helper::controler<Src0Type> src0(src0_frame);
	helper::controler<Src1Type> src1(src1_frame);
	helper::controler<Src2Type> src2(src2_frame);
	helper::controler<Src3Type> src3(src3_frame);
	helper::controler<Src4Type> src4(src4_frame);
	helper::controler<Src5Type> src5(src5_frame);
	helper::controler<Src6Type> src6(src6_frame);
	helper::controler<Src7Type> src7(src7_frame);

	//get basic draw range in 2d space
	GraphSubset end_temp = src0.get_range();
	
	//intersections with src ranges
	src0.mod_range(end_temp);
	src1.mod_range(end_temp);
	src2.mod_range(end_temp);
	src3.mod_range(end_temp);
	src4.mod_range(end_temp);
	src5.mod_range(end_temp);
	src6.mod_range(end_temp);
	src7.mod_range(end_temp);
	
	const GraphSubset end = end_temp;
	if (end.size_x() == 0 || end.size_y() == 0)
		return;
	//set final draw range in 2d space
	src0.set_range(end);
	src1.set_range(end);
	src2.set_range(end);
	src3.set_range(end);
	src4.set_range(end);
	src5.set_range(end);
	src6.set_range(end);
	src7.set_range(end);


	int begin_y = 0, end_y = end.size_y();
	//determining iteration range in y-axis
	src0.mod_y(begin_y, end_y);
	src1.mod_y(begin_y, end_y);
	src2.mod_y(begin_y, end_y);
	src3.mod_y(begin_y, end_y);
	src4.mod_y(begin_y, end_y);
	src5.mod_y(begin_y, end_y);
	src6.mod_y(begin_y, end_y);
	src7.mod_y(begin_y, end_y);
	if(begin_y>=end_y)
		return;
	//set final iteration range
	src0.set_y(begin_y, end_y);
	src1.set_y(begin_y, end_y);
	src2.set_y(begin_y, end_y);
	src3.set_y(begin_y, end_y);
	src4.set_y(begin_y, end_y);
	src5.set_y(begin_y, end_y);
	src6.set_y(begin_y, end_y);
	src7.set_y(begin_y, end_y);

	//iteration on y-axis
	for(int y = end_y-begin_y; y>0; --y,
		src0.inc_y(), src1.inc_y(),
		src2.inc_y(), src3.inc_y(),
		src4.inc_y(), src5.inc_y(),
		src6.inc_y(), src7.inc_y())
	{
		int begin_x = 0, end_x = end.size_x();
		//determining iteration range in x-axis
		src0.mod_x(begin_x, end_x);
		src1.mod_x(begin_x, end_x);
		src2.mod_x(begin_x, end_x);
		src3.mod_x(begin_x, end_x);
		src4.mod_x(begin_x, end_x);
		src5.mod_x(begin_x, end_x);
		src6.mod_x(begin_x, end_x);
		src7.mod_x(begin_x, end_x);
		if(begin_x>=end_x)
			continue;
		//set final iteration range
		src0.set_x(begin_x, end_x);
		src1.set_x(begin_x, end_x);
		src2.set_x(begin_x, end_x);
		src3.set_x(begin_x, end_x);
		src4.set_x(begin_x, end_x);
		src5.set_x(begin_x, end_x);
		src6.set_x(begin_x, end_x);
		src7.set_x(begin_x, end_x);
		
		//iteration on x-axis
		for(int x = end_x-begin_x; x>0; --x,
			src0.inc_x(), src1.inc_x(),
			src2.inc_x(), src3.inc_x(),
			src4.inc_x(), src5.inc_x(),
			src6.inc_x(), src7.inc_x())
		{
			helper::ArgReducer<ColorFunc,
				helper::controler<Src0Type>, helper::controler<Src1Type>,
				helper::controler<Src2Type>, helper::controler<Src3Type>,
				helper::controler<Src4Type>, helper::controler<Src5Type>,
				helper::controler<Src6Type>, helper::controler<Src7Type> >
				::func(src0, src1, src2, src3, src4, src5, src6, src7);
		}
	}

};

template<typename ColorFunc,
	typename DestType, typename Src0Type,
	typename Src1Type, typename Src2Type,
	typename Src3Type, typename Src4Type,
	typename Src5Type>
static inline void ShaderDraw(
	const DestType& dest_frame, const Src0Type& src0_frame,
	const Src1Type& src1_frame, const Src2Type& src2_frame,
	const Src3Type& src3_frame, const Src4Type& src4_frame,
	const Src5Type& src5_frame)
{
	helper::Nothing none_frame;
	ShaderDraw<ColorFunc>(
		dest_frame, src0_frame,
		src1_frame, src2_frame,
		src3_frame, src4_frame,
		src5_frame, none_frame);
}

template<typename ColorFunc,
	typename DestType, typename Src0Type,
	typename Src1Type, typename Src2Type,
	typename Src3Type, typename Src4Type>
static inline void ShaderDraw(
	const DestType& dest_frame, const Src0Type& src0_frame,
	const Src1Type& src1_frame, const Src2Type& src2_frame,
	const Src3Type& src3_frame, const Src4Type& src4_frame)
{
	helper::Nothing none_frame;
	ShaderDraw<ColorFunc>(
		dest_frame, src0_frame,
		src1_frame, src2_frame,
		src3_frame, src4_frame,
		none_frame, none_frame);
}

template<typename ColorFunc,
	typename DestType, typename Src0Type,
	typename Src1Type, typename Src2Type,
	typename Src3Type>
static inline void ShaderDraw(
	const DestType& dest_frame, const Src0Type& src0_frame,
	const Src1Type& src1_frame, const Src2Type& src2_frame,
	const Src3Type& src3_frame)
{
	helper::Nothing none_frame;
	ShaderDraw<ColorFunc>(
		dest_frame, src0_frame,
		src1_frame, src2_frame,
		src3_frame, none_frame,
		none_frame, none_frame);
}

template<typename ColorFunc, typename DestType, typename Src0Type, typename Src1Type, typename Src2Type>
static inline void ShaderDraw(const DestType& dest_frame, const Src0Type& src0_frame, const Src1Type& src1_frame, const Src2Type& src2_frame)
{
	helper::Nothing none_frame;
	ShaderDraw<ColorFunc>(
		dest_frame, src0_frame,
		src1_frame, src2_frame,
		none_frame, none_frame,
		none_frame, none_frame);
}

template<typename ColorFunc, typename DestType, typename Src0Type, typename Src1Type>
static inline void ShaderDraw(const DestType& dest_frame, const Src0Type& src0_frame, const Src1Type& src1_frame)
{
	helper::Nothing none_frame;
	ShaderDraw<ColorFunc>(
		dest_frame, src0_frame,
		src1_frame, none_frame,
		none_frame, none_frame,
		none_frame, none_frame);
}

template<typename ColorFunc, typename DestType, typename Src0Type>
static inline void ShaderDraw(const DestType& dest_frame, const Src0Type& src0_frame)
{
	helper::Nothing none_frame;
	ShaderDraw<ColorFunc>(
		dest_frame, src0_frame,
		none_frame, none_frame,
		none_frame, none_frame,
		none_frame, none_frame);
}

template<typename ColorFunc, typename DestType>
static inline void ShaderDraw(const DestType& dest_frame)
{
	helper::Nothing none_frame;
	ShaderDraw<ColorFunc>(
		dest_frame, none_frame,
		none_frame, none_frame,
		none_frame, none_frame,
		none_frame, none_frame);
}

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

namespace helper
{
	
const Uint8 ColorGroup = 15<<4;
const Uint8 ColorShade = 15;
const Uint8 ColorShadeMax = 15;
const Uint8 BLACK = 15;

}//namespace helper

}//namespace OpenXcom


#endif	/* OPENXCOM_SHADERDRAW_H */

