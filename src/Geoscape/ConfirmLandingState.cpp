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
#include "CraftErrorState.h"
#include "ConfirmLandingState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Engine/SurfaceSet.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Target.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Base.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/AlienBase.h"
#include "../Battlescape/BriefingState.h"
#include "../Battlescape/BattlescapeGenerator.h"
#include "../Engine/Exception.h"
#include "../Engine/Options.h"
#include "../Mod/RuleStartingCondition.h"
#include "../Mod/AlienDeployment.h"
#include "../Mod/AlienRace.h"
#include "../Mod/Mod.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Confirm Landing window.
 * @param game Pointer to the core game.
 * @param craft Pointer to the craft to confirm.
 * @param texture Texture of the landing site.
 * @param shade Shade of the landing site.
 */
ConfirmLandingState::ConfirmLandingState(Craft *craft, Texture *texture, int shade) : _craft(craft), _texture(texture), _shade(shade)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 216, 160, 20, 20, POPUP_BOTH);
	_btnYes = new TextButton(80, 20, 40, 150);
	_btnNo = new TextButton(80, 20, 136, 150);
	_txtMessage = new Text(206, 80, 25, 40);
	_txtBegin = new Text(206, 17, 25, 130);
	_sprite = new Surface(24, 24, 202, 30);

	// Set palette
	setInterface("confirmLanding");

	add(_window, "window", "confirmLanding");
	add(_btnYes, "button", "confirmLanding");
	add(_btnNo, "button", "confirmLanding");
	add(_txtMessage, "text", "confirmLanding");
	add(_txtBegin, "text", "confirmLanding");
	add(_sprite);

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK15.SCR"));

	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick((ActionHandler)&ConfirmLandingState::btnYesClick);
	_btnYes->onKeyboardPress((ActionHandler)&ConfirmLandingState::btnYesClick, Options::keyOk);

	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick((ActionHandler)&ConfirmLandingState::btnNoClick);
	_btnNo->onKeyboardPress((ActionHandler)&ConfirmLandingState::btnNoClick, Options::keyCancel);

	_txtMessage->setBig();
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setWordWrap(true);
	_txtMessage->setText(tr("STR_CRAFT_READY_TO_LAND_NEAR_DESTINATION")
						 .arg(_craft->getName(_game->getLanguage()))
						 .arg(_craft->getDestination()->getName(_game->getLanguage())));

	_txtBegin->setBig();
	_txtBegin->setAlign(ALIGN_CENTER);
	std::ostringstream ss;
	ss << Unicode::TOK_COLOR_FLIP << tr("STR_BEGIN_MISSION");
	_txtBegin->setText(ss.str());

	SurfaceSet *sprites = _game->getMod()->getSurfaceSet("DayNightIndicator", false);
	if (sprites != 0)
	{
		if (_shade <= 0)
		{
			// day (0)
			sprites->getFrame(0)->blitNShade(_sprite, 0, 0);
		}
		else if (_shade > _game->getMod()->getMaxDarknessToSeeUnits())
		{
			// night (10-15); note: this is configurable in the ruleset (in OXCE only)
			sprites->getFrame(1)->blitNShade(_sprite, 0, 0);
		}
		else
		{
			// dusk/dawn (1-9)
			sprites->getFrame(2)->blitNShade(_sprite, 0, 0);
		}
	}
}

/**
 *
 */
ConfirmLandingState::~ConfirmLandingState()
{

}

/*
 * Make sure we aren't returning to base.
 */
void ConfirmLandingState::init()
{
	State::init();
	Base* b = dynamic_cast<Base*>(_craft->getDestination());
	if (b == _craft->getBase())
		_game->popState();
}

/**
* Checks the starting condition.
*/
bool ConfirmLandingState::checkStartingCondition()
{
	Ufo* u = dynamic_cast<Ufo*>(_craft->getDestination());
	MissionSite* m = dynamic_cast<MissionSite*>(_craft->getDestination());
	AlienBase* b = dynamic_cast<AlienBase*>(_craft->getDestination());

	AlienDeployment *ruleDeploy = 0;
	if (u != 0)
	{
		ruleDeploy = _game->getMod()->getDeployment(u->getRules()->getType());
	}
	else if (m != 0)
	{
		ruleDeploy = _game->getMod()->getDeployment(m->getDeployment()->getType());
	}
	else if (b != 0)
	{
		AlienRace *race = _game->getMod()->getAlienRace(b->getAlienRace());
		ruleDeploy = _game->getMod()->getDeployment(race->getBaseCustomMission());
		if (!ruleDeploy) ruleDeploy = _game->getMod()->getDeployment(b->getDeployment()->getType());
	}
	else
	{
		// irrelevant for this check
		return true;
	}

	if (ruleDeploy == 0)
	{
		// just in case
		return true;
	}

	RuleStartingCondition *rule = _game->getMod()->getStartingCondition(ruleDeploy->getStartingCondition());
	return rule == 0 || rule->isCraftAllowed(_craft->getRules()->getType());
}

/**
 * Enters the mission.
 * @param action Pointer to an action.
 */
void ConfirmLandingState::btnYesClick(Action *)
{
	if (!checkStartingCondition())
	{
		_craft->returnToBase();
		_game->popState();
		_game->pushState(new CraftErrorState(0, tr("STR_STARTING_CONDITION_CRAFT")));
		return;
	}

	_game->popState();
	Ufo* u = dynamic_cast<Ufo*>(_craft->getDestination());
	MissionSite* m = dynamic_cast<MissionSite*>(_craft->getDestination());
	AlienBase* b = dynamic_cast<AlienBase*>(_craft->getDestination());

	SavedBattleGame *bgame = new SavedBattleGame(_game->getMod());
	_game->getSavedGame()->setBattleGame(bgame);
	BattlescapeGenerator bgen(_game);
	bgen.setWorldTexture(_texture);
	bgen.setWorldShade(_shade);
	bgen.setCraft(_craft);
	if (u != 0)
	{
		if (u->getStatus() == Ufo::CRASHED)
			bgame->setMissionType("STR_UFO_CRASH_RECOVERY");
		else
			bgame->setMissionType("STR_UFO_GROUND_ASSAULT");
		bgen.setUfo(u);
		bgen.setAlienCustomDeploy(_game->getMod()->getDeployment(u->getCraftStats().craftCustomDeploy));
		bgen.setAlienRace(u->getAlienRace());
	}
	else if (m != 0)
	{
		bgame->setMissionType(m->getDeployment()->getType());
		bgen.setMissionSite(m);
		bgen.setAlienCustomDeploy(m->getMissionCustomDeploy());
		bgen.setAlienRace(m->getAlienRace());
	}
	else if (b != 0)
	{
		AlienRace *race = _game->getMod()->getAlienRace(b->getAlienRace());
		bgame->setMissionType(b->getDeployment()->getType());
		bgen.setAlienBase(b);
		bgen.setAlienRace(b->getAlienRace());
		bgen.setAlienCustomDeploy(_game->getMod()->getDeployment(race->getBaseCustomDeploy()), _game->getMod()->getDeployment(race->getBaseCustomMission()));
		bgen.setWorldTexture(0);
	}
	else
	{
		throw Exception("No mission available!");
	}
	bgen.run();
	_game->pushState(new BriefingState(_craft));
}

/**
 * Returns the craft to base and closes the window.
 * @param action Pointer to an action.
 */
void ConfirmLandingState::btnNoClick(Action *)
{
	_craft->returnToBase();
	_game->popState();
}

}
