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
#include <cmath>
#include <algorithm>
#include <numeric>
#include "HangarAllocation.h"
#include "../Engine/Collections.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Base.h"
#include "../Savegame/Production.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Transfer.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleManufacture.h"
#include "../Mod/RuleBaseFacility.h"


namespace OpenXcom
{

inline unsigned operator+(HangarAllocation::CraftPos pos)
{
	return static_cast<unsigned>(pos);
}

inline unsigned operator+(HangarAllocation::HangarPos pos)
{
	return static_cast<unsigned>(pos);
}

inline HangarAllocation::CraftPos& operator++(HangarAllocation::CraftPos& pos)
{
	pos = static_cast<HangarAllocation::CraftPos>((+pos) + 1);
	return pos;
}

inline HangarAllocation::HangarPos& operator++(HangarAllocation::HangarPos& pos)
{
	pos = static_cast<HangarAllocation::HangarPos>((+pos) + 1);
	return pos;
}

inline HangarAllocation::CraftPos& operator--(HangarAllocation::CraftPos& pos)
{
	pos = static_cast<HangarAllocation::CraftPos>((+pos) - 1);
	return pos;
}

inline HangarAllocation::HangarPos& operator--(HangarAllocation::HangarPos& pos)
{
	pos = static_cast<HangarAllocation::HangarPos>((+pos) - 1);
	return pos;
}

inline bool operator<(HangarAllocation::CraftPos a, HangarAllocation::CraftPos b)
{
	return +a < +b;
}

inline bool operator<(HangarAllocation::HangarPos a, HangarAllocation::HangarPos b)
{
	return +a < +b;
}

inline unsigned operator<<(unsigned i, HangarAllocation::CraftPos pos)
{
	return i << (+pos);
}

inline unsigned operator<<(unsigned i, HangarAllocation::HangarPos pos)
{
	return i << (+pos);
}

/**
 * Initializes a craft weapon of the specified type.
 * @param rules Pointer to ruleset.
 * @param ammo Initial ammo.
 */
HangarAllocation::HangarAllocation(const Base *base, const Skip s) : _base(base)
{
	// load of data
	for (auto craft : *_base->getCrafts())
	{
		if (s.craft != craft && initCraft(craft, nullptr, nullptr, craft->getRules()) != CraftPos::CraftInvaild)
		{
			break;
		}
	}

	for (auto prod : _base->getProductions())
	{
		auto rule = prod->getRules()->getProducedCraft();
		if (s.production != prod && rule && initCraft(nullptr, nullptr, prod, rule) != CraftPos::CraftInvaild)
		{
			break;
		}
	}

	for (auto trans : *_base->getTransfers())
	{
		auto craft = trans->getCraft();
		if (s.transfer != trans && craft && initCraft(nullptr, trans, nullptr, craft->getRules()) != CraftPos::CraftInvaild)
		{
			break;
		}
	}

	using FacPair = std::pair<BaseFacility*, int>;
	auto facPairLess = [](FacPair a, FacPair b){ return a.second < b.second; };

	FacPair tempFaci[MaxHangarsSlots] = { };

	auto outputRange = Collections::range(tempFaci);
	auto inputRange = Collections::filter(
		Collections::range(*_base->getFacilities()),
		[&](BaseFacility* fac)
		{
			return s.hangar != fac && fac->getBuildTime() == 0 && fac->getRules()->getCrafts();
		}
	);

	auto begin = outputRange.begin();
	auto end = begin;
	for (auto zip : Collections::nonDeref(Collections::zip(outputRange, inputRange)))
	{
		auto in = *zip.second;
		*zip.first = FacPair(in, in->getRules()->getCrafts());
		end = zip.first + 1;
	}

	[&] //loop return scope
	{
		std::sort(begin, end, facPairLess);
		for (auto hangarSize : Collections::rangeValueLess(MaxHangarsSlots))
		{
			auto lowerLimit = std::upper_bound(begin, end, FacPair(nullptr, hangarSize), facPairLess);
			if (lowerLimit == end)
			{
				return;
			}
			for (FacPair pair : Collections::range(lowerLimit, end))
			{
				if (initHangar(pair.first) == HangarPos::HangarInvaild)
				{
					return;
				}
			}
		}
	}();
}

/**
 * Destructor.
 */
HangarAllocation::~HangarAllocation()
{
}


bool HangarAllocation::getUse(CraftPos craft, HangarPos hangar) const
{
	assert(craft != CraftPos::CraftInvaild);
	assert(hangar != HangarPos::HangarInvaild);
	assert(+craft < MaxCraftSlots);
	assert(+hangar < MaxHangarsSlots);
	return _useArrayHangar[+hangar] & (1 << craft);
}

void HangarAllocation::setUse(CraftPos craft, HangarPos hangar, bool set)
{
	auto currentUse = getUse(craft, hangar);
	if (!currentUse && set)
	{
		++_avaiableCraftsMatchs[+craft];
		++_avaiableHangarsMatchs[+hangar];
		_useArrayCraft[+craft] |= (1 << hangar);
		_useArrayHangar[+hangar] |= (1 << craft);
	}
	else if (currentUse && !set)
	{
		--_avaiableCraftsMatchs[+craft];
		--_avaiableHangarsMatchs[+hangar];
		_useArrayCraft[+craft] &= ~(1 << hangar);
		_useArrayHangar[+hangar] &= ~(1 << craft);
	}
}

Uint8 HangarAllocation::getCraftMatchsNum(CraftPos craft) const
{
	return _avaiableCraftsMatchs[+craft];
}

Uint8 HangarAllocation::getHangarMatchsNum(HangarPos hangar) const
{
	return _avaiableHangarsMatchs[+hangar];
}

HangarAllocation::UseMaskType HangarAllocation::getCraftMatchs(CraftPos craft) const
{
	return _useArrayCraft[+craft];
}

HangarAllocation::UseMaskType HangarAllocation::getHangarsMatchs(HangarPos hangar) const
{
	return _useArrayHangar[+hangar];
}

BaseFacility* HangarAllocation::getCurrCraftHangar(CraftPos c) const
{
	if (_avaiableCrafts[+c])
	{
		return _avaiableCrafts[+c]->getHangar();
	}
	else if (_avaiableTransfers[+c])
	{
		return _avaiableTransfers[+c]->getHangar();
	}
	else if (_avaiableProductions[+c])
	{
		return _avaiableProductions[+c]->getHangar();
	}
	else
	{
		return nullptr;
	}
};

void HangarAllocation::setCurrCraftHangar(CraftPos c, BaseFacility* hangar)
{
	if (_avaiableCrafts[+c])
	{
		_avaiableCrafts[+c]->setHangar(hangar);
	}
	else if (_avaiableTransfers[+c])
	{
		_avaiableTransfers[+c]->setHangar(hangar);
	}
	else if (_avaiableProductions[+c])
	{
		_avaiableProductions[+c]->setHangar(hangar);
	}
	else
	{
		//nothing
	}
};

BaseFacility* HangarAllocation::getHangar(HangarPos h) const
{
	return _avaiableHangars[+h];
}

void HangarAllocation::initUse(CraftPos craft, HangarPos hangar)
{
	assert(+craft < MaxCraftSlots);
	assert(+hangar < MaxHangarsSlots);

	bool set = _avaiableCraftTypes[+craft] && _avaiableHangars[+hangar] && _avaiableHangars[+hangar]->getRules()->isCraftTypeAllowed(_avaiableCraftTypes[+craft]);
	if (set)
	{
		_useArrayHangarDefault[+hangar] |= (1 << craft);
	}
	else
	{
		_useArrayHangarDefault[+hangar] &= ~(1 << craft);
	}

	resetUse(craft, hangar);
}

void HangarAllocation::resetUse(CraftPos craft, HangarPos hangar)
{
	setUse(craft, hangar, _useArrayHangarDefault[+hangar] & (1 << craft));
}

/**
 * Reset all connetions to match prevoius ones.
 */
void HangarAllocation::resetAllByPrevoiusMatch()
{
	for (auto c : Collections::rangeValueLess(_currCraftMax))
	{
		auto hangar = getCurrCraftHangar(c);
		if (hangar != nullptr)
		{
			for (auto h : Collections::rangeValueLess(_currHangarMax))
			{
				if (getHangar(h) == hangar)
				{
					for (auto o : Collections::rangeValueLess(_currHangarMax))
					{
						if (o != h)
						{
							setUse(c, o, false);
						}
					}
				}
			}
		}
	}
}

/**
 * Reset all connections to max possible ones.
 */
void HangarAllocation::resetAll()
{
	for (auto c : Collections::rangeValueLess(_currCraftMax))
	{
		for (auto h : Collections::rangeValueLess(_currHangarMax))
		{
			resetUse(c, h);
		}
	}
}

HangarAllocation::CraftPos HangarAllocation::initCraft(Craft* craft, Transfer* transfer, Production* production, const RuleCraft* rule)
{
	if (_currCraftNum != +_currCraftMax)
	{
		for (auto curr : Collections::reverse(Collections::rangeValueLess(_currCraftMax)))
		{
			if (_avaiableCraftTypes[+curr] == nullptr)
			{
				_avaiableCrafts[+curr] = craft;
				_avaiableTransfers[+curr] = transfer;
				_avaiableProductions[+curr] = production;
				_avaiableCraftTypes[+curr] = rule;
				++_currCraftNum;
				return curr;
			}
		}
	}
	else
	{
		if (+_currCraftMax < MaxCraftSlots)
		{
			auto curr = _currCraftMax;
			++_currCraftMax;
			++_currCraftNum;

			_avaiableCrafts[+curr] = craft;
			_avaiableTransfers[+curr] = transfer;
			_avaiableProductions[+curr] = production;
			_avaiableCraftTypes[+curr] = rule;
			return curr;
		}
	}

	return CraftPos::CraftInvaild;
};

HangarAllocation::HangarPos HangarAllocation::initHangar(BaseFacility* hangar)
{
	if (+_currHangarMax < MaxHangarsSlots)
	{
		auto curr = _currHangarMax;
		++_currHangarMax;

		_avaiableHangars[+curr] = hangar;
		for (auto c : Collections::rangeValueLess(_currCraftMax))
		{
			initUse(c, curr);
		}
		return curr;
	}
	return HangarPos::HangarInvaild;
};

bool HangarAllocation::craftAllocation()
{
	//brutal force search, in wrose case `O(n^m)`, used as last step after some reductions
	auto findAllocation = [&](CraftPos* be, CraftPos* en)
	{
		std::sort(be, en, [&](CraftPos a, CraftPos b){ return getCraftMatchsNum(a) < getCraftMatchsNum(b); });

		auto curr = be;
		UseMaskType used = {};
		UseMaskType seleced[MaxCraftSlots] = {};
		int selPos = 0;
		while (true)
		{
			auto first = [](UseMaskType c) { return c & ~(c - 1); }; // (01100 & ~(01100 - 1)) -> (01100 & ~01011) -> 00100
			auto upper = [](UseMaskType c, UseMaskType u) { return (u & ~(c + (c - 1))); }; // (1011010 & ~(0001000 + 0001000 - 1)) -> (1011010 & ~0001111) -> 1010000

			used &= ~seleced[selPos];
			auto n = first(upper(seleced[selPos], getCraftMatchs(*curr) & ~used));
			if (n)
			{
				//finded position
				used |= n;
				seleced[selPos] = n;
				++selPos;
				++curr;
				if (curr == en)
				{
					break;
				}
			}
			else
			{
				if (curr == be)
				{
					//no more postion to backtrack, imposible to match
					return false;
				}
				else
				{
					//can find mach, check if we could match prevous differently
					seleced[selPos] = 0;
					--selPos;
					--curr;
				}
			}
		}

		for (auto p : Collections::zip(Collections::range(be, en), Collections::range(seleced)))
		{
			for (auto h : Collections::rangeValueLess(_currHangarMax))
			{
				if ((1 << h) == p.second)
				{
					setUse(p.first, h, false);
				}
			}
		}

		return true;
	};

	//find all required combinations and remove that are impossible, worst case is `O(m*m)`
	//algorithm is for any `curr`
	//if number of all `j` position that have `_useArrayCraft[j]` is subset of `_useArrayCraft[curr]`
	//is equal to `_avaiableCraftsOptions[curr]` then we can remove `_useArrayCraft[curr]` from any other positions.
	UseMaskType ResultImposible = ~UseMaskType{};
	UseMaskType ResultNone = 0u;
	auto reduceCombinations = [&](CraftPos* be, CraftPos* en)
	{
		std::sort(be, en, [&](CraftPos a, CraftPos b){ return getCraftMatchsNum(a) < getCraftMatchsNum(b); });

		auto currentRange = Collections::range(be, en);
		for (auto curr : currentRange)
		{
			const auto size = getCraftMatchsNum(curr);
			const auto include = getCraftMatchs(curr);
			const auto exclude = ~include;
			auto subsets = 0;
			for (auto j : currentRange)
			{
				const auto js = getCraftMatchsNum(j);
				if (js > size)
				{
					//have more elements, impossible to be subset
					break;
				}
				const auto u = getCraftMatchs(j);
				if ((u & include) && !(u & exclude))
				{
					++subsets;
					if (subsets == size)
					{
						//finded, we can stop serching
						for (auto j : currentRange)
						{
							const auto u = getCraftMatchs(j);
							if (subsets > 0 && (u & include) && !(u & exclude))
							{
								//we do protect usage of positions that include to subsets
								//caveat: `curr` can be outside of this "protection" but this could happens when we can't assign all crafts in first place
								--subsets;
							}
							else if (u & include)
							{
								if (!(u & exclude))
								{
									//imposible to find free hangar, becasue if we remove everything nothig will left
									return ResultImposible;
								}
								for (auto h : Collections::rangeValueLess(_currHangarMax))
								{
									if ((1 << h) & include)
									{
										setUse(j, h, false);
									}
								}
							}
						}
						return static_cast<UseMaskType>(exclude);
					}
				}
			}
		}
		return ResultNone;
	};

	CraftPos offsets[MaxCraftSlots];
	auto begin = std::begin(offsets);
	auto end = std::end(offsets);
	std::iota(begin, end, CraftPos{});

	//skip emply slots
	end = std::partition(begin, end, [&](CraftPos a){ return _avaiableCraftTypes[+a]; });

	auto workToDo = std::accumulate(begin, end, UseMaskType{}, [&](UseMaskType acc, CraftPos a){ return acc | getCraftMatchs(a); });
	while (begin != end)
	{
		auto exclude = reduceCombinations(begin, end);
		if (exclude == ResultImposible)
		{
			return false;
		}

		workToDo &= exclude;
		auto split = std::partition(begin, end, [&](CraftPos a){ return !(getCraftMatchs(a) & workToDo); });
		if (split == begin)
		{
			assert(false && "partition bug in craftAllocation");
		}

		if (!findAllocation(begin, split))
		{
			return false;
		}
		begin = split;
	}

	return true;
}

BaseFacility* HangarAllocation::getFreeHangar(const RuleCraft* rule)
{
	auto pos = initCraft(nullptr, nullptr, nullptr, rule);
	if (pos == CraftPos::CraftInvaild)
	{
		return nullptr;
	}

	//init new craft
	for (auto h : Collections::rangeValueLess(_currHangarMax))
	{
		initUse(pos, h);
	}

	if (getCraftMatchsNum(pos) == 0)
	{
		removeCraftType(rule);
		return nullptr;
	}

	resetAllByPrevoiusMatch();
	if (!craftAllocation())
	{
		resetAll();
		if (!craftAllocation())
		{
			removeCraftType(rule);
			return nullptr;
		}
	}

	//find what hangar exacly have free space
	for (auto h : Collections::rangeValueLess(_currHangarMax))
	{
		if (getUse(pos, h))
		{
			return getHangar(h);
		}
	}

	assert(false && "return bug in getFreeHangar");

	return nullptr;
}




/**
 * Add new craft type to base.
 */
bool HangarAllocation::addCraftType(const RuleCraft* rule)
{
	return getFreeHangar(rule);
}

/**
 * Remove craft type from base.
 */
void HangarAllocation::removeCraftType(const RuleCraft* rule)
{
	for (auto c : Collections::reverse(Collections::rangeValueLess(_currCraftMax)))
	{
		if (_avaiableCraftTypes[+c] == rule)
		{
			--_currCraftNum;
			_avaiableCraftTypes[+c] = nullptr;
			_avaiableCrafts[+c] = nullptr;
			_avaiableProductions[+c] = nullptr;
			_avaiableTransfers[+c] = nullptr;
			for (auto h : Collections::rangeValueLess(_currHangarMax))
			{
				initUse(c, h);
			}
		}
	}

	assert(false && "return bug in removeCraftType");
}

/**
 * Add new craft to base.
 */
bool HangarAllocation::addCraft(Craft* craft)
{
	auto hangar = getFreeHangar(craft->getRules());
	if (hangar)
	{
		craft->setHangar(hangar);
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Add craft transfer to base
 */
bool HangarAllocation::addCraftTransfer(Transfer* transfer)
{
	auto craft = transfer->getCraft();
	if (craft)
	{
		auto hangar = getFreeHangar(craft->getRules());
		if (hangar)
		{
			transfer->setHangar(hangar);
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}

/**
 * Add craft production to base.
 */
bool HangarAllocation::addCraftProduction(Production* prod)
{
	auto craftType = prod->getRules()->getProducedCraft();
	if (craftType)
	{
		auto hangar = getFreeHangar(craftType);
		if (hangar)
		{
			prod->setHangar(hangar);
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}

/**
 * Assign crafts, transfers and  productuions to avaiable hangars
 */
void HangarAllocation::assignAll()
{
	resetAllByPrevoiusMatch();

	craftAllocation();

	for (auto c : Collections::rangeValueLess(_currCraftMax))
	{
		if (getCraftMatchsNum(c) != 1)
		{
			continue;
		}
		for (auto h : Collections::rangeValueLess(_currHangarMax))
		{
			if (getHangarMatchsNum(h) != 1)
			{
				continue;
			}
			if (getUse(c, h))
			{
				setCurrCraftHangar(c, getHangar(h));
				break;
			}
		}
	}
}

}
