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
	
	/**
	 * Initialize difficulty parameters for lilly puzzle.
	 * IDA: lilly_setDifficultyParams (0x4264AC)
	 * Sets up mud ball count and obstacle rows based on difficulty level.
	 */
	void setDifficultyParams();
	
	/**
	 * Initialize the 12x12 grid with attribute patterns.
	 * IDA: fleens_initGridWithAttributes (0x427955), shared with Fleens puzzle.
	 * Sets up grid cell positions and initial attribute state.
	 */
	void initGridWithAttributes();
	
	/**
	 * Create per-zoombini runner features.
	 * IDA: word_4AE3C2[] — SCRB 10109+i for each loaded zoombini.
	 */
	void createZoombiniRunners();

	/** Difficulty level + 1 (1-4). IDA: lilly_difficultyLevel */
	int16 _difficultyLevel = 0;
	
	// --- Puzzle state (IDA: lilly_setDifficultyParams globals) ---
	
	/** Number of mud ball obstacles (0-6 based on difficulty). IDA: lilly_mudBallCount */
	int16 _mudBallCount = 0;
	
	/** Number of obstacle rows (0-3 based on difficulty). IDA: lilly_obstacleRows */
	int16 _obstacleRows = 0;

	/** Count of occupied zoombinis loaded from pack. IDA: lilly_totalZmbCount */
	int16 _totalZmbCount = 0;
	
	// --- Grid state (IDA: fleens_initGridWithAttributes globals) ---
	
	/** Grid type based on difficulty (3/4/5). IDA: word_4AE9CA (implicit from grid selection) */
	int16 _gridType = 0;
	
	/** 12x12 grid cell positions. IDA: posArr_4B7C44, computed from REGS 100 offsets */
	Common::Point _gridCellPositions[12];
	
	/** 12x12 grid primary attribute. IDA: byte_4AC686 */
	byte _gridPrimaryAttr[12][13];
	
	/** 12x12 grid secondary attribute. IDA: byte_4AC688 */
	byte _gridSecondaryAttr[12][13];
	
	// --- Per-zoombini runners (IDA: word_4AE3C2[]) ---
	
	/** Per-zoombini runner features. SCRB 10109+i for each loaded zoombini. */
	ZmbFeature *_zmbRunners[21] = {};
	
	// --- Grid rendering features ---
	
	/** Virtual grid renderer (custom render callback). IDA: word_4A14C8 */
	ZmbFeature *_gridRendererFeature = nullptr;
	
	/** Cursor indicator runner. IDA: lilly_cursorRunnerIdx */
	ZmbFeature *_cursorRunnerFeature = nullptr;
	
	/** Cell animation runner A. IDA: lilly_cellAnimRunnerA */
	ZmbFeature *_cellAnimRunnerA = nullptr;
	
	/** Cell animation runner B. IDA: lilly_cellAnimRunnerB */
	ZmbFeature *_cellAnimRunnerB = nullptr;

	/** 5 overlay features for SCRB 14000-14004. IDA: word_4A7636[14000..14004] */
	ZmbFeature *_overlayFeatures[5] = {};
	/** SCRB 11000 (0x2AF8) — frog event feature (diff > 1). IDA: lilly_frogScrbIdx */
	ZmbFeature *_frogScrbFeature = nullptr;
	/** Frog obstacle runner (diff > 1). IDA: lilly_frogRunnerIdx */
	ZmbFeature *_frogRunnerFeature = nullptr;
};

} // End of namespace Mohawk

#endif
