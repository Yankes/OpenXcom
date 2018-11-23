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
#include "AlienInventoryState.h"
#include "AlienInventory.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Surface.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/Text.h"
#include "../Mod/Mod.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "../Savegame/BattleUnit.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the AlienInventory screen.
 * @param unit Pointer to the alien unit.
 */
AlienInventoryState::AlienInventoryState(BattleUnit *unit)
{
	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	}

	// Create objects
	_bg = new Surface(320, 200, 0, 0);
	_soldier = new Surface(240, 200, 80, 0);
	_txtName = new Text(308, 17, 6, 6);
	_btnArmor = new BattlescapeButton(40, 70, 140, 65);
	_inv = new AlienInventory(_game, 320, 200, 0, 0);

	// Set palette
	setPalette("PAL_BATTLESCAPE");

	add(_bg);
	add(_soldier);
	add(_txtName, "textName", "inventory", _bg);
	add(_btnArmor, "buttonOK", "inventory", _bg);
	add(_inv);

	centerAllSurfaces();

	// Set up objects
	Surface *tmp = _game->getMod()->getSurface("AlienInventory", false);
	if (tmp)
	{
		tmp->blitNShade(_bg, 0, 0);
	}

	_txtName->setBig();
	_txtName->setHighContrast(true);
	_txtName->setAlign(ALIGN_CENTER);
	if (unit->getOriginalFaction() == FACTION_NEUTRAL)
	{
		_txtName->setText(tr(unit->getType()));
	}
	else
	{
		if (unit->getUnitRules())
		{
			if (unit->getUnitRules()->getShowFullNameInAlienInventory(_game->getMod()))
			{
				// e.g. Sectoid Leader
				_txtName->setText(unit->getName(_game->getLanguage()));
			}
			else
			{
				// e.g. Sectoid
				_txtName->setText(tr(unit->getUnitRules()->getRace()));
			}
		}
		else
		{
			// Soldier names
			_txtName->setText(unit->getName(_game->getLanguage()));
		}
	}

	_btnArmor->onKeyboardPress((ActionHandler)&AlienInventoryState::btnOkClick, Options::keyCancel);
	_btnArmor->onMouseClick((ActionHandler)&AlienInventoryState::btnArmorClickMiddle, SDL_BUTTON_MIDDLE);

	_soldier->clear();

	Soldier *s = unit->getGeoscapeSoldier();
	if (s)
	{
		auto defaultPrefix = s->getArmor()->getLayersDefaultPrefix();
		if (!defaultPrefix.empty())
		{
			auto layers = s->getArmorLayers();
			for (auto layer : layers)
			{
				auto surf = _game->getMod()->getSurface(layer, true);
				surf->blitNShade(_soldier->getSurface(), 0, 0);
			}
		}
		else
		{
			const std::string look = s->getArmor()->getSpriteInventory();
			const std::string gender = s->getGender() == GENDER_MALE ? "M" : "F";
			std::stringstream ss;
			Surface *surf = 0;

			for (int i = 0; i <= 4; ++i)
			{
				ss.str("");
				ss << look;
				ss << gender;
				ss << (int)s->getLook() + (s->getLookVariant() & (15 >> i)) * 4;
				ss << ".SPK";
				surf = _game->getMod()->getSurface(ss.str(), false);
				if (surf)
				{
					break;
				}
			}
			if (!surf)
			{
				ss.str("");
				ss << look;
				ss << ".SPK";
				surf = _game->getMod()->getSurface(ss.str(), true);
			}
			surf->blitNShade(_soldier, 0, 0);
		}
	}
	else
	{
		Surface *armorSurface = _game->getMod()->getSurface(unit->getArmor()->getSpriteInventory(), false);
		if (!armorSurface)
		{
			armorSurface = _game->getMod()->getSurface(unit->getArmor()->getSpriteInventory() + ".SPK", false);
		}
		if (!armorSurface)
		{
			armorSurface = _game->getMod()->getSurface(unit->getArmor()->getSpriteInventory() + "M0.SPK", false);
		}
		if (armorSurface)
		{
			armorSurface->blitNShade(_soldier, 0, 0);
		}
	}

	_inv->setSelectedUnit(unit);
	_inv->draw();

	// Bleeding indicator
	tmp = _game->getMod()->getSurface("BigWoundIndicator", false);
	if (tmp && unit->getFatalWounds() > 0)
	{
		tmp->blitNShade(_soldier, 32, 32);
	}
}

/**
 *
 */
AlienInventoryState::~AlienInventoryState()
{
	if (Options::maximizeInfoScreens)
	{
		Screen::updateScale(Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
		_game->getScreen()->resetDisplay(false);
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void AlienInventoryState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Opens Ufopaedia entry for the corresponding armor.
 * @param action Pointer to an action.
 */
void AlienInventoryState::btnArmorClickMiddle(Action *action)
{
	BattleUnit *unit = _inv->getSelectedUnit();
	if (unit != 0)
	{
		std::string articleId = unit->getArmor()->getType();
		Ufopaedia::openArticle(_game, articleId);
	}
}

}
