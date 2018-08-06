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

#include <SDL_stdinc.h>
#include <cassert>
#include <limits.h>

namespace OpenXcom
{

class RuleCraft;
class Craft;
class Mod;
class Transfer;
class Production;
class Base;
class BaseFacility;

/**
 *
 */
class HangarAllocation
{
public:
	enum class CraftPos : unsigned { CraftInvaild = (unsigned)-1, CraftStart = 0, };
	enum class HangarPos : unsigned { HangarInvaild = (unsigned)-1, HangarStart = 0, };

	struct Skip
	{
		const BaseFacility* hangar = nullptr;
		const Transfer* transfer = nullptr;
		const Craft* craft = nullptr;
		const Production* production = nullptr;

		Skip(){ }
	};

private:
	using UseMaskType = Uint16;
	static constexpr size_t MaxCraftSlots = CHAR_BIT * sizeof(UseMaskType);
	static constexpr size_t MaxHangarsSlots = CHAR_BIT * sizeof(UseMaskType);

	UseMaskType _useArrayCraft[MaxCraftSlots] = { };
	UseMaskType _useArrayHangar[MaxHangarsSlots] = { };
	UseMaskType _useArrayHangarDefault[MaxHangarsSlots] = { };

	Uint8 _avaiableCraftsMatchs[MaxCraftSlots] = { };
	const RuleCraft* _avaiableCraftTypes[MaxCraftSlots] = { };
	Craft* _avaiableCrafts[MaxCraftSlots] = { };
	Transfer* _avaiableTransfers[MaxCraftSlots] = { };
	Production* _avaiableProductions[MaxCraftSlots] = { };
	CraftPos _currCraftMax = CraftPos::CraftStart;
	size_t _currCraftNum = 0u;

	Uint8 _avaiableHangarsMatchs[MaxHangarsSlots] = { };
	BaseFacility* _avaiableHangars[MaxHangarsSlots] = { };
	HangarPos _currHangarMax = HangarPos::HangarStart;

	const Base* _base = nullptr;

	/// Get curent possible usage of craft for hangar.
	bool getUse(CraftPos craft, HangarPos hangar) const;
	/// Set curent possible usage of craft for hangar.
	void setUse(CraftPos craft, HangarPos hangar, bool set);

	Uint8 getCraftMatchsNum(CraftPos craft) const;
	Uint8 getHangarMatchsNum(HangarPos hangar) const;

	UseMaskType getCraftMatchs(CraftPos craft) const;
	UseMaskType getHangarsMatchs(HangarPos hangar) const;

	BaseFacility* getCurrCraftHangar(CraftPos c) const;
	void setCurrCraftHangar(CraftPos c, BaseFacility* hangar);

	BaseFacility* getHangar(HangarPos hangar) const;

	/// Init flag of avaiabli of craft x hangar.
	void initUse(CraftPos craft, HangarPos hangar);
	/// Reset to starting state.
	void resetUse(CraftPos craft, HangarPos hangar);
	/// Reset all connetions to match prevoius ones.
	void resetAllByPrevoiusMatch();
	/// Reset all connections to max possible ones.
	void resetAll();

	/// Init craft info.
	CraftPos initCraft(Craft* craft, Transfer* transfer, Production* production, const RuleCraft* rule);
	/// Init hangar info facility.
	HangarPos initHangar(BaseFacility* hangar);
	/// Check space for next craft.
	BaseFacility* getFreeHangar(const RuleCraft* rule);
	/// Run code and allocate all crafts to hangars.
	bool craftAllocation();

public:
	/// Constructor.
	HangarAllocation(const Base *base, const Skip s = {});
	/// Destructor.
	~HangarAllocation();

	/// Add new craft type to base.
	bool addCraftType(const RuleCraft* rule);
	/// Remove craft type from base.
	void removeCraftType(const RuleCraft* rule);

	/// Add new craft to base.
	bool addCraft(Craft* craft);
	/// Add craft transfer to base
	bool addCraftTransfer(Transfer* transfer);
	/// Add craft production to base.
	bool addCraftProduction(Production* prod);

	/// Assign crafts, transfers and productuions to avaiable hangars
	void assignAll();
};

}
