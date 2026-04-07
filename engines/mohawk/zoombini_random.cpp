/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "mohawk/resource.h"

#include "common/random.h"
#include "common/system.h"
#include "common/config-manager.h"
#include "gui/EventRecorder.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_random.h"

namespace Mohawk {

ZoombiniRandom::ZoombiniRandom(const Common::String &name) : _scummRnd(name) {
	_useOriginal = ConfMan.getBool("original_prng");

#ifdef ENABLE_EVENTRECORDER
	assert(g_system);
	setSeed(g_eventRec.getRandomSeed(name));
#else
	setSeed(generateNewSeed());
#endif
}

void ZoombiniRandom::setSeed(uint16 seed) {	
	if (seed == 0)
		seed++;
	_randSeed = seed;
}

uint16 ZoombiniRandom::generateNewSeed() {
	return Common::RandomSource::generateNewSeed();
}

uint16 ZoombiniRandom::getRandomNumber(uint16 max) {
	if (_useOriginal) {
		_randSeed = 214013u * _randSeed + 2531011u;
		return static_cast<uint16>(_randSeed) % (max + 1);
	}
	return static_cast<uint16>(_scummRnd.getRandomNumber(max));
}

uint16 ZoombiniRandom::getRandomNumber(uint16 min, uint16 max) {
	if (max < min) {
		warning("ZoombiniRandom::getRandomNumber: max(%u) is smaller than min(%u), swapping", max, min);
		uint16 tmp = max;
		max = min;
		min = tmp;
	}
	return getRandomNumber(max - min) + min;
}

int16 ZoombiniRandom::getRandomNumberSigned(int16 min, int16 max) {
	if (max < min) {
		warning("ZoombiniRandom::getRandomNumberSigned: max(%d) is smaller than min(%d), swapping", max, min);
		int16 tmp = max;
		max = min;
		min = tmp;
	}
	return getRandomNumber(max - min) + min;
}

uint16 ZoombiniRandom::getNonRepeatRandom(uint16 poolSize, uint32 &bitmask) {
	uint32 fullMask = (poolSize < 32) ? ((1u << poolSize) - 1u) : 0xFFFFFFFFu;
	if ((bitmask & fullMask) == fullMask)
		bitmask = 0;

	uint16 idx = getRandomNumber(poolSize - 1);
	while (bitmask & (1u << idx))
		idx = (idx + 1) % poolSize;
	bitmask |= (1u << idx);
	return idx;
}

} // End of namespace Mohawk
