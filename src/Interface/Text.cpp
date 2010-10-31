/*
 * Copyright 2010 OpenXcom Developers.
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
#include "Text.h"
#include <sstream>
#include "../Engine/Font.h"

/**
 * Sets up a blank text with the specified size and position.
 * The different fonts need to be passed in advance since the
 * text size can change mid-text.
 * @param big Pointer to the big-size font.
 * @param small Pointer to the small-size font.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
Text::Text(Font *big, Font *small, int width, int height, int x, int y) : Surface(width, height, x, y), _big(big), _small(small), _font(small), _text(""), _wrap(false), _invert(false), _align(ALIGN_LEFT), _valign(ALIGN_TOP), _color(0)
{
	processText();
}

/**
 *
 */
Text::~Text()
{
	
}

/**
 * Takes an integer value and formats it as currency,
 * spacing the thousands and adding a $ sign to the front.
 * @param funds The funding value.
 * @return The formatted string.
 */
std::string Text::formatFunding(int funds)
{
	std::stringstream ss;
	ss << funds;
	std::string s = ss.str();
	size_t spacer = s.size() - 3;
	while (spacer > 0 && spacer < s.size())
	{
		s.insert(spacer, " ");
		spacer -= 3;
	}
	s.insert(0, "$");
	return s;
}

/**
 * Changes the text to use the big-size font.
 */
void Text::setBig()
{
	_font = _big;
}

/**
 * Changes the text to use the small-size font.
 */
void Text::setSmall()
{
	_font = _small;
}

/**
 * Returns the font currently used by the text.
 * @return Pointer to font.
 */
Font *const Text::getFont() const
{
	return _font;
}

/**
 * Changes the string displayed on screen.
 * @param text Text string.
 */
void Text::setText(const std::string &text)
{
	if (text != _text)
	{
		_text = text;
		processText();
	}
}

/**
 * Returns the string displayed on screen.
 * @return Text string.
 */
std::string Text::getText() const
{
	return _text;
}

/**
 * Enables/disables text wordwrapping. When enabled, lines of
 * text are automatically split to ensure they stay within the
 * drawing area, otherwise they simply go off the edge.
 * @param wrap Wordwrapping setting.
 */
void Text::setWordWrap(bool wrap)
{
	if (wrap != _wrap)
	{
		_wrap = wrap;
		processText();
	}
}

/**
 * Enables/disables color inverting. Mostly used to make
 * button text look pressed along with the button.
 * @param invert Invert setting.
 */
void Text::setInvert(bool invert)
{
	_invert = invert;
	draw();
}

/**
 * Changes the way the text is aligned horizontally
 * relative to the drawing area.
 * @param align Horizontal alignment.
 */
void Text::setAlign(TextHAlign align)
{
	_align = align;
	draw();
}

/**
 * Changes the way the text is aligned vertically
 * relative to the drawing area.
 * @param valign Vertical alignment.
 */
void Text::setVerticalAlign(TextVAlign valign)
{
	_valign = valign;
	draw();
}

/**
 * Changes the color used to render the text. Unlike regular graphics,
 * fonts are greyscale so they need to be assigned a specific position
 * in the palette to be displayed.
 * @param color Color value.
 */
void Text::setColor(Uint8 color)
{
	_color = color;
	draw();
}

/**
 * Returns the color used to render the text.
 * @return Color value.
 */
Uint8 Text::getColor() const
{
	return _color;
}

/**
 * Takes care of any text post-processing like calculating
 * line metrics for alignment and wordwrapping if necessary.
 */
void Text::processText()
{
	std::string *s = &_text;

	// Use a separate string for wordwrapping text
	if (_wrap)
	{
		_wrappedText = _text;
		s = &_wrappedText;
	}

	_lineWidth.clear();
	_lineHeight.clear();

	int width = 0, word = 0;
	std::string::iterator space = s->begin();
	Font *font = _font;

	// Go through the text character by character
	for (std::string::iterator c = s->begin(); c <= s->end(); c++)
	{
		// End of the line
		if (c == s->end() || *c == '\n' || *c == 2)
		{
			// Add line measurements for alignment later
			_lineWidth.push_back(width);
			_lineHeight.push_back(font->getHeight() + font->getSpacing());
			width = 0;
			word = 0;
			
			if (c == s->end())
				break;
			else if (*c == 2)
				font = _small;
		}
		// Keep track of spaces for wordwrapping
		else if (*c == ' ')
		{
			width += font->getWidth() / 2;
			space = c;
			word = 0;
		}
		// Keep track of the width of the last line and word
		else
		{
			width += font->getChar(*c)->getCrop()->w + font->getSpacing();
			word += font->getChar(*c)->getCrop()->w + font->getSpacing();
		}

		// Wordwrap if the last word doesn't fit the line
		if (_wrap && width > _width)
		{
			// Go back to the last space and put a linebreak there
			*space = '\n';
			c = space - 1;
			width -= word + font->getWidth() / 2;
		}
	}

	draw();
}

/**
 * Draws all the characters in the text with a really
 * nasty complex gritty text rendering algorithm logic stuff.
 */
void Text::draw()
{
	clear();

	/*
	SDL_Rect r;
	r.w = _width;
	r.h = _height;
	r.x = 0;
	r.y = 0;
	SDL_FillRect(_surface, &r, 1);
	*/

	int x = 0, y = 0, line = 0, height = 0;
	Font *font = _font;
	std::string *s = &_text;

	for (std::vector<int>::iterator i = _lineHeight.begin(); i != _lineHeight.end(); i++)
		height += *i;

	switch (_valign)
	{
	case ALIGN_TOP:
		y = 0;
		break;
	case ALIGN_MIDDLE:
		y = (_height - height) / 2;
		break;
	case ALIGN_BOTTOM:
		y = _height - height;
		break;
	}

	switch (_align)
	{
	case ALIGN_LEFT:
		x = 0;
		break;
	case ALIGN_CENTER:
		x = (_width - _lineWidth[line]) / 2;
		break;
	case ALIGN_RIGHT:
		x = _width - _lineWidth[line];
		break;
	}

	if (_wrap)
		s = &_wrappedText;

	// Draw each letter one by one
	for (std::string::iterator c = s->begin(); c != s->end(); c++)
	{
		if (*c == ' ')
		{
			x += font->getWidth() / 2;
		}
		else if (*c == '\n' || *c == 2)
		{
			line++;
			y += font->getHeight() + font->getSpacing();
			switch (_align)
			{
			case ALIGN_LEFT:
				x = 0;
				break;
			case ALIGN_CENTER:
				x = (_width - _lineWidth[line]) / 2;
				break;
			case ALIGN_RIGHT:
				x = _width - _lineWidth[line];
				break;
			}
			if (*c == 2)
				font = _small;
		}
		else
		{
			Surface* chr = font->getChar(*c);
			chr->setX(x);
			chr->setY(y);
			chr->blit(this);
			x += chr->getCrop()->w + font->getSpacing();
		}
	}

	this->offset(_color);
	if (_invert)
		this->invert(_color+3);
}
