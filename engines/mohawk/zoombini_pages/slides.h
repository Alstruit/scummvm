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

#ifndef MOHAWK_ZOOMBINI_PAGES_SLIDES_H
#define MOHAWK_ZOOMBINI_PAGES_SLIDES_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Mudball Wall / Slides puzzle page (ZoombiniPageType::kSlides).
 *
 * Route 2, End puzzle: Zoombinis must be placed into slots on a
 * hex grid. Matching attribute patterns let groups slide down.
 * At highest difficulty, NODE/PATH walking is enabled.
 *
 * IDA entry: puzzleSlides_441F0C
 */
class ZoombiniInteractiveSlides : public ZoombiniInteractive {
public:
	ZoombiniInteractiveSlides(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveSlides() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;

protected:
	void onGoButtonActivated() override;
	void onEveryFrame() override;

private:
	void loadZoombinisFromPack();

	static const Common::Point kSnoidPositions[16];

	/** Difficulty level (0-3). IDA: slides_difficultyLevel */
	int16 _difficultyLevel = 0;

	/**
	 * Slot base state for grid initialization.
	 * Default 504, at level 3: 50% chance of 505.
	 * IDA: slides_slotBaseState
	 */
	int16 _slotBaseState = 504;

	/**
	 * Cell spacing for grid positioning.
	 * Default 48, if slotBaseState=505: 24.
	 * IDA: slides_cellSpacing
	 */
	int16 _cellSpacing = 48;

	/** Pending body arrangement override (1-4, 0=none). IDA: word_4B110E */
	int16 _pendingBodyArrangement = 0;

	/** Runner ID of Zoombini currently doing a slide travel animation. IDA: word_4B110C */
	uint16 _activeTravelSnoidId = 0;

	/** Travel direction flag (0=finished, 1=traveling). IDA: word_4B1112 */
	int16 _travelState = 0;

	// Celebration state (IDA: slides_puzzleHoverUpdate @ 0x4427B7)
	/** One-shot flag: set once celebration starts, never cleared until target reached. IDA: slides_celebrationActive (0x4B1C16) */
	bool _celebrationActive = false;
	/** Number of celebrations played so far. IDA: slides_celebrationIndex (0x4B1C12) */
	int16 _celebrationIndex = 0;
	/** Total celebrations needed before reset (= loaded zmb count). IDA: slides_celebrationTarget (0x4B1C10) */
	int16 _celebrationTarget = 0;
	/** Non-repeat random pool bitmask. IDA: dword_4B1C0C */
	uint32 _celebrationPoolState = 0;
	/** Frame counter of last celebration. IDA: dword_4B1C00 */
	uint32 _celebrationLastFrame = 0;
	/** Number of matched columns (trigger for celebration). IDA: slides_matchCount (0x4B1C14) */
	int16 _matchCount = 0;
	/** Number of loaded Zoombinis (pool size for celebration). IDA: slides_numZoombinis (0x4B1AE8) */
	int16 _loadedZmbCount = 0;
};

} // End of namespace Mohawk

#endif
