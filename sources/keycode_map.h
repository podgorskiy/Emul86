#pragma once
#include <map>


enum
{
	NORMAL = 0,
	SHIFT = 1 << 0,
	CTRL = 1 << 1,
	ALT = 1 << 2
};

int GetKeyCode(int key);

namespace detail
{
	struct KeyMap
	{
		KeyMap();

		std::map<int, int> keyCodes;
	};

	const std::map<int, int>& GetKeyMap();
}
