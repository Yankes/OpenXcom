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
#include "Base.h"
#include "../fmath.h"
#include <stack>
#include <algorithm>
#include <cassert>
#include <tuple>
#include "BaseFacility.h"
#include "../Mod/RuleBaseFacility.h"
#include "Craft.h"
#include "CraftWeapon.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Mod/Mod.h"
#include "ItemContainer.h"
#include "Soldier.h"
#include "../Engine/Language.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleManufacture.h"
#include "../Mod/RuleResearch.h"
#include "Transfer.h"
#include "ResearchProject.h"
#include "Production.h"
#include "Vehicle.h"
#include "Target.h"
#include "Ufo.h"
#include "../Engine/RNG.h"
#include "../Engine/Options.h"
#include "../Mod/RuleSoldier.h"
#include "../Engine/Logger.h"
#include "../Engine/Collections.h"
#include "HangarAllocation.h"

namespace OpenXcom
{

/**
 * Initializes an empty base.
 * @param mod Pointer to mod.
 */
Base::Base(const Mod *mod) : Target(), _mod(mod), _scientists(0), _engineers(0), _inBattlescape(false), _retaliationTarget(false)
{
	_items = new ItemContainer();
}

/**
 * Deletes the contents of the base from memory.
 */
Base::~Base()
{
	for (std::vector<BaseFacility*>::iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<Soldier*>::iterator i = _soldiers.begin(); i != _soldiers.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<Craft*>::iterator i = _crafts.begin(); i != _crafts.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<Transfer*>::iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<Production *>::iterator i = _productions.begin(); i != _productions.end(); ++i)
	{
		delete *i;
	}
	delete _items;
	for (std::vector<ResearchProject*>::iterator i = _research.begin(); i != _research.end(); ++i)
	{
		delete *i;
	}
	Collections::deleteAll(_vehiclesFromBase);
}

/**
 * Loads the base from a YAML file.
 * @param node YAML node.
 * @param save Pointer to saved game.
 * @param newGame Is this the first base of a new game?
 * @param newBattleGame Is this the base of a skirmish game?
 */
void Base::load(const YAML::Node &node, SavedGame *save, bool newGame, bool newBattleGame)
{
	Target::load(node);

	if (!newGame || !Options::customInitialBase || newBattleGame)
	{
		for (YAML::const_iterator i = node["facilities"].begin(); i != node["facilities"].end(); ++i)
		{
			std::string type = (*i)["type"].as<std::string>();
			if (_mod->getBaseFacility(type))
			{
				BaseFacility *f = new BaseFacility(_mod->getBaseFacility(type), this);
				f->load(*i);
				_facilities.push_back(f);
			}
			else
			{
				Log(LOG_ERROR) << "Failed to load facility " << type;
			}
		}
	}

	for (YAML::const_iterator i = node["crafts"].begin(); i != node["crafts"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (_mod->getCraft(type))
		{
			Craft *c = new Craft(_mod->getCraft(type), this);
			c->load(*i, _mod, save);
			_crafts.push_back(c);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load craft " << type;
		}
	}

	for (YAML::const_iterator i = node["soldiers"].begin(); i != node["soldiers"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>(_mod->getSoldiersList().front());
		if (_mod->getSoldier(type))
		{
			Soldier *s = new Soldier(_mod->getSoldier(type), 0);
			s->load(*i, _mod, save, _mod->getScriptGlobal());
			s->setCraft(0);
			if (const YAML::Node &craft = (*i)["craft"])
			{
				CraftId craftId = Craft::loadId(craft);
				for (std::vector<Craft*>::iterator j = _crafts.begin(); j != _crafts.end(); ++j)
				{
					if ((*j)->getUniqueId() == craftId)
					{
						s->setCraft(*j);
						break;
					}
				}
			}
			_soldiers.push_back(s);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load soldier " << type;
		}
	}

	_items->load(node["items"]);
	// Some old saves have bad items, better get rid of them to avoid further bugs
	for (std::map<std::string, int>::iterator i = _items->getContents()->begin(); i != _items->getContents()->end();)
	{
		if (_mod->getItem(i->first) == 0)
		{
			Log(LOG_ERROR) << "Failed to load item " << i->first;
			_items->getContents()->erase(i++);
		}
		else
		{
			++i;
		}
	}

	_scientists = node["scientists"].as<int>(_scientists);
	_engineers = node["engineers"].as<int>(_engineers);
	_inBattlescape = node["inBattlescape"].as<bool>(_inBattlescape);

	for (YAML::const_iterator i = node["transfers"].begin(); i != node["transfers"].end(); ++i)
	{
		int hours = (*i)["hours"].as<int>();
		Transfer *t = new Transfer(hours);
		if (t->load(*i, this, _mod, save))
		{
			_transfers.push_back(t);
		}
	}

	for (YAML::const_iterator i = node["research"].begin(); i != node["research"].end(); ++i)
	{
		std::string research = (*i)["project"].as<std::string>();
		if (_mod->getResearch(research))
		{
			ResearchProject *r = new ResearchProject(_mod->getResearch(research));
			r->load(*i);
			_research.push_back(r);
		}
		else
		{
			_scientists += (*i)["assigned"].as<int>(0);
			Log(LOG_ERROR) << "Failed to load research " << research;
		}
	}

	for (YAML::const_iterator i = node["productions"].begin(); i != node["productions"].end(); ++i)
	{
		std::string item = (*i)["item"].as<std::string>();
		if (_mod->getManufacture(item))
		{
			Production *p = new Production(_mod->getManufacture(item), 0);
			p->load(*i, this);
			_productions.push_back(p);
		}
		else
		{
			_engineers += (*i)["assigned"].as<int>(0);
			Log(LOG_ERROR) << "Failed to load manufacture " << item;
		}
	}

	_retaliationTarget = node["retaliationTarget"].as<bool>(_retaliationTarget);

	initHangars();
}

/**
 * Saves the base to a YAML file.
 * @return YAML node.
 */
YAML::Node Base::save() const
{
	YAML::Node node = Target::save();
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		node["facilities"].push_back((*i)->save());
	}
	for (std::vector<Soldier*>::const_iterator i = _soldiers.begin(); i != _soldiers.end(); ++i)
	{
		node["soldiers"].push_back((*i)->save(_mod->getScriptGlobal()));
	}
	for (std::vector<Craft*>::const_iterator i = _crafts.begin(); i != _crafts.end(); ++i)
	{
		node["crafts"].push_back((*i)->save());
	}
	node["items"] = _items->save();
	node["scientists"] = _scientists;
	node["engineers"] = _engineers;
	if (_inBattlescape)
		node["inBattlescape"] = _inBattlescape;
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		node["transfers"].push_back((*i)->save(this, _mod));
	}
	for (std::vector<ResearchProject*>::const_iterator i = _research.begin(); i != _research.end(); ++i)
	{
		node["research"].push_back((*i)->save());
	}
	for (std::vector<Production*>::const_iterator i = _productions.begin(); i != _productions.end(); ++i)
	{
		node["productions"].push_back((*i)->save(this));
	}
	if (_retaliationTarget)
	node["retaliationTarget"] = _retaliationTarget;
	return node;
}

/**
 * Saves the base's unique identifiers to a YAML file.
 * @return YAML node.
 */
YAML::Node Base::saveId() const
{
	YAML::Node node = Target::saveId();
	node["type"] = "STR_BASE";
	node["id"] = 0;
	return node;
}

/**
 * Returns the custom name for the base.
 * @param lang Language to get strings from.
 * @return Name.
 */
std::wstring Base::getName(Language *) const
{
	return _name;
}

/**
 * Returns the globe marker for the base.
 * @return Marker sprite, -1 if none.
 */
int Base::getMarker() const
{
	// Cheap hack to hide bases when they haven't been placed yet
	if (AreSame(_lon, 0.0) && AreSame(_lat, 0.0))
		return -1;
	return 0;
}

/**
 * Returns the list of facilities in the base.
 * @return Pointer to the facility list.
 */
std::vector<BaseFacility*> *Base::getFacilities()
{
	return &_facilities;
}

/**
 * Returns the list of facilities in the base.
 * @return Pointer to the facility list.
 */
const std::vector<BaseFacility*> *Base::getFacilities() const
{
	return &_facilities;
}

/**
 * Get facilitie from reference.
 */
BaseFacility* Base::loadFacilitieReference(const YAML::Node &node) const
{
	if (node)
	{
		BaseFacility temp(nullptr, nullptr);
		temp.load(node);
		auto x = temp.getX();
		auto y = temp.getY();

		for (auto f : _facilities)
		{
			if (f->getX() == x && f->getY() == y)
			{
				return f;
			}
		}
	}
	return nullptr;
}
/**
 *  Save facilitie reference.
 */
YAML::Node Base::saveFacilitieReference(BaseFacility* fac) const
{
	YAML::Node n;
	if (fac)
	{
		// we need only x and y but we save copy of all data for debuging
		n = fac->save();
	}
	return n;
}

/**
 * Returns the list of soldiers in the base.
 * @return Pointer to the soldier list.
 */
std::vector<Soldier*> *Base::getSoldiers()
{
	return &_soldiers;
}

/**
 * Returns the list of crafts in the base.
 * @return Pointer to the craft list.
 */
std::vector<Craft*> *Base::getCrafts()
{
	return &_crafts;
}

/**
 * Returns the list of crafts in the base.
 * @return Pointer to the craft list.
 */
const std::vector<Craft*> *Base::getCrafts() const
{
	return &_crafts;
}

/**
 * Returns the list of transfers destined
 * to this base.
 * @return Pointer to the transfer list.
 */
std::vector<Transfer*> *Base::getTransfers()
{
	return &_transfers;
}

/**
 * Returns the list of transfers destined
 * to this base.
 * @return Pointer to the transfer list.
 */
const std::vector<Transfer*> *Base::getTransfers() const
{
	return &_transfers;
}

/**
 * Returns the list of items in the base storage rooms.
 * Does NOT return items assigned to craft or in transfer.
 * @return Pointer to the item list.
 */
ItemContainer *Base::getStorageItems()
{
	return _items;
}

/**
 * Returns the amount of scientists currently in the base.
 * @return Number of scientists.
 */
int Base::getScientists() const
{
	return _scientists;
}

/**
 * Changes the amount of scientists currently in the base.
 * @param scientists Number of scientists.
 */
void Base::setScientists(int scientists)
{
	 _scientists = scientists;
}

/**
 * Returns the amount of engineers currently in the base.
 * @return Number of engineers.
 */
int Base::getEngineers() const
{
	return _engineers;
}

/**
 * Changes the amount of engineers currently in the base.
 * @param engineers Number of engineers.
 */
void Base::setEngineers(int engineers)
{
	 _engineers = engineers;
}

/**
 * Returns if a certain target is covered by the base's
 * radar range, taking in account the range and chance.
 * @param target Pointer to target to compare.
 * @return 0 - not detected, 1 - detected by conventional radar, 2 - detected by hyper-wave decoder.
 */
int Base::detect(Target *target) const
{
	int chance = 0;
	double distance = getDistance(target) * 60.0 * (180.0 / M_PI);
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getRules()->getRadarRange() >= distance && (*i)->getBuildTime() == 0)
		{
			int radarChance = (*i)->getRules()->getRadarChance();
			if ((*i)->getRules()->isHyperwave())
			{
				if (radarChance == 100 || RNG::percent(radarChance))
				{
					return 2;
				}
			}
			else
			{
				chance += radarChance;
			}
		}
	}
	if (chance == 0) return 0;

	Ufo *u = dynamic_cast<Ufo*>(target);
	if (u != 0)
	{
		chance = chance * (100 + u->getVisibility()) / 100;
	}

	return RNG::percent(chance)? 1 : 0;
}

/**
 * Returns if a certain target is inside the base's
 * radar range, taking in account the positions of both.
 * @param target Pointer to target to compare.
 * @return 0 - outside radar range, 1 - inside conventional radar range, 2 - inside hyper-wave decoder range.
 */
int Base::insideRadarRange(Target *target) const
{
	bool insideRange = false;
	double distance = getDistance(target) * 60.0 * (180.0 / M_PI);
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getRules()->getRadarRange() >= distance && (*i)->getBuildTime() == 0)
		{
			if ((*i)->getRules()->isHyperwave())
			{
				return 2;
			}
			insideRange = true;
		}
	}

	return insideRange? 1 : 0;
}

/**
 * Returns the amount of soldiers contained
 * in the base without any assignments.
 * @param checkCombatReadiness does what it says on the tin.
 * @return Number of soldiers.
 */
int Base::getAvailableSoldiers(bool checkCombatReadiness) const
{
	int total = 0;
	for (std::vector<Soldier*>::const_iterator i = _soldiers.begin(); i != _soldiers.end(); ++i)
	{
		if (!checkCombatReadiness && (*i)->getCraft() == 0)
		{
			total++;
		}
		else if (checkCombatReadiness && (((*i)->getCraft() != 0 && (*i)->getCraft()->getStatus() != "STR_OUT") ||
			((*i)->getCraft() == 0 && (*i)->getWoundRecovery() == 0)))
		{
			total++;
		}
	}
	return total;
}

/**
 * Returns the total amount of soldiers contained
 * in the base.
 * @return Number of soldiers.
 */
int Base::getTotalSoldiers() const
{
	size_t total = _soldiers.size();
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_SOLDIER)
		{
			total += (*i)->getQuantity();
		}
	}
	return total;
}

/**
 * Returns the amount of scientists contained
 * in the base without any assignments.
 * @return Number of scientists.
 */
int Base::getAvailableScientists() const
{
	return getScientists();
}

/**
 * Returns the total amount of scientists contained
 * in the base.
 * @return Number of scientists.
 */
int Base::getTotalScientists() const
{
	int total = _scientists;
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_SCIENTIST)
		{
			total += (*i)->getQuantity();
		}
	}
	const std::vector<ResearchProject *> & research (getResearch());
	for (std::vector<ResearchProject *>::const_iterator itResearch = research.begin();
		 itResearch != research.end();
		 ++itResearch)
	{
		total += (*itResearch)->getAssigned();
	}
	return total;
}

/**
 * Returns the amount of engineers contained
 * in the base without any assignments.
 * @return Number of engineers.
 */
int Base::getAvailableEngineers() const
{
	return getEngineers();
}

/**
 * Returns the total amount of engineers contained
 * in the base.
 * @return Number of engineers.
 */
int Base::getTotalEngineers() const
{
	int total = _engineers;
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_ENGINEER)
		{
			total += (*i)->getQuantity();
		}
	}
	for (std::vector<Production *>::const_iterator iter = _productions.begin(); iter != _productions.end(); ++iter)
	{
		total += (*iter)->getAssignedEngineers();
	}
	return total;
}

/**
 * Returns the amount of living quarters used up
 * by personnel in the base.
 * @return Living space.
 */
int Base::getUsedQuarters() const
{
	return getTotalSoldiers() + getTotalScientists() + getTotalEngineers();
}

/**
 * Returns the total amount of living quarters
 * available in the base.
 * @return Living space.
 */
int Base::getAvailableQuarters() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getPersonnel();
		}
	}
	return total;
}

/**
 * Returns the amount of stores used up by equipment in the base,
 * and equipment about to arrive.
 * @return Storage space.
 */
double Base::getUsedStores()
{
	double total = _items->getTotalSize(_mod);
	for (std::vector<Craft*>::const_iterator i = _crafts.begin(); i != _crafts.end(); ++i)
	{
		total += (*i)->getItems()->getTotalSize(_mod);
		for (std::vector<Vehicle*>::const_iterator j = (*i)->getVehicles()->begin(); j != (*i)->getVehicles()->end(); ++j)
		{
			total += (*j)->getRules()->getSize();
		}
	}
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_ITEM)
		{
			total += (*i)->getQuantity() * _mod->getItem((*i)->getItems(), true)->getSize();
		}
		else if ((*i)->getType() == TRANSFER_CRAFT)
		{
			Craft *craft = (*i)->getCraft();
			total += craft->getItems()->getTotalSize(_mod);
		}
	}
	total -= getIgnoredStores();
	return total;
}

/**
 * Checks if the base's stores are overfull.
 *
 * Supplying an offset will add/subtract to the used capacity before performing the check.
 * A positive offset simulates adding items to the stores, whereas a negative offset
 * can be used to check whether sufficient items have been removed to stop the stores overflowing.
 * @param offset Adjusts the used capacity.
 * @return True if the base's stores are over their limit.
 */
bool Base::storesOverfull(double offset)
{
	int capacity = getAvailableStores() * 100;
	double used = (getUsedStores() + offset) * 100;
	return (int)used > capacity;
}

/**
 * Returns the total amount of stores
 * available in the base.
 * @return Storage space.
 */
int Base::getAvailableStores() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getStorage();
		}
	}
	return total;
}

/**
 * Determines space taken up by ammo clips about to rearm craft.
 * @return Ignored storage space.
 */
double Base::getIgnoredStores()
{
	double space = 0;
	for (std::vector<Craft*>::iterator c = getCrafts()->begin(); c != getCrafts()->end(); ++c)
	{
		if ((*c)->getStatus() == "STR_REARMING")
		{
			for (std::vector<CraftWeapon*>::iterator w = (*c)->getWeapons()->begin(); w != (*c)->getWeapons()->end() ; ++w)
			{
				if (*w != 0 && (*w)->isRearming())
				{
					std::string clip = (*w)->getRules()->getClipItem();
					int available = getStorageItems()->getItem(clip);
					if (!clip.empty() && available > 0)
					{
						int clipSize = _mod->getItem(clip, true)->getClipSize();
						int needed = 0;
						if (clipSize > 0)
						{
							needed = ((*w)->getRules()->getAmmoMax() - (*w)->getAmmo()) / clipSize;
						}
						space += std::min(available, needed) * _mod->getItem(clip, true)->getSize();
					}
				}
			}
		}
	}
	return space;
}

/**
 * Returns the amount of laboratories used up
 * by research projects in the base.
 * @return Laboratory space.
 */
int Base::getUsedLaboratories() const
{
	const std::vector<ResearchProject *> & research (getResearch());
	int usedLabSpace = 0;
	for (std::vector<ResearchProject *>::const_iterator itResearch = research.begin();
		 itResearch != research.end();
		 ++itResearch)
	{
		usedLabSpace += (*itResearch)->getAssigned();
	}
	return usedLabSpace;
}

/**
 * Returns the total amount of laboratories
 * available in the base.
 * @return Laboratory space.
 */
int Base::getAvailableLaboratories() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getLaboratories();
		}
	}
	return total;
}

/**
 * Returns the amount of workshops used up
 * by manufacturing projects in the base.
 * @return Storage space.
 */
int Base::getUsedWorkshops() const
{
	int usedWorkShop = 0;
	for (std::vector<Production *>::const_iterator iter = _productions.begin(); iter != _productions.end(); ++iter)
	{
		usedWorkShop += ((*iter)->getAssignedEngineers() + (*iter)->getRules()->getRequiredSpace());
	}
	return usedWorkShop;
}

/**
 * Returns the total amount of workshops
 * available in the base.
 * @return Workshop space.
 */
int Base::getAvailableWorkshops() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getWorkshops();
		}
	}
	return total;
}

/**
 * Returns the amount of hangars used up
 * by crafts in the base.
 * @return Number of hangars.
 */
int Base::getUsedHangars() const
{
	size_t total = _crafts.size();
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_CRAFT)
		{
			total += (*i)->getQuantity();
		}
	}
	for (std::vector<Production*>::const_iterator i = _productions.begin(); i != _productions.end(); ++i)
	{
		if ((*i)->getRules()->getProducedCraft())
		{
			// This should be fixed on the case when (*i)->getInfiniteAmount() == TRUE
			total += ((*i)->getAmountTotal() - (*i)->getAmountProduced());
		}
	}
	return total;
}

/**
 * Returns the total amount of hangars
 * available in the base.
 * @return Number of hangars.
 */
int Base::getAvailableHangars() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getCrafts();
		}
	}
	return total;
}

/**
 *  Allocate all crafts to hangars.
 */
void Base::initHangars()
{
	auto sortValue = [](BaseFacility* a) { return std::make_tuple(a->getX(), a->getY()); };
	std::sort(_facilities.begin(), _facilities.end(), [&](BaseFacility* a, BaseFacility* b){ return sortValue(a) < sortValue(b); });

	HangarAllocation allocation(this);
	allocation.assignAll();
}

/**
 * Get hangar by it number.
 */
bool Base::haveHangarForCrafrType(const RuleCraft* type) const
{
	HangarAllocation allocation(this);
	return allocation.addCraftType(type);
}

/**
 * Add new craft to base.
 */
bool Base::addHangarCraft(Craft* craft)
{
	HangarAllocation::Skip s;
	s.craft = craft;
	HangarAllocation allocation(this, s);
	return allocation.addCraft(craft);
}

/*
 * Add craft transfer to base
 */
bool Base::addHangarCraftTransfer(Transfer* transfer)
{
	HangarAllocation::Skip s;
	s.transfer = transfer;
	HangarAllocation allocation(this, s);
	return allocation.addCraftTransfer(transfer);
}

/**
 * Add craft production to base.
 */
bool Base::addHangarCraftProduction(Production* prod)
{
	HangarAllocation::Skip s;
	s.production = prod;
	HangarAllocation allocation(this, s);
	return allocation.addCraftProduction(prod);
}

/**
 * Can remove hangar from base.
 */
bool Base::isHangarNeeded(const BaseFacility* hangar) const
{
	return false;
}

/**
 * Remove hangar from base.
 */
bool Base::removeHangar(BaseFacility* hangar)
{
	HangarAllocation::Skip s;
	s.hangar = hangar;
	HangarAllocation allocation(this, s);
	allocation.assignAll();

	Collections::deleteIf(_crafts, _crafts.size(),
		[&](Craft* c)
		{
			if (c->getHangar() == hangar)
			{
				c->unload(_mod);
				return true;
			}
			else
			{
				return false;
			}
		}
	);
	Collections::deleteIf(_productions, _productions.size(),
		[&](Production* i)
		{
			if (i->getHangar() == hangar)
			{
				_engineers += i->getAssignedEngineers();
				return true;
			}
			else
			{
				return false;
			}
		}
	);
	Collections::deleteIf(_transfers, _transfers.size(),
		[&](Transfer* i)
		{
			return i->getHangar() == hangar;
		}
	);

	return true;
}

/**
 * Return laboratories space not used by a ResearchProject
 * @return laboratories space not used by a ResearchProject
 */
int Base::getFreeLaboratories() const
{
	return getAvailableLaboratories() - getUsedLaboratories();
}

/**
 * Return workshop space not used by a Production
 * @return workshop space not used by a Production
 */
int Base::getFreeWorkshops() const
{
	return getAvailableWorkshops() - getUsedWorkshops();
}

/**
 * Return psilab space not in use
 * @return psilab space not in use
 */
int Base::getFreePsiLabs() const
{
	return getAvailablePsiLabs() - getUsedPsiLabs();
}

/**
 * Return containment space not in use
 * @return containment space not in use
 */
int Base::getFreeContainment() const
{
	return getAvailableContainment() - getUsedContainment();
}

/**
 * Returns the amount of scientists currently in use.
 * @return Amount of scientists.
 */
int Base::getAllocatedScientists() const
{
	int total = 0;
	const std::vector<ResearchProject *> & research (getResearch());
	for (std::vector<ResearchProject *>::const_iterator itResearch = research.begin();
		 itResearch != research.end();
		 ++itResearch)
	{
		total += (*itResearch)->getAssigned();
	}
	return total;
}

/**
 * Returns the amount of engineers currently in use.
 * @return Amount of engineers.
 */
int Base::getAllocatedEngineers() const
{
	int total = 0;
	for (std::vector<Production *>::const_iterator iter = _productions.begin(); iter != _productions.end(); ++iter)
	{
		total += (*iter)->getAssignedEngineers();
	}
	return total;
}

/**
 * Returns the total defense value of all
 * the facilities in the base.
 * @return Defense value.
 */
int Base::getDefenseValue() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getDefenseValue();
		}
	}
	return total;
}

/**
 * Returns the total amount of short range
 * detection facilities in the base.
 * @return Defense value.
 */
int Base::getShortRangeDetection() const
{
	int total = 0;
	int minRadarRange = _mod->getMinRadarRange();

	if (minRadarRange == 0) return 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getRules()->getRadarRange() == minRadarRange && (*i)->getBuildTime() == 0)
		{
			total++;
		}
	}
	return total;
}

/**
 * Returns the total amount of long range
 * detection facilities in the base.
 * @return Defense value.
 */
int Base::getLongRangeDetection() const
{
	int total = 0;
	int minRadarRange = _mod->getMinRadarRange();

	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getRules()->getRadarRange() > minRadarRange && (*i)->getBuildTime() == 0)
		{
			total++;
		}
	}
	return total;
}

/**
 * Returns the total amount of craft of
 * a certain type stored in the base.
 * @param craft Craft type.
 * @return Number of craft.
 */
int Base::getCraftCount(const RuleCraft *craft) const
{
	int total = 0;
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_CRAFT && (*i)->getCraft()->getRules() == craft)
		{
			total++;
		}
	}
	for (std::vector<Craft*>::const_iterator i = _crafts.begin(); i != _crafts.end(); ++i)
	{
		if ((*i)->getRules() == craft)
		{
			total++;
		}
	}
	return total;
}

/**
 * Gets the base's crafts of a certain type.
 */
int Base::getCraftCountForProduction(const RuleCraft *craft) const
{
	int total = 0;
	for (std::vector<Craft*>::const_iterator i = _crafts.begin(); i != _crafts.end(); ++i)
	{
		if ((*i)->getRules() == craft && (*i)->getStatus() != "STR_OUT")
		{
			total++;
		}
	}
	return total;
}

/**
 * Returns the total amount of monthly costs
 * for maintaining the craft in the base.
 * @return Maintenance costs.
 */
int Base::getCraftMaintenance() const
{
	int total = 0;
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_CRAFT)
		{
			total += (*i)->getCraft()->getRules()->getRentCost();
		}
	}
	for (std::vector<Craft*>::const_iterator i = _crafts.begin(); i != _crafts.end(); ++i)
	{
		total += (*i)->getRules()->getRentCost();
	}
	return total;
}

/**
 * Returns the total amount of soldiers of
 * a certain type stored in the base.
 * @param soldier Soldier type.
 * @return Number of soldiert.
 */
int Base::getSoldierCount(const std::string &soldier) const
{
	int total = 0;
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_SOLDIER && (*i)->getSoldier()->getRules()->getType() == soldier)
		{
			total++;
		}
	}
	for (std::vector<Soldier*>::const_iterator i = _soldiers.begin(); i != _soldiers.end(); ++i)
	{
		if ((*i)->getRules()->getType() == soldier)
		{
			total++;
		}
	}
	return total;
}

/**
 * Returns the total amount of monthly costs
 * for maintaining the personnel in the base.
 * @return Maintenance costs.
 */
int Base::getPersonnelMaintenance() const
{
	int total = 0;
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_SOLDIER)
		{
			total += (*i)->getSoldier()->getRules()->getSalaryCost();
		}
	}
	for (std::vector<Soldier*>::const_iterator i = _soldiers.begin(); i != _soldiers.end(); ++i)
	{
		total += (*i)->getRules()->getSalaryCost();
	}
	total += getTotalEngineers() * _mod->getEngineerCost();
	total += getTotalScientists() * _mod->getScientistCost();
	return total;
}

/**
 * Returns the total amount of monthly costs
 * for maintaining the facilities in the base.
 * @return Maintenance costs.
 */
int Base::getFacilityMaintenance() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getMonthlyCost();
		}
	}
	return total;
}

/**
 * Returns the total amount of all the maintenance
 * monthly costs in the base.
 * @return Maintenance costs.
 */
int Base::getMonthlyMaintenace() const
{
	return getCraftMaintenance() + getPersonnelMaintenance() + getFacilityMaintenance();
}

/**
 * Returns the list of all base's ResearchProject
 * @return list of base's ResearchProject
 */
const std::vector<ResearchProject *> & Base::getResearch() const
{
	return _research;
}

/**
 * Add a new Production to the Base
 * @param p A pointer to a Production
 */
void Base::addProduction (Production * p)
{
	_productions.push_back(p);
}

/**
 * Add A new ResearchProject to Base
 * @param project The project to add
 */
void Base::addResearch(ResearchProject * project)
{
	_research.push_back(project);
}

/**
 * Remove a ResearchProject from base
 * @param project the project to remove
 */
void Base::removeResearch(ResearchProject * project)
{
	_scientists += project->getAssigned();
	if (!project->isFinished())
	{
		const RuleResearch *ruleResearch = project->getRules();
		if (ruleResearch->needItem() && ruleResearch->destroyItem())
		{
			getStorageItems()->addItem(ruleResearch->getName(), 1);
		}
	}

	Collections::deleteIf(_research, 1,
		[&](ResearchProject* r)
		{
			return r == project;
		}
	);
}

/**
 * Remove a Production from the Base
 * @param p A pointer to a Production
 */
void Base::removeProduction(Production* production)
{
	_engineers += production->getAssignedEngineers();

	Collections::deleteIf(_productions, 1,
		[&](Production* r)
		{
			return r == production;
		}
	);
}

/**
 * Get the list of Base Production's
 * @return the list of Base Production's
 */
const std::vector<Production *> & Base::getProductions() const
{
	return _productions;
}


/**
 * Returns whether or not this base
 * is equipped with hyper-wave
 * detection facilities.
 * @return True if the base has hyper-wave detection.
 */
bool Base::getHyperDetection() const
{
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getRules()->isHyperwave() && (*i)->getBuildTime() == 0)
		{
			return true;
		}
	}
	return false;
}

/**
 * Returns the total amount of Psi Lab Space
 * available in the base.
 * @return Psi Lab space.
 */
int Base::getAvailablePsiLabs() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getPsiLaboratories();
		}
	}
	return total;
}

/**
 * Returns the total amount of used
 * Psi Lab Space in the base.
 * @return used Psi Lab space.
 */
int Base::getUsedPsiLabs() const
{
	int total = 0;
	for (std::vector<Soldier*>::const_iterator s = _soldiers.begin(); s != _soldiers.end(); ++s)
	{
		if ((*s)->isInPsiTraining())
		{
			total ++;
		}
	}
	return total;
}

/**
 * Returns the total amount of training space
 * available in the base.
 * @return Training space.
 */
int Base::getAvailableTraining() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getTrainingFacilities();
		}
	}
	return total;
}

/**
 * Returns the total amount of used training space
 * available in the base.
 * @return used training space.
 */
int Base::getUsedTraining() const
{
	int total = 0;
	for (std::vector<Soldier*>::const_iterator s = _soldiers.begin(); s != _soldiers.end(); ++s)
	{
		if ((*s)->isInTraining())
		{
			total ++;
		}
	}
	return total;
}

/**
 * Returns the total amount of used
 * Containment Space in the base.
 * @return Containment Lab space.
 */
int Base::getUsedContainment() const
{
	int total = 0;
	for (std::map<std::string, int>::iterator i = _items->getContents()->begin(); i != _items->getContents()->end(); ++i)
	{
		if (_mod->getItem((i)->first, true)->isAlien())
		{
			total += (i)->second;
		}
	}
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_ITEM)
		{
			if (_mod->getItem((*i)->getItems(), true)->isAlien())
			{
				total += (*i)->getQuantity();
			}
		}
	}
	for (std::vector<ResearchProject*>::const_iterator i = _research.begin(); i != _research.end(); ++i)
	{
		const RuleResearch *projRules = (*i)->getRules();
		if (projRules->needItem() && _mod->getUnit(projRules->getName()))
		{
			++total;
		}
	}
	return total;
}

/**
 * Returns the total amount of Containment Space
 * available in the base.
 * @return Containment Lab space.
 */
int Base::getAvailableContainment() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getAliens();
		}
	}
	return total;
}

/**
 * Returns the base's battlescape status.
 * @return Is the craft on the battlescape?
 */
bool Base::isInBattlescape() const
{
	return _inBattlescape;
}

/**
 * Changes the base's battlescape status.
 * @param inbattle True if it's in battle, False otherwise.
 */
void Base::setInBattlescape(bool inbattle)
{
	_inBattlescape = inbattle;
}

/**
 * Mark the base as a valid alien retaliation target.
 * @param mark Mark (if @c true) or unmark (if @c false) the base.
 */
void Base::setRetaliationTarget(bool mark)
{
	_retaliationTarget = mark;
}

/**
 * Get the base's retaliation status.
 * @return If the base is a valid target for alien retaliation.
 */
bool Base::getRetaliationTarget() const
{
	return _retaliationTarget;
}

/**
 * Functor to check for mind shield capability.
 */
struct isMindShield: public std::unary_function<BaseFacility*, bool>
{
	/// Check isMindShield() for @a facility.
	bool operator()(const BaseFacility *facility) const;
};

/**
 * Only fully operational facilities are checked.
 * @param facility Pointer to the facility to check.
 * @return If @a facility can act as a mind shield.
 */
bool isMindShield::operator()(const BaseFacility *facility) const
{
	if (facility->getBuildTime() != 0)
	{
		// Still building this
		return false;
	}
	return (facility->getRules()->isMindShield());
}

/**
 * Functor to check for completed facilities.
 */
struct isCompleted: public std::unary_function<BaseFacility*, bool>
{
	/// Check isCompleted() for @a facility.
	bool operator()(const BaseFacility *facility) const;
};

/**
 * Facilities are checked for construction completion.
 * @param facility Pointer to the facility to check.
 * @return If @a facility has completed construction.
 */
bool isCompleted::operator()(const BaseFacility *facility) const
{
	return (facility->getBuildTime() == 0);
}

/**
 * Calculate the detection chance of this base.
 * Big bases without mindshields are easier to detect.
 * @param difficulty The savegame difficulty.
 * @return The detection chance.
 */
size_t Base::getDetectionChance() const
{
	size_t mindShields = std::count_if (_facilities.begin(), _facilities.end(), isMindShield());
	size_t completedFacilities = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			completedFacilities += (*i)->getRules()->getSize() * (*i)->getRules()->getSize();
		}
	}
	return ((completedFacilities / 6 + 15) / (mindShields + 1));
}

int Base::getGravShields() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0 && (*i)->getRules()->isGravShield())
		{
			++total;
		}
	}
	return total;
}

void Base::setupDefenses()
{
	_defenses.clear();
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0 && (*i)->getRules()->getDefenseValue())
		{
			_defenses.push_back(*i);
		}
	}

	_vehicles.clear();
	Collections::deleteAll(_vehiclesFromBase);

	// add vehicles that are in the crafts of the base, if it's not out
	for (std::vector<Craft*>::iterator c = getCrafts()->begin(); c != getCrafts()->end(); ++c)
	{
		if ((*c)->getStatus() != "STR_OUT")
		{
			for (std::vector<Vehicle*>::iterator i = (*c)->getVehicles()->begin(); i != (*c)->getVehicles()->end(); ++i)
			{
				_vehicles.push_back(*i);
			}
		}
	}

	// add vehicles left on the base
	for (std::map<std::string, int>::iterator i = _items->getContents()->begin(); i != _items->getContents()->end(); )
	{
		std::string itemId = (i)->first;
		int itemQty = (i)->second;
		RuleItem *rule = _mod->getItem(itemId, true);
		if (rule->getVehicleUnit())
		{
			int size = rule->getVehicleUnit()->getArmor()->getTotalSize();
			if (rule->getPrimaryCompatibleAmmo()->empty()) // so this vehicle does not need ammo
			{
				for (int j = 0; j < itemQty; ++j)
				{
					auto vehicle = new Vehicle(rule, rule->getClipSize(), size);
					_vehicles.push_back(vehicle);
					_vehiclesFromBase.push_back(vehicle);
				}
				_items->removeItem(itemId, itemQty);
			}
			else // so this vehicle needs ammo
			{
				RuleItem *ammo = _mod->getItem(rule->getPrimaryCompatibleAmmo()->front(), true);
				int ammoPerVehicle, clipSize;
				if (ammo->getClipSize() > 0 && rule->getClipSize() > 0)
				{
					clipSize = rule->getClipSize();
					ammoPerVehicle = clipSize / ammo->getClipSize();
				}
				else
				{
					clipSize = ammo->getClipSize();
					ammoPerVehicle = clipSize;
				}
				int baseQty = _items->getItem(ammo->getType()) / ammoPerVehicle;
				if (!baseQty)
				{
					++i;
					continue;
				}
				int canBeAdded = std::min(itemQty, baseQty);
				for (int j=0; j<canBeAdded; ++j)
				{
					auto vehicle = new Vehicle(rule, clipSize, size);
					_vehicles.push_back(vehicle);
					_vehiclesFromBase.push_back(vehicle);
					_items->removeItem(ammo->getType(), ammoPerVehicle);
				}
				_items->removeItem(itemId, canBeAdded);
			}

			i = _items->getContents()->begin(); // we have to start over because iterator is broken because of the removeItem
		}
		else ++i;
	}
}

std::vector<BaseFacility*> *Base::getDefenses()
{
	return &_defenses;
}

/**
 * Returns the list of vehicles currently equipped
 * in the base.
 * @return Pointer to vehicle list.
 */
std::vector<Vehicle*> *Base::getVehicles()
{
	return &_vehicles;
}

/**
 * Destroys all disconnected facilities in the base.
 */
void Base::destroyDisconnectedFacilities()
{
	std::list<std::vector<BaseFacility*>::iterator> disFacs = getDisconnectedFacilities(0);
	for (std::list<std::vector<BaseFacility*>::iterator>::reverse_iterator i = disFacs.rbegin(); i != disFacs.rend(); ++i)
	{
		destroyFacility(*i);
	}
}

/**
 * Gets a sorted list of the facilities(=iterators) NOT connected to the Access Lift.
 * @param remove Facility to ignore (in case of facility dismantling).
 * @return a sorted list of iterators pointing to elements in _facilities.
 */
std::list<std::vector<BaseFacility*>::iterator> Base::getDisconnectedFacilities(BaseFacility *remove)
{
	std::list<std::vector<BaseFacility*>::iterator> result;

	if (remove != 0 && remove->getRules()->isLift())
	{ // Theoretically this is impossible, but sanity check is good :)
		for (std::vector<BaseFacility*>::iterator i = _facilities.begin(); i != _facilities.end(); ++i)
		{
			if ((*i) != remove) result.push_back(i);
		}
		return result;
	}

	std::vector<std::pair<std::vector<BaseFacility*>::iterator, bool>*> facilitiesConnStates;
	std::pair<std::vector<BaseFacility*>::iterator, bool> *grid[BASE_SIZE][BASE_SIZE];
	BaseFacility *lift = 0;

	for (int x = 0; x < BASE_SIZE; ++x)
	{
		for (int y = 0; y < BASE_SIZE; ++y)
		{
			grid[x][y] = 0;
		}
	}

	// Ok, fill up the grid(+facilitiesConnStates), and search the lift
	for (std::vector<BaseFacility*>::iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i) != remove)
		{
			if ((*i)->getRules()->isLift()) lift = (*i);
			for (int x = 0; x != (*i)->getRules()->getSize(); ++x)
			{
				for (int y = 0; y != (*i)->getRules()->getSize(); ++y)
				{
					std::pair<std::vector<BaseFacility*>::iterator, bool> *p = new std::pair<std::vector<BaseFacility*>::iterator, bool>(i,false);
					facilitiesConnStates.push_back(p);
					grid[(*i)->getX() + x][(*i)->getY() + y] = p;
				}
			}
		}
	}

	// we're in real trouble if this happens...
	if (lift == 0)
	{
		//TODO: something clever.
		return result;
	}

	// Now make the recursion manually using a stack
	std::stack<std::pair<int, int> > stack;
	stack.push(std::make_pair(lift->getX(),lift->getY()));
	while (!stack.empty())
	{
		int x = stack.top().first, y = stack.top().second;
		stack.pop();
		if (x >= 0 && x < BASE_SIZE && y >= 0 && y < BASE_SIZE && grid[x][y] != 0 && !grid[x][y]->second)
		{
			grid[x][y]->second = true;
			BaseFacility *fac = *(grid[x][y]->first);
			BaseFacility *neighborLeft = (x-1 >= 0 && grid[x-1][y] != 0) ? *(grid[x-1][y]->first) : 0;
			BaseFacility *neighborRight = (x+1 < BASE_SIZE && grid[x+1][y] != 0) ? *(grid[x+1][y]->first) : 0;
			BaseFacility *neighborTop = (y-1 >= 0 && grid[x][y-1] != 0) ? *(grid[x][y-1]->first) : 0;
			BaseFacility *neighborBottom= (y+1 < BASE_SIZE && grid[x][y+1] != 0) ? *(grid[x][y+1]->first) : 0;
			if ((fac->getBuildTime() == 0) || (neighborLeft != 0 && (neighborLeft == fac || neighborLeft->getBuildTime() > neighborLeft->getRules()->getBuildTime()))) stack.push(std::make_pair(x-1,y));
			if ((fac->getBuildTime() == 0) || (neighborRight != 0 && (neighborRight == fac || neighborRight->getBuildTime() > neighborRight->getRules()->getBuildTime()))) stack.push(std::make_pair(x+1,y));
			if ((fac->getBuildTime() == 0) || (neighborTop != 0 && (neighborTop == fac || neighborTop->getBuildTime() > neighborTop->getRules()->getBuildTime()))) stack.push(std::make_pair(x,y-1));
			if ((fac->getBuildTime() == 0) || (neighborBottom != 0 && (neighborBottom == fac || neighborBottom->getBuildTime() > neighborBottom->getRules()->getBuildTime()))) stack.push(std::make_pair(x,y+1));
		}
	}

	BaseFacility *lastFacility = 0;
	for (std::vector<std::pair<std::vector<BaseFacility*>::iterator, bool>*>::iterator i = facilitiesConnStates.begin(); i != facilitiesConnStates.end(); ++i)
	{
		// Not a connected fac.? -> push its iterator into the list!
		// Oh, and we don't want duplicates (facilities with bigger sizes like hangar)
		if (*((*i)->first) != lastFacility && !(*i)->second) result.push_back((*i)->first);
		lastFacility = *((*i)->first);
		delete *i; // We don't need the pair anymore.
	}

	return result;
}

/**
 * Removes a base module, and deals with the ramifications thereof.
 * @param facility An iterator reference to the facility to destroy and remove.
 */
void Base::destroyFacility(std::vector<BaseFacility*>::iterator facility)
{
	if ((*facility)->getRules()->getCrafts() > 0)
	{
		// hangar destruction - destroy crafts and any production of crafts
		// if this will mean there is no hangar to contain it
		removeHangar((*facility));
	}
	if ((*facility)->getRules()->getPsiLaboratories() > 0)
	{
		// psi lab destruction: remove any soldiers over the maximum allowable from psi training.
		int toRemove = (*facility)->getRules()->getPsiLaboratories() - getFreePsiLabs();
		for (std::vector<Soldier*>::iterator i = _soldiers.begin(); i != _soldiers.end() && toRemove > 0; ++i)
		{
			if ((*i)->isInPsiTraining())
			{
				(*i)->setPsiTraining();
				--toRemove;
			}
		}
	}
	if ((*facility)->getRules()->getLaboratories())
	{
		// lab destruction: enforce lab space limits. take scientists off projects until
		// it all evens out. research is not cancelled.
		int toRemove = (*facility)->getRules()->getLaboratories() - getFreeLaboratories();
		for (std::vector<ResearchProject*>::iterator i = _research.begin(); i != _research.end() && toRemove > 0;)
		{
			if ((*i)->getAssigned() >= toRemove)
			{
				(*i)->setAssigned((*i)->getAssigned() - toRemove);
				_scientists += toRemove;
				break;
			}
			else
			{
				toRemove -= (*i)->getAssigned();
				_scientists += (*i)->getAssigned();
				(*i)->setAssigned(0);
				++i;
			}
		}
	}
	if ((*facility)->getRules()->getWorkshops())
	{
		// workshop destruction: similar to lab destruction, but we'll lay off engineers instead
		// in this case, however, production IS cancelled, as it takes up space in the workshop.
		int toRemove = (*facility)->getRules()->getWorkshops() - getFreeWorkshops();
		Collections::deleteIf(_productions, _productions.size(),
			[&](Production* p)
			{
				if (p->getAssignedEngineers() > toRemove)
				{
					p->setAssignedEngineers(p->getAssignedEngineers() - toRemove);
					_engineers += toRemove;
					toRemove = 0;
					return false;
				}
				else
				{
					_engineers += p->getAssignedEngineers();
					toRemove -= p->getAssignedEngineers();
					return true;
				}
			}
		);
	}
	if ((*facility)->getRules()->getStorage())
	{
		// we won't destroy the items physically AT the base,
		// but any items in transit will end up at the dead letter office.
		if (storesOverfull((*facility)->getRules()->getStorage()))
		{
			Collections::deleteIf(_transfers, _transfers.size(),
				[&](Transfer* i)
				{
					if (i->getType() == TRANSFER_ITEM)
					{
						return true;
					}
					else
					{
						return false;
					}
				}
			);
		}
	}
	if ((*facility)->getRules()->getPersonnel())
	{
		// as above, we won't actually fire people, but we'll block any new ones coming in.
		if ((getAvailableQuarters() - getUsedQuarters()) - (*facility)->getRules()->getPersonnel() < 0)
		{
			Collections::deleteIf(_transfers, _transfers.size(),
				[&](Transfer* i)
				{
					if (i->getType() == TRANSFER_ENGINEER || i->getType() == TRANSFER_SCIENTIST)
					{
						return true;
					}
					else
					{
						return false;
					}
				}
			);
		}
	}
	delete *facility;
	_facilities.erase(facility);
}

/**
 * Cleans up the defenses vector and optionally reclaims the tanks and their ammo.
 * @param reclaimItems determines whether the HWPs should be returned to storage.
 */
void Base::cleanupDefenses(bool reclaimItems)
{
	Collections::removeAll(_defenses);

	if (reclaimItems)
	{
		for (auto v : _vehiclesFromBase)
		{
			RuleItem *rule = v->getRules();
			std::string type = rule->getType();
			_items->addItem(type);
			if (!rule->getPrimaryCompatibleAmmo()->empty())
			{
				RuleItem *ammo = _mod->getItem(rule->getPrimaryCompatibleAmmo()->front(), true);
				int ammoPerVehicle;
				if (ammo->getClipSize() > 0 && rule->getClipSize() > 0)
				{
					ammoPerVehicle = rule->getClipSize() / ammo->getClipSize();
				}
				else
				{
					ammoPerVehicle = ammo->getClipSize();
				}
				_items->addItem(ammo->getType(), ammoPerVehicle);
			}
		}
	}

	Collections::removeAll(_vehicles);
	Collections::deleteAll(_vehiclesFromBase);
}

namespace
{

/**
 * Store unique values from diffrent vectors.
 * @param result Vector where final data will be send.
 * @param temp Temporaly data container storing working buffer.
 * @param data Data to add.
 */
void aggregateUnique(std::vector<std::string> &result, std::vector<std::string> &temp, const std::vector<std::string> &data)
{
	temp.clear();

	std::set_union(std::make_move_iterator(std::begin(result)), std::make_move_iterator(std::end(result)), std::begin(data), std::end(data), std::back_inserter(temp));

	std::swap(result, temp);
}

}

/**
 * Return list of all provided functionality in base.
 * @param skip Skip functions provide by this facility.
 * @return List of custom IDs.
 */
std::vector<std::string> Base::getProvidedBaseFunc(const BaseFacility *skip) const
{
	std::vector<std::string> ret, temp;

	for (std::vector<BaseFacility*>::const_iterator bf = _facilities.begin(); bf != _facilities.end(); ++bf)
	{
		if (*bf == skip)
		{
			continue;
		}
		if ((*bf)->getBuildTime() > 0)
		{
			continue;
		}
		aggregateUnique(ret, temp, (*bf)->getRules()->getProvidedBaseFunc());
	}

	return ret;
}

/**
 * Return list of all required functionality in base.
 * @param skip Skip functions require by this facility.
 * @return List of custom IDs.
 */
std::vector<std::string> Base::getRequireBaseFunc(const BaseFacility *skip) const
{
	std::vector<std::string> ret, temp;

	for (std::vector<BaseFacility*>::const_iterator bf = _facilities.begin(); bf != _facilities.end(); ++bf)
	{
		if (*bf == skip)
		{
			continue;
		}
		aggregateUnique(ret, temp,  (*bf)->getRules()->getRequireBaseFunc());
	}

	for (std::vector<ResearchProject*>::const_iterator res = _research.begin(); res != _research.end(); ++res)
	{
		aggregateUnique(ret, temp, (*res)->getRules()->getRequireBaseFunc());
	}

	for (std::vector<Production*>::const_iterator prod = _productions.begin(); prod != _productions.end(); ++prod)
	{
		aggregateUnique(ret, temp, (*prod)->getRules()->getRequireBaseFunc());
	}

	return ret;
}

/**
 * Return list of all forbiden functionality in base.
 * @return List of custom IDs.
 */
std::vector<std::string> Base::getForbiddenBaseFunc() const
{
	std::vector<std::string> ret, temp;

	for (std::vector<BaseFacility*>::const_iterator bf = _facilities.begin(); bf != _facilities.end(); ++bf)
	{
		aggregateUnique(ret, temp, (*bf)->getRules()->getForbiddenBaseFunc());
	}

	return ret;
}

/**
 * Return list of all future provided functionality in base.
 * @return List of custom IDs.
 */
std::vector<std::string> Base::getFutureBaseFunc() const
{
	std::vector<std::string> ret, temp;

	for (std::vector<BaseFacility*>::const_iterator bf = _facilities.begin(); bf != _facilities.end(); ++bf)
	{
		aggregateUnique(ret, temp, (*bf)->getRules()->getProvidedBaseFunc());
	}

	return ret;
}

}
