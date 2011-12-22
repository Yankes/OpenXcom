/*
 * Copyright 2011 OpenXcom Developers.
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

namespace OpenXcom
{
namespace helper
{
	
	
class Nothing
{
	
};

template<typename T>
class Scalar
{
public:
	T& ref;
	inline Scalar(T& t) : ref(t)
	{
		
	}
};


template<typename Pixel>
class ShaderBase
{
public:
	typedef Pixel* PixelPtr;
	typedef Pixel& PixelRef;
	
protected:
	const PixelPtr _orgin;
	const GraphSubset _range_base;
	GraphSubset _range_domain;
	const int _pitch;
	
public:
	inline ShaderBase(const ShaderBase& s):
		_orgin(s.ptr()),
		_range_base(s._range_base),
		_range_domain(s.getDomain()),
		_pitch(s.pitch())		
	{
			
	}
		
	inline ShaderBase(std::vector<Pixel>& f, int max_x, int max_y):
		_orgin(&(f[0])),
		_range_base(max_x, max_y),
		_range_domain(max_x, max_y),
		_pitch(max_x)	
	{
		
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

template<typename Pixel>
class ShaderBase<const Pixel>
{
public:
	typedef const Pixel* PixelPtr;
	typedef const Pixel& PixelRef;
	
protected:
	const PixelPtr _orgin;
	const GraphSubset _range_base;
	GraphSubset _range_domain;
	const int _pitch;
	
public:
	inline ShaderBase(const ShaderBase& s):
		_orgin(s.ptr()),
		_range_base(s.getBaseDomain()),
		_range_domain(s.getDomain()),
		_pitch(s.pitch())		
	{
			
	}
		
	inline ShaderBase(const ShaderBase<Pixel>& s):
		_orgin(s.ptr()),
		_range_base(s.getBaseDomain()),
		_range_domain(s.getDomain()),
		_pitch(s.pitch())		
	{
			
	}
		
	inline ShaderBase(const std::vector<Pixel>& f, int max_x, int max_y):
		_orgin(&(f[0])),
		_range_base(max_x, max_y),
		_range_domain(max_x, max_y),
		_pitch(max_x)	
	{
		
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

template<>
class ShaderBase<Uint8>
{
public:
	typedef Uint8* PixelPtr;
	typedef Uint8& PixelRef;
	
protected:
	const PixelPtr _orgin;
	const GraphSubset _range_base;
	GraphSubset _range_domain;
	const int _pitch;
	
public:
	inline ShaderBase(const ShaderBase& s):
		_orgin(s.ptr()),
		_range_base(s.getBaseDomain()),
		_range_domain(s.getDomain()),
		_pitch(s.pitch())		
	{
			
	}
		
	inline ShaderBase(Surface* s):
		_orgin((Uint8*) s->getSurface()->pixels),
		_range_base(s->getWidth(), s->getHeight()),
		_range_domain(s->getWidth(), s->getHeight()),
		_pitch(s->getSurface()->pitch)		
	{
			
	}
		
	inline ShaderBase(std::vector<Uint8>& f, int max_x, int max_y):
		_orgin(&(f[0])),
		_range_base(max_x, max_y),
		_range_domain(max_x, max_y),
		_pitch(max_x)	
	{
		
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

template<>
class ShaderBase<const Uint8>
{
public:
	typedef const Uint8* PixelPtr;
	typedef const Uint8& PixelRef;
	
protected:
	const PixelPtr _orgin;
	const GraphSubset _range_base;
	GraphSubset _range_domain;
	const int _pitch;
	
public:
	inline ShaderBase(const ShaderBase& s):
		_orgin(s.ptr()),
		_range_base(s.getBaseDomain()),
		_range_domain(s.getDomain()),
		_pitch(s.pitch())		
	{
			
	}
		
	inline ShaderBase(const ShaderBase<Uint8>& s):
		_orgin(s.ptr()),
		_range_base(s.getBaseDomain()),
		_range_domain(s.getDomain()),
		_pitch(s.pitch())		
	{
			
	}
		
	inline ShaderBase(const Surface* s):
		_orgin((Uint8*) s->getSurface()->pixels),
		_range_base(s->getWidth(), s->getHeight()),
		_range_domain(s->getWidth(), s->getHeight()),
		_pitch(s->getSurface()->pitch)		
	{
			
	}
	
	inline ShaderBase(const std::vector<Uint8>& f, int max_x, int max_y):
		_orgin(&(f[0])),
		_range_base(max_x, max_y),
		_range_domain(max_x, max_y),
		_pitch(max_x)	
	{
		
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


/// helper class for handlig implementation differece in different surfacestypes
template<typename SurfaceType>
struct controler
{
	//NOT IMPLEMENTED ANYWHERE!
	//you need create your own specification or use diffrent type, no default version

	/**
	 * function used only when `SurfaceType` can be used as destionation surface
	 * if that type should not be used as `dest` dont implements this.
	 * @return start drawing range 
	 */
	inline const GraphSubset& get_range();
	/**
	 * function used only when `SurfaceType` is used as source surface.
	 * function reduce drawing range.
	 * @param g modyfed drawing range 
	 */
	inline void mod_range(GraphSubset& g);
	/**
	 * set final drawing range.
	 * @param g drawing range 
	 */
	inline void set_range(const GraphSubset& g);

	inline void mod_y(int& begin, int& end);
	inline void set_y(const int& begin, const int& end);
	inline void inc_y();


	inline void mod_x(int& begin, int& end);
	inline void set_x(const int& begin, const int& end);
	inline void inc_x();

	inline int& get_ref();
};

/// implementation for scalar types aka int, double, float
template<typename T>
struct controler<Scalar<T> >
{
	T& ref;
	
	inline controler(const Scalar<T>& s) : ref(s.ref)
	{
		
	}
	
	//cant use
	//inline GraphSubset get_range()
	
	inline void mod_range(GraphSubset&)
	{
		//nothing
	}
	inline void set_range(const GraphSubset&)
	{
		//nothing
	}
	
	inline void mod_y(int& begin, int& end)
	{
		//nothing
	}
	inline void set_y(const int& begin, const int& end)
	{
		//nothing
	}
	inline void inc_y()
	{
		//nothing
	}
	
	
	inline void mod_x(int& begin, int& end)
	{
		//nothing
	}
	inline void set_x(const int& begin, const int& end)
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
	const int i;
	inline controler(const Nothing& s) : i(0)
	{
		
	}
	
	//cant use
	//inline GraphSubset get_range()
	
	inline void mod_range(GraphSubset&)
	{
		//nothing
	}
	inline void set_range(const GraphSubset&)
	{
		//nothing
	}
	
	inline void mod_y(int& begin, int& end)
	{
		//nothing
	}
	inline void set_y(const int& begin, const int& end)
	{
		//nothing
	}
	inline void inc_y()
	{
		//nothing
	}
	
	
	inline void mod_x(int& begin, int& end)
	{
		//nothing
	}
	inline void set_x(const int& begin, const int& end)
	{
		//nothing
	}
	inline void inc_x()
	{
		//nothing
	}
	
	inline const int& get_ref()
	{
		return i;
	}
};

template<typename PixelPtr, typename PixelRef>
struct controler_base
{
	
	const PixelPtr data;
	PixelPtr ptr_pos_y;
	PixelPtr ptr_pos_x;
	GraphSubset range;
	int beg_x;
	int beg_y;
	
	const std::pair<int, int> step;

		
	controler_base(PixelPtr base, const GraphSubset& r, const std::pair<int, int>& s) : data(base), ptr_pos_y(0), ptr_pos_x(0), range(r), beg_x(), beg_y(), step(s)
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
		beg_x = r.beg_x - range.beg_x;
		beg_y = r.beg_y - range.beg_y;
		range = r;
	}
	
	inline void mod_y(int& begin, int& end)
	{
		ptr_pos_y = data + step.first * beg_x + step.second * beg_y;
	}
	inline void set_y(const int& begin, const int& end)
	{
		ptr_pos_y += step.second*begin;		
	}
	inline void inc_y()
	{
		ptr_pos_y += step.second;		
	}
	
	
	inline void mod_x(int& begin, int& end)
	{
		ptr_pos_x = ptr_pos_y;
	}
	inline void set_x(const int& begin, const int& end)
	{
		ptr_pos_x += step.first*begin;
	}
	inline void inc_x()
	{
		ptr_pos_x += step.first;
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
		
	controler(const ShaderBase<Pixel>& f) : base_type(f.ptr(), f.getImage(), std::make_pair(1, f.pitch()))
	{
		
	}
	
};

}//namespace helper

}//namespace OpenXcom

#endif	/* SHADERDRAWHELPER_H */

