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
#include <string>
#include <vector>
#include <map>
#include <yaml-cpp/yaml.h>
#include "MapScript.h"

namespace OpenXcom
{

class Mod;
class RuleCraft;

/**
 * Represents a specific type of base facility.
 * Contains constant info about a facility like
 * costs, capacities, size, etc.
 * @sa BaseFacility
 */
class RuleBaseFacility
{
private:
	std::string _type;
	std::vector<std::string> _requires, _requiresBaseFunc, _provideBaseFunc, _forbiddenBaseFunc;
	int _spriteShape, _spriteFacility;
	bool _lift, _hyper, _mind, _grav;
	int _size, _buildCost, _refundValue, _buildTime, _monthlyCost;
	std::map<std::string, std::pair<int, int> > _buildCostItems;
	int _storage, _personnel, _aliens, _crafts, _labs, _workshops, _psiLabs;
	int _radarRange, _radarChance, _defense, _hitRatio, _fireSound, _hitSound;
	std::string _mapName;
	int _listOrder, _trainingRooms;
	std::vector<VerticalLevel> _verticalLevels;
public:
	/// Creates a blank facility ruleset.
	RuleBaseFacility(const std::string &type);
	/// Cleans up the facility ruleset.
	~RuleBaseFacility();
	/// Loads the facility from YAML.
	void load(const YAML::Node& node, Mod *mod, int listOrder);
	/// Gets the facility's type.
	std::string getType() const;
	/// Gets the facility's requirements.
	const std::vector<std::string> &getRequirements() const;
	/// Gets the facility's required function in base to build.
	const std::vector<std::string> &getRequireBaseFunc() const;
	/// Gets the functions that facility provide in base.
	const std::vector<std::string> &getProvidedBaseFunc() const;
	/// Gets the functions that facility prevent in base.
	const std::vector<std::string> &getForbiddenBaseFunc() const;
	/// Gets the facility's shape sprite.
	int getSpriteShape() const;
	/// Gets the facility's content sprite.
	int getSpriteFacility() const;
	/// Gets the facility's size.
	int getSize() const;
	/// Gets if the facility is an access lift.
	bool isLift() const;
	/// Gets if the facility has hyperwave detection.
	bool isHyperwave() const;
	/// Gets if the facility is a mind shield.
	bool isMindShield() const;
	/// Gets if the facility is a grav shield.
	bool isGravShield() const;
	/// Gets the facility's construction cost.
	int getBuildCost() const;
	/// Gets the facility's refund value.
	int getRefundValue() const;
	/// Gets the facility's construction cost in items, `first` is build cost, `second` is refund.
	const std::map<std::string, std::pair<int, int> >& getBuildCostItems() const;
	/// Gets the facility's construction time.
	int getBuildTime() const;
	/// Gets the facility's monthly cost.
	int getMonthlyCost() const;
	/// Gets the facility's storage capacity.
	int getStorage() const;
	/// Gets the facility's personnel capacity.
	int getPersonnel() const;
	/// Gets the facility's alien capacity.
	int getAliens() const;
	/// Gets the facility's craft capacity.
	int getCrafts() const;
	/// Gets if given craft type can be stroed in hangar.
	bool isCraftTypeAllowed(const RuleCraft* craft) const;
	/// Gets the facility's laboratory space.
	int getLaboratories() const;
	/// Gets the facility's workshop space.
	int getWorkshops() const;
	/// Gets the facility's psi-training capacity.
	int getPsiLaboratories() const;
	/// Gets the facility's radar range.
	int getRadarRange() const;
	/// Gets the facility's detection chance.
	int getRadarChance() const;
	/// Gets the facility's defense value.
	int getDefenseValue() const;
	/// Gets the facility's weapon hit ratio.
	int getHitRatio() const;
	/// Gets the facility's battlescape map name.
	std::string getMapName() const;
	/// Gets the facility's fire sound.
	int getFireSound() const;
	/// Gets the facility's hit sound.
	int getHitSound() const;
	/// Gets the facility's list weight.
	int getListOrder() const;
	/// Gets the facility's training capacity.
	int getTrainingFacilities() const;
	/// Gets the vertical levels for this facility map generation.
	const std::vector<VerticalLevel> &getVerticalLevels() const;
};

}
