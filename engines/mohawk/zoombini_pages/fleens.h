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

#ifndef MOHAWK_ZOOMBINI_PAGES_FLEENS_H
#define MOHAWK_ZOOMBINI_PAGES_FLEENS_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Fleens puzzle page (ZoombiniPageType::kFleens).
 *
 * Route 2, Puzzle 2: Zoombinis must avoid Fleens (floating creatures)
 * that capture them if they have wrong attributes. The player launches
 * Zoombinis on a raft while managing Fleen patrol patterns.
 *
 * IDA entry: fleens_initAndSetupPuzzle (0x41C3DC)
 */
class ZoombiniInteractiveFleens : public ZoombiniInteractive {
public:
	ZoombiniInteractiveFleens(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveFleens() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onEveryFrame() override;
	void onGoButtonActivated() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;

private:
	void loadZoombinisFromPack();
	
	/**
	 * Build trait transformation data for Zoombinis. 
	 * IDA: ferry_buildZmbRunners_41D9F4
	 * Selects mismatch zoombinis and generates trait transformation offsets.
	 */
	void buildZmbTraitSetup();

	// Attribute slot render callbacks
	bool attrSlots_preRender(ZmbFeature *feature);
	ZmbRenderResult attrSlots_render(ZmbFeature *feature);

	static const Common::Point kSnoidPositions[16];

	/** Raft DRAW_ON_REG position. IDA: dword_4A1028 */
	static const Common::Point kRaftPosition;

	/** Route difficulty level. IDA: fleens_routeLevel */
	int16 _difficultyLevel = 0;

	// --- Puzzle state (IDA: fleens_initAndSetupPuzzle globals) ---
	
	/** Whether the raft is ready to depart. IDA: fleens_bRaftReady (word_4AB202) */
	bool _bRaftReady = false;
	
	/** Whether player interaction is allowed. IDA: fleens_bInteractionAllowed (word_4AB204) */
	bool _bInteractionAllowed = false;
	
	/** Number of Zoombinis that don't match. IDA: fleens_mismatchCount */
	int16 _mismatchCount = 0;
	
	/** Indices of mismatched Zoombinis (1-based). IDA: word_4AB1BC, word_4AB1BE, word_4AB1C0 */
	int16 _mismatchIdx[3] = {0, 0, 0};
	
	/** Trait transformation offsets per attribute type (mod-5). IDA: g_pGameState->wTransitionsDisable[1]+j */
	uint8 _traitOffsets[4] = {0, 0, 0, 0};
	
	/** Second set of trait slot indices (for higher difficulty). IDA: g_pGameState->wTransitionsDisable[3]+j */
	uint8 _traitSlotOrder[4] = {0, 0, 0, 0};

	// --- Dirty flags for attr slot rendering ---
	
	/** Dirty flag for raft button rect. IDA: word_4A1030 */
	bool _raftButtonDirty = false;
	
	/** Dirty flag for attr slot 1 rect. IDA: word_4A1032 */
	bool _attrSlot1Dirty = false;
	
	/** Dirty flag for attr slot 2 rect. IDA: word_4A1034 */
	bool _attrSlot2Dirty = false;

	// Puzzle-specific feature runners
	/** Animation runner (SCRB 1000). IDA: word_4AB1A4 */
	ZmbFeature *_animFeature = nullptr;
	/** Raft DRAW_ON_REG runner (SCRB 1100). IDA: scrb_drawOnRegRunnerIdxArr[0] */
	ZmbFeature *_raftFeature = nullptr;
	/** 7 overlay runners (SCRB 1200-1206). IDA: word_4AA848[] */
	ZmbFeature *_overlayFeatures[7] = {};

	// --- Animation event state ---

	/** Pending body arrangement shape (1-4, or 0 for none). IDA: word_4AB1A0 */
	int16 _pendingBodyArrangement = 0;

	// --- Idle animation state (IDA: fleens_onHoverPerFrame @ 0x41CA77) ---

	/** Number of idle animations played this round. IDA: word_4AB1D0 */
	int16 _idleAnimCount = 0;
	/** Target number of idle animations to play. IDA: word_4AB1A2 */
	int16 _idleAnimTarget = 0;
	/** Frame counter of last idle anim trigger. IDA: dword_4AB438 */
	uint32 _idleAnimLastFrame = 0;
	/** Idle anim interval in frames. IDA: dword_4AB43C (init 60) */
	uint32 _idleAnimInterval = 0;
	/** Non-repeat random pool state for idle anim selection. IDA: dword_4AB440 */
	uint32 _idleAnimPoolState = 0;
	/** Number of loaded Zoombinis from active pack. */
	int16 _loadedZmbCount = 0;
};

} // End of namespace Mohawk

#endif
