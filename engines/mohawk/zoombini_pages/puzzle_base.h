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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_BASE_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_BASE_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Puzzle difficulty level (1-based, 1-4).
 * readActivePageRouteLevel() returns 0-3; add 1 to convert.
 */
enum ZmbPuzzleDifficultyLevel : int16 {
	/**
	 * Level1: "Not So Easy"
	 */
	kPuzzleDiffLevel1 = 1,
	/**
	 * Level2: "Oh So Hard"
	 */
	kPuzzleDiffLevel2 = 2,
	/**
	 * Level3: "Very Hard"
	 */
	kPuzzleDiffLevel3 = 3,
	/**
	 * Level4: "Very Very Hard"
	 */
	kPuzzleDiffLevel4 = 4
};

/**
 * Base class for the 12 puzzle pages. Three puzzles form each route,
 * and the player plays them in sequence.
 */
class ZoombiniPuzzle : public ZoombiniInteractive {
public:
	ZoombiniPuzzle(MohawkEngine_Zoombini *vm, ZoombiniPageType pageType);
	~ZoombiniPuzzle() override;
};

} // End of namespace Mohawk

#endif
