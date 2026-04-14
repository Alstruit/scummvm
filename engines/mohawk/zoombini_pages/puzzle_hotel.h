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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_HOTEL_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_HOTEL_H

#include "common/array.h"
#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Hotel puzzle page (ZoombiniPageType::kHotel).
 * Route 3, Puzzle 2
 * 
 * Zoombinis must be assigned to hotel rooms based on attribute matching.
 * Room assignments become more complex at higher difficulty levels with different SCRB sets loaded.
 *
 * IDA entry: hotel_initAndSetupPuzzle (0x41ede4)
 */
class ZoombiniPuzzleHotel : public ZoombiniPuzzle {
public:
	ZoombiniPuzzleHotel(MohawkEngine_Zoombini *vm);
	~ZoombiniPuzzleHotel() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;
	void onEveryFrame() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;

	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;

protected:
	void onGoButtonActivated() override;
	ZmbSnoid *findSnoidAtPoint(const Common::Point &pos) override;

private:
	// --- Initialization helpers ---
	void loadZoombinisFromPack();
	void computeTraitVariantCounts();
	void generateRoomRules();

	// --- Game board setup ---
	void setupGameBoard();
	void registerDisplayScrbs();
	/** Reassign remaining non-placed snoids to first N pedestal positions. IDA: zmb_assignPedestalPositions */
	void reassignPedestalPositions(int16 count);

	// --- Attribute validation ---
	/** Validate placement for diff 0–2 (2 axes). Returns true if valid. IDA: picker_checkAttrFilter_421729 */
	bool validate2AttrPlacement(int16 slot, int16 axis2Val, int16 axis1Val) const;
	/** Validate placement for diff 3 (3 axes). Returns true if valid. IDA: hotel_checkZmbFitsRoom_421E41 */
	bool validate3AttrPlacement(int16 slot, int16 axis3Val, int16 axis2Val, int16 axis1Val) const;

	// --- Attribute grid management ---
	/** Fill row/column constraints after placing a zmb. IDA: ferry_fillCellRow_4216BC */
	void fillCellRow(int16 rowIdx, int16 axis2Val, int16 axis1Val);
	/** Set diff-3 constraints in all three grids. IDA: maze_setCellAttrsInGrids_422197 */
	void setCellAttrsIn3Grids(int16 cellIdx, int16 attrType, int16 attrValue, int16 gridLayer);

	// --- Zmb placement ---
	/** Place zoombini into assigned room slot — plays SCRS animation. IDA: hotel_setupRoomSlotScrb_422534 */
	void placeZoombiniInRoom(int16 roomSlot, ZmbSnoid *snoid);

	// --- Win condition ---
	/** Register checkpoint callbacks on hotspot runners. IDA: maze_registerCheckpointRunners_422A61 */
	void registerWinCheckpoints();

	// --- Palette effect on wrong placement ---
	/** Dim palette by 88/90/92% based on difficulty. IDA: picker_applyBrightnessDim_42185D */
	void dimPaletteOnError();

	// --- Drag-and-drop ---
	void endDrag(const Common::Point &mousePos);
	/** Find closest room slot for the drop position, or -1 if none. */
	int16 getDropTargetSlot(const Common::Point &dropPos) const;

	// --- Trait access ---
	/**
	 * Return the trait byte for axis-index axisIdx.
	 * Axis ordering in the original packed DWORD: byte0=foot, byte1=nose, byte2=eye, byte3=head.
	 * IDA: *(char*)(&traitDword + axisIdx)
	 */
	byte getTraitValue(const ZmbTrait &trait, int16 axisIdx) const;

	// -----------------------------------------------------------------------
	// Static data tables (IDA binary data)
	// -----------------------------------------------------------------------
	/** Room center positions for diff 0–2 (25 slots). IDA: dword_4A1110 */
	static const Common::Point kRoomPositions25[25];
	/** Room center positions for diff 3 (125 slots). IDA: dword_4A1178 */
	static const Common::Point kRoomPositions125[125];
	/** Column X-offsets for diff-3 room-runner registration. IDA: word_4A138C[5] */
	static const int16 kColumnOffsetX[5];
	/** Column Y-offsets for diff-3 room-runner registration. IDA: word_4A1396[5] */
	static const int16 kColumnOffsetY[5];
	/** Pedestal positions for the 20 zoombini pack slots at the hotel entrance. IDA: 0x4A13E4 */
	static const Common::Point kSnoidPositions[20];

	// -----------------------------------------------------------------------
	// Difficulty / setup
	// -----------------------------------------------------------------------
	/** Difficulty level (1–4). IDA: hotel_difficultyLevel */
	ZmbPuzzleDifficultyLevel _difficultyLevel = kPuzzleDiffLevel1;
	/** Max rejection steps per round. IDA: hotel_maxStepsPerRound */
	int16 _maxStepsPerRound = 5;
	/** Total room slot count (25 for diff 0–2, 125 for diff 3). IDA: word_4AB776 */
	int16 _totalRoomCount = 0;

	// -----------------------------------------------------------------------
	// Feature runners (set in loadFeatures or setupGameBoard)
	// -----------------------------------------------------------------------
	/**
	 * Room/guide animation runner (dual purpose: intro=7000+type, then guide=7500+diff).
	 * IDA: hotel_roomRunnerIdxArr_4AB742
	 */
	ZmbFeature *_roomAnimFeature = nullptr;
	/** Intro overlay runner (SCRB 11500+adj_diff). Freed after board setup. IDA: word_4AB750 */
	ZmbFeature *_introFeature = nullptr;
	/** Elevator/room SCRB runner (SCRB 11800). IDA: word_4AB752 */
	ZmbFeature *_roomScrbFeature = nullptr;
	/** Per-slot room display features (SCRB slot+6013 / slot%5+9002 for diff3). IDA: word_4AB648/word_4AB998 */
	ZmbFeature *_roomDisplayFeatures[125] = {};
	/** Per-slot room icon/click-zone features (SCRB slot+6038). IDA: hotel_roomIconRunnerArr_4AB54E */
	ZmbFeature *_roomIconFeatures[125] = {};
	/** Label feature (SCRB 11503/11504/11505). IDA: word_4AB744 */
	ZmbFeature *_labelFeature = nullptr;
	/** Per-slot obstacle/forbidden-room features. IDA: word_4AB89E */
	ZmbFeature *_forbiddenFeatures[125] = {};
	/** Step-counter animation feature (SCRB stepCount+6000). IDA: word_4AB782 */
	ZmbFeature *_counterFeature = nullptr;

	// -----------------------------------------------------------------------
	// Attribute constraint grids
	// -----------------------------------------------------------------------
	/**
	 * Axis-1 constraint per slot (diff 0–2: per-slot, diff 3: per row-group).
	 * 0 = unset. IDA: word_4AB830
	 */
	int16 _attrGrid1[25] = {};
	/**
	 * Axis-2 constraint per slot (diff 0–2: per-slot, diff 3: per floor).
	 * 0 = unset. IDA: word_4AB862
	 */
	int16 _attrGrid2[25] = {};
	/**
	 * Axis-3 constraint per column (diff 3 only). 0 = unset. IDA: word_4AB894
	 */
	int16 _attrGrid3[5] = {};
	/** When true, all attribute validators return valid immediately. IDA: word_4AB54C */
	bool _attrBypass = false;

	// -----------------------------------------------------------------------
	// Puzzle axis selection
	// -----------------------------------------------------------------------
	/** Random axis for constraint dimension 1 (0=foot,1=nose,2=eye,3=head). IDA: word_4AB766 */
	int16 _attrAxis1 = 0;
	/** Random axis for constraint dimension 2. IDA: word_4AB768 */
	int16 _attrAxis2 = 0;
	/** Random axis for constraint dimension 3 (diff 3 only). IDA: word_4AB76A */
	int16 _attrAxis3 = 0;
	/** Distinct trait-value counts per axis (axis 0-3). IDA: word_4AB78E[4] */
	int16 _traitVariantCounts[4] = {};
	/** Count of zoombinis currently in the active pack. IDA: hotel_totalZmbCount */
	int16 _totalZmbCount = 0;

	// -----------------------------------------------------------------------
	// Room grid state
	// -----------------------------------------------------------------------
	/**
	 * Per-slot state: -1 = forbidden, 0 = empty, 1-6 = zmbs placed (depth).
	 * 25 entries for diff 0–2; 125 for diff 3. IDA: hotel_roomGrid
	 */
	int16 _roomGrid[125] = {};
	/** SCRB-offset IDs for forbidden-room obstacle selection (up to 8). IDA: hotel_forbiddenRoomIds */
	int16 _forbiddenRoomIds[8] = {};

	// -----------------------------------------------------------------------
	// Placement tracking
	// -----------------------------------------------------------------------
	/** Current target room slot (0-based). IDA: hotel_targetRoomSlot (word_4AB774) */
	int16 _targetRoomSlot = -1;
	/** Number of zoombinis successfully placed. IDA: hotel_placedCount */
	int16 _placedCount = 0;
	/** Rejection step counter (1-based). IDA: hotel_stepCounter */
	int16 _stepCounter = 1;
	/** Pack snoid IDs that have already been placed (no longer draggable). */
	Common::Array<uint16> _placedSnoidIds;

	// -----------------------------------------------------------------------
	// Pending placement
	// -----------------------------------------------------------------------
	/** Pack snoid just dropped — deferred visual placement pending. IDA: word_4ABB8C */
	ZmbSnoid *_pendingPlacementSnoid = nullptr;
	/** True = accepted; false = rejected. IDA: pcStr1[11]==1 */
	bool _pendingAccepted = false;
	/** Snoid whose room-entrance SCRS animation is currently playing. IDA: word_4AB772 */
	ZmbSnoid *_placedZmbSnoid = nullptr;
	/** Final room position for the placed snoid. IDA: dword_4ABB98 */
	Common::Point _placedZmbPos;

	// -----------------------------------------------------------------------
	// Puzzle state flags
	// -----------------------------------------------------------------------
	/** True once the room board is fully set up. IDA: hotel_bPuzzleActive */
	bool _bPuzzleActive = false;
	/** True = puzzle completed. IDA: hotel_bPuzzleComplete */
	bool _bPuzzleComplete = false;
	/** True = this is the very first placement attempt. IDA: hotel_bFirstPlacement */
	bool _bFirstPlacement = true;
	/** True = a rejection animation is playing, interaction locked. IDA: word_4AB756 */
	bool _bRejectAnimActive = false;
	/** True = drag lock (drop evaluation in progress). IDA: word_4AB764 */
	bool _bInteractionLock = false;

	// -----------------------------------------------------------------------
	// State machine completion flags (set by onFeatureAnimEvent)
	// -----------------------------------------------------------------------
	/** Intro PLAY_ONCE completed → triggers setupGameBoard(). IDA: word_4AB77A */
	bool _bBatchWalkDone = false;
	/** Guide-prompt animation completed → start counter. IDA: word_4AB77C */
	bool _bPromptAnimDone = false;
	/** Counter-step animation completed → advance step. IDA: word_4AB784 */
	bool _bCounterAnimDone = false;
	/** Counter-step-rejection hotspot fired (counter animation done during rejection). IDA: word_4AB778 */
	bool _bCounterStepDone = false;
	/** Win/cheer hotspot completed (guide cheer or counter chain done). IDA: word_4AB77E */
	bool _bWinAnimDone = false;
	/** Room-entrance SCRS completed → finalise position + check win. IDA: word_4AB780 */
	bool _bPlacedZmbAnimDone = false;

	// -----------------------------------------------------------------------
	// Counter / reset
	// -----------------------------------------------------------------------
	/** User clicked to skip guide animation. IDA: word_4AB7C4 */
	bool _bGuideSkipped = false;
	/** Enables click-to-skip during animations. IDA: word_4AB7C2 */
	bool _bClickToSkipEnabled = true;
	/** Number of board resets (drives guide escalation). IDA: word_4AB76E */
	int16 _overflowCounter = 0;

	// -----------------------------------------------------------------------
	// Guide animation state
	// -----------------------------------------------------------------------
	/**
	 * What animation _roomAnimFeature is currently playing:
	 * 0=none, 1=prompt(7500+diff), 2=cheer(7507+), 3=win.
	 */
	uint8 _guideAnimPurpose = 0;
	/** Intro room animation type (0-6). IDA: word_4AB746 */
	int16 _introAnimType = 0;
	/** True when intro type 0 or 4 (guide needs prompt after intro). IDA: word_4AB748 */
	bool _bIntroNeedsGuide = false;
	/** Tutorial state counter. IDA: word_4AB75E */
	int16 _tutorialState = 0;
	/** Frame counter saved at game-board setup (for fidget timeout). IDA: dword_4AB7C8 */
	uint32 _setupFrameCount = 0;
};

} // End of namespace Mohawk

#endif
