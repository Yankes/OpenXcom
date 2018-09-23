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
#include <SDL.h>
#include <string>
#include <memory>
#include <vector>
#include <assert.h>
#include "GraphSubset.h"

namespace OpenXcom
{

class Font;
class Language;
class ScriptWorkerBase;
class SurfaceCrop;
template<typename Pixel> class SurfaceRaw;

/**
 * Element that is blit (rendered) onto the screen.
 * Mainly an encapsulation for SDL's SDL_Surface struct, so it
 * borrows a lot of its terminology. Takes care of all the common
 * rendering tasks and color effects, while serving as the base
 * class for more specialized screen elements.
 */
class Surface
{
public:
	struct UniqueBufferDeleter
	{
		void operator()(Uint8*);
	};
	struct UniqueSurfaceDeleter
	{
		void operator()(SDL_Surface*);
	};

	using UniqueBufferPtr = std::unique_ptr<Uint8, UniqueBufferDeleter>;
	using UniqueSurfacePtr = std::unique_ptr<SDL_Surface, UniqueSurfaceDeleter>;

protected:
	UniqueBufferPtr _alignedBuffer;
	UniqueSurfacePtr _surface;
	Sint16 _x, _y;
	Uint16 _width, _height, _pitch;
	bool _visible, _hidden, _redraw;

	void resize(int width, int height);
public:
	/// Default empy surface.
	Surface();
	/// Creates a new surface with the specified size and position.
	Surface(int width, int height, int x = 0, int y = 0, int bpp = 8);
	/// Creates a new surface from an existing one.
	Surface(const Surface& other);
	/// Move surface to another place.
	Surface(Surface&& other) = default;
	/// Move assigment
	Surface& operator=(Surface&& other) = default;
	/// Copy assigment
	Surface& operator=(const Surface& other) { *this = Surface(other); return *this; };
	/// Cleans up the surface.
	virtual ~Surface();

	/// Is surface empy?
	explicit operator bool() const
	{
		return _alignedBuffer.get();
	}

	/// Loads an X-Com SCR graphic.
	void loadScr(const std::string &filename);
	/// Loads an X-Com SPK graphic.
	void loadSpk(const std::string &filename);
	/// Loads a TFTD BDY graphic.
	void loadBdy(const std::string &filename);
	/// Loads a general image file.
	void loadImage(const std::string &filename);
	/// Clears the surface's contents eith a specified colour.
	void clear(Uint32 color = 0);
	/// Offsets the surface's colors by a set amount.
	void offset(int off, int min = -1, int max = -1, int mul = 1);
	/// Offsets the surface's colors in a color block.
	void offsetBlock(int off, int blk = 16, int mul = 1);
	/// Inverts the surface's colors.
	void invert(Uint8 mid);
	/// Runs surface functionality every cycle
	virtual void think();
	/// Draws the surface's graphic.
	virtual void draw();
	/// Blits this surface onto another one.
	virtual void blit(Surface *surface);
	/// Initializes the surface's various text resources.
	virtual void initText(Font *, Font *, Language *) {};
	/// Copies a portion of another surface into this one.
	void copy(Surface *surface);
	/// Draws a filled rectangle on the surface.
	void drawRect(SDL_Rect *rect, Uint8 color);
	/// Draws a filled rectangle on the surface.
	void drawRect(Sint16 x, Sint16 y, Sint16 w, Sint16 h, Uint8 color);
	/// Draws a line on the surface.
	void drawLine(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 color);
	/// Draws a filled circle on the surface.
	void drawCircle(Sint16 x, Sint16 y, Sint16 r, Uint8 color);
	/// Draws a filled polygon on the surface.
	void drawPolygon(Sint16 *x, Sint16 *y, int n, Uint8 color);
	/// Draws a textured polygon on the surface.
	void drawTexturedPolygon(Sint16 *x, Sint16 *y, int n, Surface *texture, int dx, int dy);
	/// Draws a string on the surface.
	void drawString(Sint16 x, Sint16 y, const char *s, Uint8 color);
	/// Sets the surface's palette.
	virtual void setPalette(SDL_Color *colors, int firstcolor = 0, int ncolors = 256);
	/**
	 * Returns the surface's 8bpp palette.
	 * @return Pointer to the palette's colors.
	 */
	SDL_Color *getPalette() const
	{
		return _surface->format->palette->colors;
	}
	/// Sets the X position of the surface.
	virtual void setX(int x);
	/**
	 * Returns the position of the surface in the X axis.
	 * @return X position in pixels.
	 */
	int getX() const
	{
		return _x;
	}
	/// Sets the Y position of the surface.
	virtual void setY(int y);
	/**
	 * Returns the position of the surface in the Y axis.
	 * @return Y position in pixels.
	 */
	int getY() const
	{
		return _y;
	}
	/// Sets the surface's visibility.
	virtual void setVisible(bool visible);
	/// Gets the surface's visibility.
	bool getVisible() const;
	/// Gets the cropping rectangle for the surface.
	SurfaceCrop getCrop();
	/**
	 * Changes the color of a pixel in the surface, relative to
	 * the top-left corner of the surface.
	 * @param x X position of the pixel.
	 * @param y Y position of the pixel.
	 * @param pixel New color for the pixel.
	 */
	void setPixel(int x, int y, Uint8 pixel)
	{
		if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight())
		{
			return;
		}
		((Uint8 *)_surface->pixels)[y * _surface->pitch + x * _surface->format->BytesPerPixel] = pixel;
	}
	/**
	 * Changes the color of a pixel in the surface and returns the
	 * next pixel position. Useful when changing a lot of pixels in
	 * a row, eg. loading images.
	 * @param x Pointer to the X position of the pixel. Changed to the next X position in the sequence.
	 * @param y Pointer to the Y position of the pixel. Changed to the next Y position in the sequence.
	 * @param pixel New color for the pixel.
	 */
	void setPixelIterative(int *x, int *y, Uint8 pixel)
	{
		setPixel(*x, *y, pixel);
		(*x)++;
		if (*x == getWidth())
		{
			(*y)++;
			*x = 0;
		}
	}
	/**
	 * Returns the color of a specified pixel in the surface.
	 * @param x X position of the pixel.
	 * @param y Y position of the pixel.
	 * @return Color of the pixel.
	 */
	Uint8 getPixel(int x, int y) const
	{
		if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight())
		{
			return 0;
		}
		return ((Uint8 *)_surface->pixels)[y * _surface->pitch + x * _surface->format->BytesPerPixel];
	}
	/**
	 * Returns the internal SDL_Surface for SDL calls.
	 * @return Pointer to the surface.
	 */
	SDL_Surface *getSurface() const
	{
		return _surface.get();
	}
	/**
	 * Returns the width of the surface.
	 * @return Width in pixels.
	 */
	int getWidth() const
	{
		return _width;
	}
	/// Sets the width of the surface.
	virtual void setWidth(int width);
	/**
	 * Returns the height of the surface.
	 * @return Height in pixels
	 */
	int getHeight() const
	{
		return _height;
	}
	/// Sets the height of the surface.
	virtual void setHeight(int height);
	/// Get surface pitch
	int getPitch() const
	{
		return _pitch;
	}
	/// Get pointer to buffer
	Uint8* getBuffer()
	{
		return _alignedBuffer.get();
	}
	/// Get pointer to buffer
	const Uint8* getBuffer() const
	{
		return _alignedBuffer.get();
	}
	/// Sets the surface's special hidden flag.
	void setHidden(bool hidden);
	/// Locks the surface.
	void lock();
	/// Unlocks the surface.
	void unlock();
	/// Specific blit function to blit battlescape terrain data in different shades in a fast way.
	static void blitRaw(SurfaceRaw<Uint8> dest, SurfaceRaw<const Uint8> src, int x, int y, int shade, bool half = false, int newBaseColor = 0);
	/// Specific blit function to blit battlescape terrain data in different shades in a fast way.
	void blitNShade(SurfaceRaw<Uint8> surface, int x, int y, int shade, bool half = false, int newBaseColor = 0);
	/// Specific blit function to blit battlescape terrain data in different shades in a fast way.
	void blitNShade(SurfaceRaw<Uint8> surface, int x, int y, int shade, GraphSubset range);
	/// Invalidate the surface: force it to be redrawn
	void invalidate(bool valid = true);

	/// Sets the color of the surface.
	virtual void setColor(Uint8 /*color*/) { /* empty by design */ };
	/// Sets the secondary color of the surface.
	virtual void setSecondaryColor(Uint8 /*color*/) { /* empty by design */ };
	/// Sets the border colour of the surface.
	virtual void setBorderColor(Uint8 /*color*/) { /* empty by design */ };
	/// Sets the high contrast color setting of the surface.
	virtual void setHighContrast(bool /*contrast*/) { /* empty by design */ };
};

/**
 * Raw pointer to surface buffer, can be created from diffrent sources
 */
template<typename Pixel>
class SurfaceRaw
{
	Pixel* _buffer;
	Uint16 _width, _height, _pitch;

public:
	/// Default constructor
	SurfaceRaw() :
		_buffer{ nullptr },
		_width{ 0 },
		_height{ 0 },
		_pitch{ 0 }
	{

	}

	/// Copy constructor
	SurfaceRaw(const SurfaceRaw&) = default;

	/// Move constructor
	SurfaceRaw(SurfaceRaw&&) = default;

	/// Constructor
	SurfaceRaw(Pixel* buffer, int width, int height, int pitch) :
		_buffer{ buffer },
		_width{ static_cast<Uint16>(width) },
		_height{ static_cast<Uint16>(height) },
		_pitch{ static_cast<Uint16>(pitch) }
	{

	}

	/// Constructor, SFINAE enable it only for `Uint8`
	template<typename = std::enable_if<std::is_same<Uint8, Pixel>::value, void>>
	SurfaceRaw(Surface* surf) : SurfaceRaw{ surf->getBuffer(), surf->getWidth(), surf->getHeight(), surf->getPitch() }
	{

	}

	/// Constructor, SFINAE enable it only for `Uint8`
	template<typename = std::enable_if<std::is_same<const Uint8, Pixel>::value, void>>
	SurfaceRaw(const Surface* surf) : SurfaceRaw{ surf->getBuffer(), surf->getWidth(), surf->getHeight(), surf->getPitch() }
	{

	}

	/// Constructor, SFINAE enable it only for `Uint8`
	template<typename = std::enable_if<std::is_same<Uint8, Pixel>::value, void>>
	SurfaceRaw(SDL_Surface* surf) : SurfaceRaw{ (Pixel*)surf->pixels, surf->w, surf->h, surf->pitch }
	{

	}

	/// Constructor, SFINAE enable it only for `const Uint8`
	template<typename = std::enable_if<std::is_same<const Uint8, Pixel>::value, void>>
	SurfaceRaw(const SDL_Surface* surf) : SurfaceRaw{ (Pixel*)surf->pixels, surf->w, surf->h, surf->pitch }
	{

	}

	/// Constructor, SFINAE enable it only for non const `PixelType`
	template<typename = std::enable_if<std::is_const<Pixel>::value == false, void>>
	SurfaceRaw(std::vector<Pixel>& vec, int width, int height) : SurfaceRaw{ vec.data(), width, height, width }
	{
		assert((size_t)(width*height) <= vec.size() && "Incorrect dimensions compared to vector size");
	}

	/// Constructor, SFINAE enable it only for `const PixelType`
	template<typename = std::enable_if<std::is_const<Pixel>::value, void>>
	SurfaceRaw(const std::vector<Pixel>& vec, int width, int height) : SurfaceRaw{ vec.data(), width, height, width }
	{
		assert((size_t)(width*height) <= vec.size() && "Incorrect dimensions compared to vector size");
	}

	/// Constructor
	template<int I>
	SurfaceRaw(Pixel (&buffer)[I], int width, int height) : SurfaceRaw{ buffer, width, height, width }
	{
		assert(width*height <= I && "Incorrect dimensions compared to array size");
	}

	/// Assigment from nullptr
	SurfaceRaw& operator=(std::nullptr_t)
	{
		*this = SurfaceRaw{};
		return *this;
	}

	/// Assigment
	SurfaceRaw& operator=(const SurfaceRaw&) = default;

	/// Is empty?
	explicit operator bool() const
	{
		return _buffer;
	}

	/// Returns the width of the surface.
	int getWidth() const
	{
		return _width;
	}

	/// Returns the height of the surface.
	int getHeight() const
	{
		return _height;
	}

	/// Get surface pitch
	int getPitch() const
	{
		return _pitch;
	}

	/// Get pointer to buffer
	Pixel* getBuffer() const
	{
		return _buffer;
	}
};

/**
 * Helper class used to blit part of surface to another one.
 */
class SurfaceCrop
{
	Surface* _surface;
	SDL_Rect _crop;
	int _x, _y;

public:
	/// Default constructor
	SurfaceCrop() : _surface{ nullptr }, _crop{ }, _x{ }, _y{ }
	{

	}

	/// Constructor
	SurfaceCrop(Surface* surf) : _surface{ surf }, _crop{ }, _x{ surf->getX() }, _y{ surf->getY() }
	{

	}

	/// Get crop rectangle.
	SDL_Rect* getCrop()
	{
		return &_crop;
	}

	/// Get Surface.
	Surface* getSurface()
	{
		return _surface;
	}

	/// Sets the X position of the surface.
	void setX(int x)
	{
		_x = x;
	}

	/// Returns the position of the surface in the X axis.
	int getX() const
	{
		return _x;
	}

	/// Sets the Y position of the surface.
	void setY(int y)
	{
		_y = y;
	}

	/// Returns the position of the surface in the Y axis.
	int getY() const
	{
		return _y;
	}

	/// Blit Croped surface to another surface.
	void blit(Surface* dest);
};

}
