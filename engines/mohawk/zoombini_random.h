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

#ifndef MOHAWK_ZOOMBINI_RANDOM_H
#define MOHAWK_ZOOMBINI_RANDOM_H

#include "common/scummsys.h"

namespace Mohawk {

/**
 * Mimics the random algorithms used in the Zoombinis
 */
class ZoombiniRandom {
private:
	uint16 _randSeed;

public:
	/**
	 * Construct a new randomness source with the specific @p name.
	 * The name used must be globally unique, and is used to
	 * register the randomness source with the active event recorder,
	 * if any.
	 */
	ZoombiniRandom(const Common::String &name);

	/**
	 * Generates new seed based on the current date/time
	 */
	static uint16 generateNewSeed();

	void setSeed(uint16 seed); /*!< Set the seed used to initialize the RNG. */

	uint16 getSeed() const { /*!< Get a random seed that can be used to initialize the RNG. */
		return _randSeed;
	}

	/**
	 * Generate a random unsigned integer in the interval [0, max].
	 * @param max	The upper bound
	 * @return	A random number in the interval [0, max].
	 */
	uint16 getRandomNumber(uint16 max);

	/**
	 * Generate a random unsigned integer in the interval [min, max].
	 * @param min	The lower bound.
	 * @param max	The upper bound.
	 * @return	A random number in the interval [min, max].
	 */
	uint16 getRandomNumber(uint16 min, uint16 max);

	/**
	 * Generates a random signed integer in the interval [min, max].
	 * @param min	the lower bound
	 * @param max	the upper bound
	 * @return	a random number in the interval [min, max]
	 */
	int16 getRandomNumberSigned(int16 min, int16 max);
};

} // End of namespace Mohawk

#endif
