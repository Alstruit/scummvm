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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_CAVES_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_CAVES_H

#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Caves puzzle page (ZoombiniPageType::kCaves).
 * Route 4, Puzzle 1
 *
 * Zoombinis must enter the correct cave based on hieroglyph patterns. 
 * Each cave entrance has a pattern that the player must match with Zoombini attributes.
 *
 * IDA entry: caves_funcInit (0x416978)
 */
class ZoombiniPuzzleCaves : public ZoombiniPuzzle {
public:
	ZoombiniPuzzleCaves(MohawkEngine_Zoombini *vm);
	~ZoombiniPuzzleCaves() override;

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
	Common::String debugGetAnswer() const override;

private:
	void loadZoombinisFromPack();
	
	/**
	 * Initialize difficulty parameters for caves puzzle.
	 * IDA: caves_initDifficultyParams_41896E
	 * Sets up entrance count and SCRB IDs based on difficulty level.
	 */
	void initDifficultyParams();
	
	/**
	 * Setup glyph patterns for cave entrances.
	 * IDA: caves_glyphSetupDispatch_418A6E
	 * Initializes entrance attribute patterns, counts distribution, builds timing table.
	 */
	void setupEntranceGlyphs();
	
	/**
	 * Initialize random entrance attribute patterns using Fisher-Yates shuffle.
	 * IDA: caves_initEntranceAttrPattern_418A7E
	 */
	void initEntranceAttrPattern();
	
	/**
	 * Count glyph distribution across loaded Zoombinis.
	 * IDA: caves_countGlyphDistribution_418BFE
	 */
	void countGlyphDistribution();
	
	/**
	 * Build glyph animation timing table.
	 * IDA: caves_buildGlyphTimingTable_418F6C
	 */
	void buildGlyphTimingTable();
	
	/**
	 * Distribute attributes to cave entrances.
	 * IDA: caves_entranceAttrDist_418CB1
	 */
	void distributeEntranceAttributes();

	/**
	 * Find which cave entrance matches a Zoombini's traits.
	 * IDA: caves_findMatchingGlyphSlot (approx 0x418x)
	 * @return entrance slot (0-based), or -1 if none match
	 */
	int16 findMatchingGlyphSlot(const ZmbTrait &traits, int16 droppedSlot);

	/** Process a correct cave entrance placement. */
	void handleCorrectPlacement(ZmbSnoid *snoid, int16 entranceSlot);

	/**
	 * IDA caves_triggerSuccessAnim_41814F(staggerFrames, x, y).
	 * Picks up to 3 placed snoids (highest entrance slot first) and animates
	 * them walking toward (x, y) staggered by `staggerFrames` ticks each.
	 * Locks UI drag during the celebration sequence.
	 */
	void triggerSuccessAnim(int16 staggerFrames, int16 x, int16 y);

	/** Process a wrong cave entrance placement with redirect. */
	void handleWrongPlacement(ZmbSnoid *snoid, int16 droppedSlot, int16 correctSlot);

	/** End drag and evaluate drop target. */
	void endDrag(const Common::Point &dropPos);

	/** Get entrance slot from screen position. */
	int16 getEntranceSlotAtPoint(const Common::Point &pos) const;

	/**
	 * Custom render callback for the virtual glyph renderer feature.
	 * Draws hieroglyph symbols on each active cave entrance.
	 * IDA: caves_renderAllEntranceGlyphs_41846A
	 */
	ZmbRenderResult renderEntranceGlyphs(ZmbFeature *feature);

	/**
	 * Play an entrance SCRS script on the active drop snoid.
	 * IDA: caves_playEntranceScript_417A4E
	 * @param isReject true for reject type (SCRS sets snoid to reject state)
	 * @param scrsResId SCRS resource ID to play
	 */
	void playEntranceScript(bool isReject, int16 scrsResId);

	/**
	 * Load a glyph panel frame SCRB onto the glyph panel region feature.
	 * IDA: caves_loadScrbFrameOnRunner_4186C2
	 * @param frameIdx Index added to _glyphPanelScrbId for the SCRB to load
	 */
	void loadGlyphPanelFrame(int16 frameIdx);

	/**
	 * Set up door animation for entrance sequences.
	 * IDA: caves_setupDoorAnimation_4177FB
	 *
	 * @param doorIdx Phase index:
	 *   0 = Door opening for SELECTED entrance (correct/wrong drop target)
	 *   1 = Door opening for MATCHING entrance (redirect on wrong placement)
	 *   2 = Door close/reset (walking back after out-of-zone drop)
	 */
	void setupDoorAnimation(int16 doorIdx);

	/**
	 * Get the glyph overlay feature for a given entrance slot index (0-based).
	 * Maps ScummVM entrance index to the appropriate overlay feature.
	 * IDA: caves_entranceSCRBRunnerArr_4AAFF2[slotIdx]
	 *
	 * @param idx 0-based entrance slot index
	 * @return Overlay feature, or nullptr if none exists for this slot
	 */
	ZmbFeature *getEntranceOverlayFeature(int16 idx) const;

	/**
	 * Handle events from entrance door SCRB animations.
	 * IDA: caves_scrbEntranceCallback (0x417A98)
	 * Events: 1(reject SCRS), 2(normal SCRS), 4(transition), 5(pending), 10(position), 20/21(completion)
	 */
	void handleEntranceDoorEvent(ZmbFeature *feature, int16 eventCode);

	/**
	 * Handle events from glyph panel SCRB animations.
	 * IDA: caves_handleScriptEvent_417BF2
	 * Events: 10(phase change), 20(completion check), 21(force completion)
	 */
	void handleGlyphPanelEvent(ZmbFeature *feature, int16 eventCode);

	static const Common::Point kSnoidPositions[20];

	/** DRAW_ON_REG positions for cave entrance features (SCRB 7000-7019). IDA: off_4A09BC+1..+20 */
	static const Common::Point kCaveEntrancePositions[20];

	/**
	 * Entrance type per 0-based entrance index (20 entries).
	 * Type 1 or 2, giving SCRS ID = kEntranceType[idx] + 12999.
	 * IDA: byte_4A0AC8 (16-bit stride, indexed by 1-based entrance index)
	 */
	static const uint8 kEntranceType[20];

	/** Glyph Y positions per slot (0-10). IDA: unk_4A0B24 + unk_4A0B38 */
	static const int16 kGlyphYPositions[11];

	/** Glyph X positions per slot (0-10). IDA: unk_4A0B3A + unk_4A0B4E */
	static const int16 kGlyphXPositions[11];

	/** Route difficulty level + 1 (1-4). IDA: word_4AAF00 */
	ZmbPuzzleDifficultyLevel _difficultyLevel = kPuzzleDiffLevel1;
	
	// --- Glyph system state (IDA: caves_initDifficultyParams globals) ---
	
	/** Number of active entrances (4-7 based on difficulty). IDA: unk_4A08F2 */
	int16 _entranceCount = 4;
	
	/** SCRB ID for glyph panel (6003-6006 based on difficulty). IDA: unk_4A08F0 */
	int16 _glyphPanelScrbId = 6006;
	
	/** Entrance attribute requirements for slots 0-10. IDA: caves_entranceAttrReq[] */
	uint8 _entranceAttrReq[11] = {};
	
	/** Entrance attribute offsets for slots 0-10. IDA: caves_entranceAttrOffset[] */
	uint8 _entranceAttrOffset[11] = {};
	
	/** Glyph timing table for animations. IDA: built by caves_buildGlyphTimingTable_418F6C */
	int16 _glyphTimingTable[11] = {};
	
	/** Current hovered entrance slot. IDA: word_4AAEF4 */
	int16 _hoveredEntranceSlot = 0;

	/** Per-slot glyph X half-width adjustments for centering. IDA: dword_4AB0F4 (from REGS 201) */
	int16 _glyphXAdj[11] = {};
	
	// --- Extended glyph system state (IDA: caves_initEntranceAttrPattern globals) ---
	
	/** Guard complexity (1 or 2). IDA: unk_4A08E4 */
	int16 _guardComplexity = 1;
	
	/** Number of attribute columns used. IDA: word_4A08E2 (typically 5-6) */
	int16 _attrColumnCount = 5;
	
	/** Base attribute types [2]. IDA: word_4AAF02[] */
	int16 _baseAttrTypes[2] = {};
	
	/** Base attribute for second guard. IDA: caves_entranceAttrBase */
	int16 _entranceAttrBase = 0;
	
	/** Attribute columns [2 rows x 5 cols]. IDA: word_4AAF06[5*row+col] */
	int16 _attrColumns[10] = {};
	
	/** Glyph distribution counts [6x6]. IDA: word_4AAF1A[6*i+j] */
	int16 _glyphDistribution[36] = {};
	
	/** Frame-to-slot map for animations. IDA: word_4AAF9E[] */
	int16 _frameToSlotMap[21] = {};
	
	/** Cross-product timing table. IDA: word_4AAFC8[] */
	int16 _crossProductTable[21] = {};

	/**
	 * Per-slot occupancy. IDA stores the runner index (word_4AAF74[]); we store
	 * the ZmbSnoid* (nullptr = empty). Used by findMatchingGlyphSlot to skip
	 * occupied slots and by triggerSuccessAnim to walk placed snoids out.
	 */
	ZmbSnoid *_slotOccupied[21] = {};

	/** Total slots filled so far (matches advancing the search base). IDA: HIWORD(caves_nTotalSlotCount_4A08FC) */
	int16 _totalSlotCount = 0;

	/** Loaded Zoombini trait count. IDA: unk_4A0904 */
	int16 _loadedZmbCount = 0;

	/** 3 entrance animation features. IDA: word_4AB078/7A/7C */
	ZmbFeature *_entranceAnimFeatures[3] = {};
	/** 20 DRAW_ON_REG features for cave doors. IDA: scrb_drawOnRegRunnerIdxArr[0..19] + word_4B7B60[0..3] */
	ZmbFeature *_doorDrawOnRegFeatures[20] = {};
	/** 4 door panel animation features (SCRB 9011-9014). IDA: word_4AB00A-4AB010 */
	ZmbFeature *_doorPanelFeatures[4] = {};
	/** Glyph overlay features for slots 5-11 (SCRB 9004-9010). IDA: word_4AAFF2[5-11] */
	ZmbFeature *_glyphOverlayFeatures[7] = {};
	/** Extra glyph overlay features for slots 16-20 (SCRB 9015-9019). IDA: word_4AAFF2[16-20] */
	ZmbFeature *_extraGlyphOverlayFeatures[5] = {};
	/** SCRB 9014 — main glyph panel animation. IDA: word_4AB010 */
	ZmbFeature *_mainGlyphPanelFeature = nullptr;
	
	/** Glyph panel overlay feature (SCRB 6012). IDA: word_4AB080 (first registration) */
	ZmbFeature *_glyphPanelOverlayFeature = nullptr;
	
	/** Glyph panel REGION_TRACK feature. IDA: word_4AB080 (second registration) */
	ZmbFeature *_glyphPanelRegionFeature = nullptr;
	
	/**
	 * Virtual glyph renderer feature.
	 * Renders entrance glyphs via custom render callback.
	 * IDA: unk_4A090C (uses caves_clearAndInvalidateRect/caves_renderAllEntranceGlyphs_41846A)
	 */
	ZmbFeature *_virtualGlyphRenderer = nullptr;

	// --- Gameplay state ---
	bool _puzzleActive = false;
	bool _processingFrame = false;
	bool _interactionLocked = false;

	// --- Entrance callback state (IDA: caves_scrbEntranceCallback globals) ---

	/** Base SCRS resource ID for entrance scripts. IDA: caves_rejectScrbBaseId_4A08EE = 12004 */
	int16 _rejectScrsBaseId = 12004;

	/** Base SCRB ID for glyph door animations. IDA: caves_glyphScrbBaseId_4A08EC = 8200 */
	int16 _glyphScrbBaseId = 8200;

	/** Number of entrances that have completed transition. IDA: caves_entranceAnimCounter_4A08F4 */
	int16 _entranceAnimCounter = 0;

	/** Currently active door animation overlay feature. IDA: caves_doorScrbId_4A08EA (runner index) */
	ZmbFeature *_activeDoorFeature = nullptr;

	/** Entrance overlay feature for the selected (dropped) slot. IDA: word_4AAF62 */
	ZmbFeature *_selectedDoorOverlay = nullptr;

	/** Entrance overlay feature for the matching (correct) slot. IDA: word_4AAF64 */
	ZmbFeature *_matchingDoorOverlay = nullptr;

	/** Flag set by event 5; triggers reject door animation in onEveryFrame. IDA: caves_bTransitionPending_4A08F6 */
	bool _bTransitionPending = false;

	/** Set by events 20/21 when entrance sequence completes. IDA: unk_4A08E0 */
	bool _entranceCompletionFlag = false;

	/** Set to 1 by event 4; processed in onEveryFrame. IDA: caves_nActiveEntranceAnimCount_4AB01C */
	int16 _nActiveEntranceAnimCount = 0;

	/** Glyph panel animation phase. IDA: word_4AAEFA (0=idle, 1=transitioning, 2=animating, 3=success) */
	int16 _phaseState = 0;

	/** Snoid currently involved in door animation. IDA: word_4AB04A (runner index → snoid pointer) */
	ZmbSnoid *_activeDropSnoid = nullptr;

	/** Entrance index where Zoombini was dropped (1-based). IDA: caves_selectedEntranceIdx_4A0900 */
	int16 _selectedEntranceIdx = 0;

	/** Entrance index that matches Zoombini's traits (1-based). IDA: caves_hoverEntranceIdx_4A0902 */
	int16 _matchingEntranceIdx = 0;

	/** Flag set on wrong placement; triggers correct door animation in onEveryFrame. IDA: word_4AAEFE */
	bool _bWrongPlacement = false;

	/** All Zoombinis placed; mass walk-in pending. IDA: caves_bDoorAnimPending_4AB090 */
	bool _bDoorAnimPending = false;

	/** Advance button clicked; transition pending. IDA: word_4AAEF8 */
	bool _bAdvanceClicked = false;

	/** Advance button enabled. IDA: caves_bAdvanceButtonEnabled_4A08D8 */
	bool _bAdvanceEnabled = false;

	/** Number of Zoombinis placed (correct or redirected). IDA: HIWORD(caves_nTotalSlotCount_4A08FC) */
	int16 _placedZmbCount = 0;

	/**
	 * LIFO stack of snoids queued for walk-in animation after correct
	 * placement. IDA processes the entire stack each tick (not one-per-tick),
	 * so multiple snoids can start their walk-in on the same frame after a
	 * cluster of correct placements. IDA: caves_entranceAnimStates_4AB01E[]
	 * + caves_bHoverEnabled_4AB046 (stack pointer / count).
	 */
	struct WalkInEntry {
		ZmbSnoid *snoid;
		int16 scrsId;
	};
	WalkInEntry _walkInStack[20] = {};
	int16 _walkInStackIdx = 0;

	/** Active mass walk-in animations in progress. IDA: caves_bAnimInProgress_4AB08E */
	int16 _massWalkInProgress = 0;
	/** Total snoids needed for mass walk-in. IDA: caves_nRemainingZmbCount_4AB08C */
	int16 _massWalkRemaining = 0;
	/** Frame counter for 30-tick mass walk-in pacing. IDA: caves_pendingAnimFrameTime_4AB084 */
	uint32 _massWalkLastFrame = 0;
	/** Non-repeat random pool state for mass walk-in snoid picker. IDA: caves_lastAnimFrameTime_4AB088 */
	uint32 _massWalkPoolState = 0;

	/** True when snoid was dropped outside all zones, setupDoorAnimation(2) is active. */
	bool _outOfZoneDrop = false;

	/** Per-entrance hit rects (computed from kCaveEntrancePositions). */
	Common::Rect _entranceHitRects[20];

	/** Click rects for each entrance slot (radius ~30 around entrance pos). */
	static const int16 kEntranceHitRadius = 30;

	/** Hint flash counter for glyph blink at difficulty 1. IDA: word_4AAEF6 */
	int16 _hintFlashCounter = 0;

	/** Frame counter for next glyph blink toggle. IDA: runner->dNextRenderFrame in caves_funcOnHover */
	uint32 _glyphBlinkNextFrame = 0;
};

} // End of namespace Mohawk

#endif
