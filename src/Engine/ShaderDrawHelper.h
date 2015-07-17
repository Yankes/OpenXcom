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

#ifndef OPENXCOM_SHADERDRAWHELPER_H
#define	OPENXCOM_SHADERDRAWHELPER_H

#include "Surface.h"
#include "GraphSubset.h"
#include "Exception.h"
#include <vector>

namespace OpenXcom
{
namespace helper
{

/**
 * This is empty argument to `ShaderDraw`.
 * when used in `ShaderDraw` return always 0 to `ColorFunc::func` for every pixel
 */
class Nothing
{

};

/**
 * This is scalar argument to `ShaderDraw`.
 * when used in `ShaderDraw` return value of `t` to `ColorFunc::func` for every pixel
 */
template<typename T>
class Scalar
{
public:
	T& ref;
	inline Scalar(T& t) : ref(t)
	{

	}
};

/**
 * Helper class extracting constness form `Pixel` when creating `std::vector<Pixel>`
 */
template<typename Pixel>
struct CorrectConst
{
	typedef std::vector<Pixel> vector;
	typedef Surface surf;
};

/**
 * Helper class extracting constness form `Pixel` when creating `std::vector<Pixel>`
 */
template<typename Pixel>
struct CorrectConst<const Pixel>
{
	typedef const std::vector<Pixel> vector;
	typedef const Surface surf;
};

/**
 * This is surface argument to `ShaderDraw`.
 * every pixel of this surface will have type `Pixel`.
 * Modify pixels of this surface, that will modifying original data.
 */
template<typename Pixel>
class ShaderBase
{
public:
	typedef Pixel* PixelPtr;
	typedef Pixel& PixelRef;
	typedef helper::CorrectConst<Pixel> _corect_const;

protected:
	const PixelPtr _orgin;
	const GraphSubset _range_base;
	GraphSubset _range_domain;
	const int _pitch;

public:
	/**
	 * copy constructor.
     * @param s Any ShaderBase that have compatible Pixel type.
     */
	template<typename CompatiblePixel>
	inline ShaderBase(const ShaderBase<CompatiblePixel>& s) :
		_orgin(s.ptr()),
		_range_base(s.getBaseDomain()),
		_range_domain(s.getDomain()),
		_pitch(s.pitch())
	{

	}

	/**
	 * create surface using vector `f` as data source.
	 * surface will have `max_y` x `max_x` dimensions.
	 * size of `f` should be bigger or equal `max_y*max_x`.
	 * Attention: after use of this constructor you change size of `f` then `_orgin` will be invalid
	 * and use of this object will cause memory exception.
     * @param f vector that are treated as surface
     * @param max_x x dimension of `f`
     * @param max_y y dimension of `f`
     */
	inline ShaderBase(typename _corect_const::vector& f, int max_x, int max_y) :
		_orgin(&(f[0])),
		_range_base(max_x, max_y),
		_range_domain(max_x, max_y),
		_pitch(max_x*sizeof(Pixel))
	{

	}

	/**
	 * create surface using array `f` of `Pixel` as data source.
	 * surface will have `max_y` x `max_x` dimensions.
	 * size of `f` should be bigger or equal `max_y*max_x`.
     * @param f vector that are treated as surface
     * @param max_x x dimension of `f`
     * @param max_y y dimension of `f`
	 * @param pitch real length of row in bytes (`max_x*sizeof(Pixel) + padding`)
     */
	inline ShaderBase(Pixel* f, int max_x, int max_y, int pitch) :
		_orgin(f),
		_range_base(max_x, max_y),
		_range_domain(max_x, max_y),
		_pitch(pitch)
	{

	}

	/**
	 * create surface using openxcom surface `s` as data source.
	 * surface will have same dimensions as `s`.
	 * Attention: after use of this constructor you change size of surface `s`
	 * then `_orgin` will be invalid and use of this object will cause memory exception.
     * @param s openxcom surface
     */
	inline ShaderBase(typename _corect_const::surf* s) :
		_orgin((PixelPtr)s->getSurface()->pixels),
		_range_base(s->getWidth(), s->getHeight()),
		_range_domain(s->getWidth(), s->getHeight()),
		_pitch(s->getSurface()->pitch)
	{
		if(s->getSurface()->format->BytesPerPixel != sizeof(Pixel))
			throw Exception("Incompatible type of surface pixels");
	}


	inline PixelPtr ptr() const
	{
		return _orgin;
	}
	inline int pitch() const
	{
		return _pitch;
	}

	inline void setDomain(const GraphSubset& g)
	{
		_range_domain = GraphSubset::intersection(g, _range_base);
	}
	inline const GraphSubset& getDomain() const
	{
		return _range_domain;
	}
	inline const GraphSubset& getBaseDomain() const
	{
		return _range_base;
	}

	inline const GraphSubset& getImage() const
	{
		return _range_domain;
	}
};


/// helper class for handling implementation differences in different surfaces types
/// Used in function `ShaderDraw`.
template<typename SurfaceType>
struct controler
{
	/**
	 * NOT IMPLEMENTED ANYWHERE!
	 * you need create your own specification or use different type, no default version
	 */
	inline controler(const SurfaceType& s);

	/**
	 * function used only when `SurfaceType` can be used as destination surface
	 * if that type should not be used as `dest` dont implements this.
	 * @return start drawing range
	 */
	inline const GraphSubset& get_range();
	/**
	 * function used only when `SurfaceType` is used as source surface.
	 * function reduce drawing range.
	 * @param g modify drawing range
	 */
	inline void mod_range(GraphSubset& g) {}
	/**
	 * set final drawing range.
	 * @param g drawing range
	 */
	inline void set_range(const GraphSubset& g) {}

	inline void mod_y(int& begin, int& end) {}
	inline void set_y(const int& begin, const int& end) {}
	inline void inc_y() {}


	inline void mod_x(int& begin, int& end) {}
	inline void set_x(const int& begin, const int& end) {}
	inline void inc_x() {}

	inline SurfaceType& get_ref();
};

/// implementation for scalars types aka `int`, `double`, `float`
template<typename T>
struct controler<Scalar<T> >
{
	T& ref;

	inline controler(const Scalar<T>& s) : ref(s.ref)
	{

	}

	//cant use this function
	//inline GraphSubset get_range()

	inline void mod_range(GraphSubset&)
	{
		//nothing
	}
	inline void set_range(const GraphSubset&)
	{
		//nothing
	}

	inline void mod_y(int&, int&)
	{
		//nothing
	}
	inline void set_y(const int&, const int&)
	{
		//nothing
	}
	inline void inc_y()
	{
		//nothing
	}


	inline void mod_x(int&, int&)
	{
		//nothing
	}
	inline void set_x(const int&, const int&)
	{
		//nothing
	}
	inline void inc_x()
	{
		//nothing
	}

	inline T& get_ref()
	{
		return ref;
	}
};

/// implementation for not used arg
template<>
struct controler<Nothing>
{
	inline controler(const Nothing&)
	{

	}

	//cant use this function
	//inline GraphSubset get_range()

	inline void mod_range(GraphSubset&)
	{
		//nothing
	}
	inline void set_range(const GraphSubset&)
	{
		//nothing
	}

	inline void mod_y(int&, int&)
	{
		//nothing
	}
	inline void set_y(const int&, const int&)
	{
		//nothing
	}
	inline void inc_y()
	{
		//nothing
	}

	inline void mod_x(int&, int&)
	{
		//nothing
	}
	inline void set_x(const int&, const int&)
	{
		//nothing
	}
	inline void inc_x()
	{
		//nothing
	}

	/**
	 * Dont return anything
	 */
	inline void get_ref() { }
};

/**
 * Move pointer a distance in bytes
 * @param ptr basic pointer
 * @param off distance in bytes
 * @return pointer in new position
 */
template<typename PixelPtr>
static inline PixelPtr add_byte_offset(PixelPtr ptr, int off)
{
	return (PixelPtr)(((char*)ptr)+off);
}

template<typename PixelPtr, typename PixelRef>
struct controler_base
{

	const PixelPtr data;
	PixelPtr ptr_pos_y;
	PixelPtr ptr_pos_x;
	GraphSubset range;
	int start_x;
	int start_y;

	///step in bytes
	const std::pair<int, int> step;

	controler_base(PixelPtr base, const GraphSubset& d, const GraphSubset& r, const std::pair<int, int>& s) :
		data(add_byte_offset(base, d.beg_x*s.first + d.beg_y*s.second)),
		ptr_pos_y(0), ptr_pos_x(0),
		range(r),
		start_x(), start_y(),
		step(s)
	{

	}


	inline const GraphSubset& get_range()
	{
		return range;
	}

	inline void mod_range(GraphSubset& r)
	{
		r = GraphSubset::intersection(range, r);
	}

	inline void set_range(const GraphSubset& r)
	{
		start_x = r.beg_x - range.beg_x;
		start_y = r.beg_y - range.beg_y;
		range = r;
	}

	inline void mod_y(int&, int&)
	{
		ptr_pos_y = add_byte_offset(data, step.first * start_x + step.second * start_y);
	}
	inline void set_y(const int& begin, const int&)
	{
		ptr_pos_y = add_byte_offset(ptr_pos_y, step.second*begin);
	}
	inline void inc_y()
	{
		ptr_pos_y = add_byte_offset(ptr_pos_y, step.second);
	}


	inline void mod_x(int&, int&)
	{
		ptr_pos_x = ptr_pos_y;
	}
	inline void set_x(const int& begin, const int&)
	{
		ptr_pos_x = add_byte_offset(ptr_pos_x , step.first*begin);
	}
	inline void inc_x()
	{
		ptr_pos_x = add_byte_offset(ptr_pos_x, step.first);
	}

	inline PixelRef get_ref()
	{
		return *ptr_pos_x;
	}
};



template<typename Pixel>
struct controler<ShaderBase<Pixel> > : public controler_base<typename ShaderBase<Pixel>::PixelPtr, typename ShaderBase<Pixel>::PixelRef>
{
	typedef typename ShaderBase<Pixel>::PixelPtr PixelPtr;
	typedef typename ShaderBase<Pixel>::PixelRef PixelRef;

	typedef controler_base<PixelPtr, PixelRef> base_type;

	controler(const ShaderBase<Pixel>& f) : base_type(f.ptr(), f.getDomain(), f.getImage(), std::make_pair(sizeof(Pixel), f.pitch()))
	{

	}

};

}//namespace helper

}//namespace OpenXcom

#endif	/* SHADERDRAWHELPER_H */

