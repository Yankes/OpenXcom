/*
 * Copyright 2010-2018 OpenXcom Developers.
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
#include "HelloCommanderState.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the HelloCommander screen.
 */
HelloCommanderState::HelloCommanderState()
{
	_exit = false;

	// Check 1: XcomFiles
	Surface *bruce = _game->getMod()->getSurface("UFOPEDIA_IMG_NEED_TO_KNOW_BASIS_CPAL", false);
	if (!bruce)
	{
		_exit = true;
		return;
	}

	// Check 2: 100th mission, 200th mission, etc.
	const int missionModulo = 100;
	int missionNumber = _game->getSavedGame()->getMissionStatistics()->size();
	if (missionNumber % missionModulo != 0)
	{
		_exit = true;
		return;
	}

	// Check 3: Base name search
	const std::string hqNameSearch = "FBI HQ";
	bool found = false;
	for (auto base : *_game->getSavedGame()->getBases())
	{
		if (base->getName().find(hqNameSearch) == 0)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		_exit = true;
		return;
	}

	// Check 4: Dead agents name search
	const std::string soldierNameSearch1 = "Agent Zargosian";
	const std::string soldierNameSearch2 = "Agent Bicentius";
	found = false;
	for (auto dead : *_game->getSavedGame()->getDeadSoldiers())
	{
		if (dead->getName().find(soldierNameSearch1) == 0)
		{
			found = true;
			break;
		}
		if (dead->getName().find(soldierNameSearch2) == 0)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		_exit = true;
		return;
	}

	// Create objects
	_bg = new Surface(320, 200, 0, 0);
	_txtMessage = new Text(168, 144, 8, 28);
	_btnOk = new TextButton(180, 16, 70, 180);

	if (bruce)
	{
		setCustomPalette(bruce->getPalette(), Mod::UFOPAEDIA_CURSOR);
	}
	else
	{
		setPalette("PAL_UFOPAEDIA");
	}

	add(_bg);
	add(_txtMessage);
	add(_btnOk);

	centerAllSurfaces();

	// Set up objects
	if (bruce)
	{
		bruce->blitNShade(_bg, 0, 0);
	}

	_btnOk->setColor(239);
	std::string backTo = "Back to saving the world...";
	_btnOk->setText(backTo);
	_btnOk->onMouseClick((ActionHandler)&HelloCommanderState::btnOkClick);

	_txtMessage->setColor(244);
	_txtMessage->setWordWrap(true);
	std::ostringstream ss;
	ss << "Hello Lewis and Ben,\n\nthis was your ";
	ss << missionNumber;
	ss << "th mission!\n\nWe are extremely impressed with the progress of the XCOM project thus far, Commanders. Your recent results were beyond our expectations... and that is not a statement this Council makes lightly.\n\nAnd remember, we'll be watching...\n\n          -- Meridian";
	_txtMessage->setText(ss.str());
}

/**
 *
 */
HelloCommanderState::~HelloCommanderState()
{

}

/**
 * Initializes the state.
 */
void HelloCommanderState::init()
{
	if (_exit)
	{
		_game->popState();
		return;
	}
	State::init();
}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void HelloCommanderState::btnOkClick(Action *)
{
	_game->popState();
}

}
