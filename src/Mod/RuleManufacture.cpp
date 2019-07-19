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
#include <algorithm>
#include "RuleManufacture.h"
#include "RuleResearch.h"
#include "RuleCraft.h"
#include "RuleItem.h"
#include "../Engine/Collections.h"

namespace OpenXcom
{
/**
 * Creates a new Manufacture.
 * @param name The unique manufacture name.
 */
RuleManufacture::RuleManufacture(const std::string &name) : _name(name), _space(0), _time(0), _cost(0), _refund(false), _producedCraft(0), _listOrder(0)
{
	_producedItemsNames[name] = 1;
}

/**
 * Loads the manufacture project from a YAML file.
 * @param node YAML node.
 * @param listOrder The list weight for this manufacture.
 */
void RuleManufacture::load(const YAML::Node &node, int listOrder)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, listOrder);
	}
	bool same = (1 == _producedItemsNames.size() && _name == _producedItemsNames.begin()->first);
	_name = node["name"].as<std::string>(_name);
	if (same)
	{
		int value = _producedItemsNames.begin()->second;
		_producedItemsNames.clear();
		_producedItemsNames[_name] = value;
	}
	_category = node["category"].as<std::string>(_category);
	_requiresName = node["requires"].as< std::vector<std::string> >(_requiresName);
	_requiresBaseFunc = node["requiresBaseFunc"].as< std::vector<std::string> >(_requiresBaseFunc);
	_space = node["space"].as<int>(_space);
	_time = node["time"].as<int>(_time);
	_cost = node["cost"].as<int>(_cost);
	_refund = node["refund"].as<bool>(_refund);
	_requiredItemsNames = node["requiredItems"].as< std::map<std::string, int> >(_requiredItemsNames);
	_producedItemsNames = node["producedItems"].as< std::map<std::string, int> >(_producedItemsNames);
	_randomProducedItemsNames = node["randomProducedItems"].as< std::vector<std::pair<int, std::map<std::string, int> > > >(_randomProducedItemsNames);
	_spawnedPersonType = node["spawnedPersonType"].as<std::string>(_spawnedPersonType);
	_spawnedPersonName = node["spawnedPersonName"].as<std::string>(_spawnedPersonName);
	_listOrder = node["listOrder"].as<int>(_listOrder);
	if (!_listOrder)
	{
		_listOrder = listOrder;
	}
	std::sort(_requiresBaseFunc.begin(), _requiresBaseFunc.end());
}

/**
 * Cross link with other Rules.
 */
void RuleManufacture::afterLoad(const Mod* mod)
{
	_requires = mod->getResearch(_requiresName);
	if (_category == "STR_CRAFT")
	{
		auto item = _producedItemsNames.begin();
		if (item == _producedItemsNames.end())
		{
			throw Exception("No craft defined for production");
		}
		else if (item->second != 1)
		{
			throw Exception("Only one craft can be build in production");
		}
		else
		{
			_producedCraft = mod->getCraft(item->first, true);
		}
	}
	else
	{
		for (auto& i : _producedItemsNames)
		{
			_producedItems[mod->getItem(i.first, true)] = i.second;
		}
	}
	for (auto& i : _requiredItemsNames)
	{
		auto itemRule = mod->getItem(i.first, false);
		auto craftRule = mod->getCraft(i.first, false);
		if (itemRule)
		{
			_requiredItems[itemRule] = i.second;
		}
		else if (craftRule)
		{
			_requiredCrafts[craftRule] = i.second;
		}
		else
		{
			throw Exception("Unknow required item '" + i.first + "'");
		}
	}

	for (auto& itemSet : _randomProducedItemsNames)
	{
		std::map<const RuleItem*, int> tmp;
		for (auto& i : itemSet.second)
		{
			tmp[mod->getItem(i.first, true)] = i.second;
		}
		_randomProducedItems.push_back(std::make_pair(itemSet.first, tmp));
	}

	//remove not needed data
	Collections::removeAll(_requiresName);
	Collections::removeAll(_producedItemsNames);
	Collections::removeAll(_requiredItemsNames);
	Collections::removeAll(_randomProducedItemsNames);
}

/**
 * Gets the unique name of the manufacture.
 * @return The name.
 */
const std::string &RuleManufacture::getName() const
{
	return _name;
}

/**
 * Gets the category shown in the manufacture list.
 * @return The category.
 */
const std::string &RuleManufacture::getCategory() const
{
	return _category;
}

/**
 * Gets the list of research required to
 * manufacture this object.
 * @return A list of research IDs.
 */
const std::vector<const RuleResearch*> &RuleManufacture::getRequirements() const
{
	return _requires;
}

/**
 * Gets the list of base functions required to
 * manufacture this object.
 * @return A list of functions IDs.
 */
const std::vector<std::string> &RuleManufacture::getRequireBaseFunc() const
{
	return _requiresBaseFunc;
}

/**
 * Gets the required workspace to start production.
 * @return The required workspace.
 */
int RuleManufacture::getRequiredSpace() const
{
	return _space;
}

/**
 * Gets the time needed to manufacture one object.
 * @return The time needed to manufacture one object (in man/hour).
 */
int RuleManufacture::getManufactureTime() const
{
	return _time;
}


/**
 * Gets the cost of manufacturing one object.
 * @return The cost of manufacturing one object.
 */
int RuleManufacture::getManufactureCost() const
{
	return _cost;
}

/**
 * Should all resources of a cancelled project be refunded?
 * @return True, if all resources should be refunded. False otherwise.
 */
bool RuleManufacture::getRefund() const
{
	return _refund;
}

/**
 * Gets the list of items required to manufacture one object.
 * @return The list of items required to manufacture one object.
 */
const std::map<const RuleItem*, int> &RuleManufacture::getRequiredItems() const
{
	return _requiredItems;
}

/**
 * Gets the list of crafts required to manufacture one object.
 */
const std::map<const RuleCraft*, int> &RuleManufacture::getRequiredCrafts() const
{
	return _requiredCrafts;
}

/**
 * Gets the list of items produced by completing "one object" of this project.
 * @return The list of items produced by completing "one object" of this project.
 */
const std::map<const RuleItem*, int> &RuleManufacture::getProducedItems() const
{
	return _producedItems;
}

/*
 * Gets craft build by this project. null if is not craft production.
 * @return Craft rule set.
 */
const RuleCraft* RuleManufacture::getProducedCraft() const
{
	return _producedCraft;
}

/**
 * Gets the random manufacture rules.
 * @return The random manufacture rules.
 */
const std::vector<std::pair<int, std::map<const RuleItem*, int> > > &RuleManufacture::getRandomProducedItems() const
{
	return _randomProducedItems;
}

/**
* Gets the "manufactured person", i.e. person spawned when manufacturing project ends.
* @return The person type ID.
*/
const std::string &RuleManufacture::getSpawnedPersonType() const
{
	return _spawnedPersonType;
}

/**
* Gets the custom name of the "manufactured person".
* @return The person name translation code.
*/
const std::string &RuleManufacture::getSpawnedPersonName() const
{
	return _spawnedPersonName;
}

/**
 * Is it possible to use auto-sell feature for this manufacturing project?
 * @return True for normal projects, false for special projects.
 */
bool RuleManufacture::canAutoSell() const
{
	if (!_spawnedPersonType.empty())
	{
		// can't sell people
		return false;
	}
	if (!_randomProducedItems.empty())
	{
		// sell profit label can't show reasonable info
		return false;
	}
	return true;
}

/**
 * Gets the list weight for this manufacture item.
 * @return The list weight for this manufacture item.
 */
int RuleManufacture::getListOrder() const
{
	return _listOrder;
}

}
