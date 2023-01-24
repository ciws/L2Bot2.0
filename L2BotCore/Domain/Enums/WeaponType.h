#pragma once

#include <cstdint>

namespace L2Bot::Domain::Enums
{
	enum class WeaponType : uint8_t
	{
		none = 0,
		sword,
		blunt,
		dagger,
		pole,
		fist,
		bow,
		etc,
		dualsword,
		pet,
		fishingRod
	};
}