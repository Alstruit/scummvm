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

#ifndef MOHAWK_ZOOMBINI_PAGES_LILLY_H
#define MOHAWK_ZOOMBINI_PAGES_LILLY_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Lily Pads puzzle page (ZoombiniPageType::kLilly).
 *
 * Route 3, Puzzle 2: Zoombinis must cross a pond by hopping on lily pads.
 * The grid of pads has an adjacency-based movement system; the player
 * selects pads to form paths. At higher difficulty a frog obstacle appears.
 *
 * IDA entry: lilly_puzzleInit (0x422de4)
 * NOTE: Non-standard layout — does NOT use zmb_layoutStaticAndWalkInGroups.
 */
class ZoombiniInteractiveLilly : public ZoombiniInteractive {
public:
	ZoombiniInteractiveLilly(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveLilly() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;

private:
	void loadZoombinisFromPack();

	/** Difficulty level + 1 (1-4). IDA: lilly_difficultyLevel */
	int16 _difficultyLevel = 0;

	/** Count of occupied zoombinis loaded from pack. IDA: lilly_totalZmbCount */
	int16 _totalZmbCount = 0;

	/** 5 overlay features for SCRB 14000-14004. IDA: word_4A7636[14000..14004] */
	ZmbFeature *_overlayFeatures[5] = {};
	/** SCRB 11000 (0x2AF8) — frog event feature (diff > 1). IDA: lilly_frogScrbIdx */
	ZmbFeature *_frogScrbFeature = nullptr;
};

} // End of namespace Mohawk

#endif
