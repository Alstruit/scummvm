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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_SMOKE_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_SMOKE_H

#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Per-runner puzzle-specific state for the smoke puzzle.
 * Maps IDA byte offsets in the original runner struct to named fields.
 */
struct ZmbSmokeRunnerState {
	uint8 attrs[4] = {};       // IDA: runner+188..191 (grid/cliff display attrs)
	uint8 cachedAttrs[4] = {}; // IDA: runner+236..239 (cached zmb attrs stored on runner)
	uint8 orientation = 0;     // IDA: runner+241
	uint8 matchCount = 0;      // IDA: runner+245
	uint8 hasMatch = 0;        // IDA: runner+248
	uint8 attrCyclePos = 0;    // IDA: runner+293
};

/**
 * Smoke puzzle page (ZoombiniPageType::kSmoke).
 * Route 4, Puzzle 2
 * 
 * Magic mirror checks if the lvalue and rvalue matches. 
 * The player must place the correct Zoombini onto the minecart (lvalue), and correct mirror (rvalue) to make the Zoombini passable.
 * In the later difficulty levels, the player must also place the correct shims between the lvalue/rvalue and the magic mirror.
 * 
 * The puzzle uses a custom stack-based positioning system instead of the standard walk-in layout.
 * NON-STANDARD: Does not use layoutStaticAndWalkIn().
 *
 * IDA entry: smoke_init (0x44983c)
 */
class ZoombiniPuzzleSmoke : public ZoombiniPuzzle {
public:
	ZoombiniPuzzleSmoke(MohawkEngine_Zoombini *vm);
	~ZoombiniPuzzleSmoke() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;
	void onEveryFrame() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;

	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onMouseMove(const Common::Point &absPos, const Common::Point &relPos) override;

protected:
	void onGoButtonActivated() override;
	Common::String debugGetAnswer() const override;

private:
	void loadZoombinisFromPack();

	// === Static coordinate tables (from IDA) ===

	static const Common::Point kSnoidPositions[20];        // IDA: 0x4A4368 — zoombini pedestal positions
	static const Common::Point kDrawOnRegPosition;          // IDA: stru_4A400C+0x00
	static const Common::Point kCliffRunnerPositions[8];    // IDA: stru_4A400C+0x18
	static const Common::Point kGridRunnerPositions[8];     // IDA: stru_4A400C+0x40
	static const Common::Point kExitRunnerPositions[2];     // IDA: stru_4A400C+0x70
	static const Common::Point kBottomRunnerPositions[2];   // IDA: dword_4A4184
	static const Common::Point kHidePosition;               // IDA: dword_4A4008 — off-screen hide
	static const Common::Point kRejectPosition;             // IDA: dword_4A4004 — rejection position
	static const Common::Point kCliffDropSnapPosition;      // IDA: stru_4A400C+0x74
	static const Common::Rect kCliffDropRect;               // IDA: stru_4A400C+0x80 — L1-2 drop zone
	static const int16 kColumnSnapX[5];                     // IDA: word_4A435C — cliff column X snap
	static const Common::Rect kDragRectsA[9];               // IDA: unk_4A40DC — L3-4 group A (3 slots × 3 rects)
	static const Common::Rect kDragRectsB[9];               // IDA: unk_4A4124 — L3-4 group B (3 slots × 3 rects)
	static const Common::Point kDisplayPairNormalA[12];     // IDA: dword_4A418C
	static const Common::Point kDisplayPairNormalB[12];     // IDA: dword_4A41C4
	static const Common::Point kDisplayPairSwappedA[12];    // IDA: dword_4A41FC
	static const Common::Point kDisplayPairSwappedB[12];    // IDA: dword_4A4240

	// === SCRB feature runners ===

	ZmbFeature *_overlayAnimFeature = nullptr;  // IDA: smoke_scrbOverlayAnim (11013 or 11011)
	ZmbFeature *_level12ExtraFeature = nullptr;  // IDA: smoke_scrbLevel12Extra (11076, L1-2 only)
	ZmbFeature *_cliffLeftFeature = nullptr;     // IDA: smoke_scrbCliffLeft (11006)
	ZmbFeature *_cliffRightFeature = nullptr;    // IDA: smoke_scrbCliffRight (11007)
	ZmbFeature *_mainAnimFeature = nullptr;      // IDA: smoke_scrbMainAnim
	ZmbFeature *_smokeStackAFeature = nullptr;   // IDA: smoke_scrbSmokeStackA
	ZmbFeature *_smokeStackBFeature = nullptr;   // IDA: smoke_scrbSmokeStackB (L3-4 only)
	ZmbFeature *_secondAnimFeature = nullptr;    // IDA: smoke_scrbSecondAnim
	ZmbFeature *_compareAFeature = nullptr;      // IDA: smoke_scrbCompareA (11018)
	ZmbFeature *_compareBFeature = nullptr;      // IDA: smoke_scrbCompareB (11019)
	ZmbFeature *_bgOverlayFeature = nullptr;     // IDA: smoke_scrbBgOverlay (11009)
	ZmbFeature *_rejectionFeature = nullptr;     // IDA: smoke_scrbRejection (11036)
	ZmbFeature *_backgroundFeature = nullptr;    // IDA: smoke_scrbBackground (11008)
	ZmbFeature *_answerZoneFeature = nullptr;    // IDA: smoke_scrbAnswerZone (11002)
	ZmbFeature *_holdingAreaFeature = nullptr;   // IDA: smoke_scrbHoldingArea (11077)
	ZmbFeature *_drawOnRegFeature = nullptr;     // IDA: scrb_drawOnRegRunnerIdxArr[0] (11001, L<3)

	// === Gameplay methods ===

	/** IDA: smoke_buildRunnerStacks (0x44DBE2) */
	void buildRunnerStacks();

	/** IDA: smoke_spawnStackRunners (0x44DC7B) */
	void spawnStackRunners(int16 count, int16 runnerType);

	/** IDA: smoke_selectQuestionZmb (0x44D372). Mutates _questionResult and _questionAttrs[]. */
	void selectQuestionZmb();

	/** IDA: smoke_copyPairToCompareBuffer (0x44D459) */
	int16 copyPairToCompareBuffer();

	/** IDA: smoke_assignRunnerAttrsForLevel (0x44D67C) */
	void assignRunnerAttrsForLevel(int16 levelIdx, ZmbSmokeRunnerState &state);

	/** IDA: smoke_generateAttrGrid (0x44E181) */
	void generateAttrGrid(int16 rowIndex, ZmbSmokeRunnerState &state);

	/** IDA: smoke_assignZmbAttrsFromSrc (0x44C048) — copy zmb attrs to runner */
	void assignZmbAttrsFromSrc(int16 srcIdx, ZmbSnoid *zmb);

	/** IDA: smoke_cacheZmbAttrs (0x44C124) — read runner cachedAttrs to attrDisplayTable */
	void cacheZmbAttrs(int16 srcIdx, ZmbSnoid *zmb);

	/** IDA: smoke_loadZmbAttrsToCache (0x44C181) — read grid runner attrs to attrDisplayTable */
	void loadZmbAttrsToCache();

	/** IDA: smoke_cycleZmbAttrDisplay (0x44C2AB) — advance display runner attr cycling */
	void cycleZmbAttrDisplay();

	/** IDA: smoke_cacheAnswerRunnerAttrs (0x44C218) — cache answer runner attrs */
	void cacheAnswerRunnerAttrs();

	/** IDA: smoke_advanceAnswerRunnerFrames (0x44C444) */
	void advanceAnswerRunnerFrames();

	/** IDA: smoke_clearZmbAttrs (0x44C5E5) — clear runner cachedAttrs by index */
	void clearZmbAttrs(int16 idx);

	/** IDA: smoke_clearRunnerSlot (0x44C65C) */
	void clearRunnerSlot(int16 slotIdx);

	/** IDA: smoke_clearAllRunnerSlots (0x44C69A) */
	void clearAllRunnerSlots();

	/** IDA: smoke_clearDisplayRunners (0x44C6D0) */
	void clearDisplayRunners();

	/** IDA: smoke_initMatchCompareRunners (0x44C739) */
	void initMatchCompareRunners();

	/** IDA: pizza_compareTwoOrderLines (0x44C7D0) */
	int16 compareTwoOrderLines();

	/** IDA: smoke_initQuestionRunners (0x44D510) — reinit cliff runners for L1-2 */
	void initQuestionRunners(int16 count);

	/** IDA: smoke_assignAllRunnersAttrs (0x44D651) — reassign all level2 runners */
	void assignAllRunnersAttrs();

	/** IDA: smoke_initAllRunnerAttrs (0x44XXXX) — reinit grid runners for L3-4 */
	void initAllRunnerAttrs(int16 param);

	/** IDA: smoke_startNextCompareSequence (0x44C91A) */
	void startNextCompareSequence();

	/** IDA: smoke_resetAndReinitLevel (0x44BBF0) */
	void resetAndReinitLevel();

	/** IDA: smoke_handleFrameTransition (0x44D281) — L4 transition phases */
	void handleFrameTransition(int16 eventCode);

	/** IDA: smoke_playZmbRejectedAnim (0x44CA52) */
	void playRejectedAnimation();

	/** IDA: smoke_loadSCRBOnAnswerRunner (0x44BA3D) */
	void loadScrbOnAnswerRunner(uint16 scrbId);

	/** IDA: smoke_loadSCRBOnWellRunner (0x44BAB9) */
	void loadScrbOnWellRunner(uint16 scrbId);

	/** IDA: smoke_loadScoreDisplayScrbs (0x44BB5D) */
	void loadScoreDisplayScrbs();

	/** IDA: smoke_loadTimerScrb (0x44BBAD) */
	void loadTimerScrb();

	/** IDA: smoke_scrbAnimDispatch (0x44CB72) — central animation callback */
	void processAnimDispatchEvent(ZmbFeature *feature, int16 eventCode);

	/** IDA: smoke_dragZmbRunner (0x44F2B0) — custom drag mechanic (event-driven state machine) */
	int16 evaluateRunnerDrop(ZmbFeature *runner, const Common::Point &dropPos);

	/** IDA: smoke_playZmbScript — play SCRS on a snoid via page resource */
	void playZmbScript(bool linkToHotspot, ZmbFeature *parentFeature, uint16 scrsId, ZmbSnoid *snoid);

	/** IDA: smoke_unloadTimerScrb — unload L1-2 timer SCRB */
	void unloadTimerScrb();

	/** Find a SmokeRunnerState for a given runner feature. */
	ZmbSmokeRunnerState *findRunnerState(ZmbFeature *feature);

	// === Core gameplay state ===

	ZmbPuzzleDifficultyLevel _difficultyLevel = kPuzzleDiffLevel1;       // IDA: smoke_difficultyLevel — route level + 1 (1-4)
	bool _puzzleActive = false;        // IDA: smoke_bInitialized (0x4B1E64)
	bool _processingFrame = false;     // IDA: word_4A43BE — reentrancy guard
	int16 _zmbCount = 0;              // IDA: smoke_zmbCount (0x4B1D02)
	int16 _currentZmbIdx = 0;         // IDA: smoke_currentZmbIdx (0x4B1E08)
	int16 _placedZmbCount = 0;        // IDA: smoke_placedZmbCount (0x4B1D40)
	int16 _loadedOnCliffCount = 0;    // IDA: smoke_loadedOnCliffCount (0x4B1D44)
	int16 _answerState = 2;           // IDA: smoke_answerState (0x4B1E28) — 0=hidden, 1=question, 2=idle
	bool _bShowAnswer = false;         // IDA: smoke_bShowAnswer (0x4B1E26)
	int16 _compareIdx = 0;            // IDA: smoke_compareIdx (0x4B1E10)
	bool _bCompareSwapped = false;     // IDA: smoke_bCompareSwapped (0x4B1E14)
	int16 _transitionPhase = 3;       // IDA: smoke_transitionPhase (0x4B1E18)
	int16 _animSetIdx = 0;            // IDA: smoke_animSetIdx — animation set cycling index
	bool _bFirstAttrAssign = true;     // IDA: smoke_bFirstAttrAssign — first-time attr flag
	int16 _displayPairIdx = 0;        // IDA: smoke_displayPairIdx — display pair counter
	bool _bMatchReady = false;         // IDA: smoke_bMatchReady — match ready for L1-2
	bool _bRunnerToggle = false;       // IDA: smoke_bRunnerToggle — smoke stack A/B toggle
	bool _bExitGateEnabled = false;    // IDA: smoke_bExitGateEnabled
	bool _bDragLocked = false;         // IDA: smoke_bDragLocked / ui_bDragLockActive
	bool _bCheatMode = false;          // IDA: smoke_bCheatMode
	int16 _remainingExitSteps = 0;     // IDA: smoke_remainingExitSteps (0x4B1E04)
	int16 _rejectedCount = 0;         // IDA: smoke_rejectedCount
	ZmbSnoid *_currentDragZmb = nullptr; // IDA: smoke_currentDragZmb

	// --- Non-snoid runner drag state (ScummVM event-driven replacement for blocking dragZmbRunner) ---
	ZmbFeature *_draggedRunner = nullptr;    // runner being dragged (cliff/grid feature)
	Common::Point _dragRunnerOrigPos;        // original position before drag
	uint32 _dragRunnerSavedInterval = 0;     // saved frame interval during drag
	int16 _dragRunnerMatchIdx = -1;          // IDA: smoke_bMatchReady index for re-drag
	bool _bRunnerDragActive = false;         // whether a non-snoid drag is in progress

	// --- Event flags (set by anim callback, consumed by frame handler) ---

	bool _bPlaceZmb = false;          // IDA: smoke_bPlaceZmb (0x4B1E0A)
	bool _bLinkRunners = false;        // IDA: smoke_bLinkRunners (0x4B1E1A)
	bool _bReloadScrb = false;         // IDA: smoke_bReloadScrb (0x4B1E1C)
	bool _bResetLevel = false;         // IDA: smoke_bResetLevel (0x4B1E1E)
	bool _bShowResults = false;        // IDA: smoke_bShowResults (0x4B1E24)
	bool _bReloadMainRunner = false;   // IDA: smoke_bReloadMainRunner (0x4B1D18)
	int16 _word4B1E0C = 0;             // IDA: word_4B1E0C (0x4B1E0C)
	int16 _word4B1E20 = 0;             // IDA: word_4B1E20 (0x4B1E20)
	bool _bWord4A43B8 = false;         // IDA: word_4A43B8 — overlay anim reload sentinel

	// --- Question/Answer attributes ---

	uint8 _questionAttrs[8] = {};     // IDA: stru_4B1D0C — 4 primary + 4 secondary attrs
	int16 _questionResult = 0;        // IDA: smoke_questionResult (0x4B1E0E)

	// === Attribute display system ===

	/**
	 * 8-slot × 4-attr display table for attribute cycling.
	 * IDA: word_4B1DA0 (32 bytes).
	 * Slots 0-2: display runners (left side), 3-5: display runners (right side),
	 * 6: spare, 7: spare.
	 */
	uint8 _attrDisplayTable[32] = {};

	/** Seen attribute history arrays (cross-level persistence). IDA: smoke_seenAttrA/B */
	uint8 _seenAttrA[4] = {};
	uint8 _seenAttrB[4] = {};

	/** Shuffled row permutation for grid display. IDA: smoke_permutation (0x4B1E3C) */
	int16 _permutation[8] = {};

	/** Level 1/2 attribute assignment history. IDA: smoke_level1AttrHistory / level2AttrHistory */
	uint8 _level1AttrHistory[8] = {};
	uint8 _level2AttrHistory[8] = {};

	// === SCRB resource IDs (per-difficulty) ===

	uint16 _scrbAnimIdArr[4] = {};     // IDA: smoke_scrbAnimIdArr (0x4B1DE8) — 4 anim SCRBs
	uint16 _scrbZmbAnimIdArr[2] = {};  // IDA: smoke_scrbZmbAnimIdArr (0x4B1DF0) — zmb anim bases
	uint16 _scrbSmokeStackResA = 0;    // IDA: smoke_scrbSmokeStackResA
	uint16 _scrbSmokeStackResB = 0;    // IDA: smoke_scrbSmokeStackResB
	uint16 _scrbOverlayResId = 0;      // IDA: smoke_scrbOverlayResId
	uint16 _scrbTransitionResId = 0;   // IDA: smoke_scrbTransitionResId (L4 only)
	uint16 _scrbTravelResId = 0;       // IDA: smoke_scrbTravelResId (L3-4)
	uint16 _scrbPickupResId = 0;       // IDA: smoke_scrbPickupResId (L3-4)
	uint16 _scrbDropResId = 0;         // IDA: smoke_scrbDropResId (L3-4)
	uint16 _scrbWalkResId = 0;         // IDA: smoke_scrbWalkResId (L3-4)

	// === Runner arrays ===

	ZmbFeature *_smokeColumnRunners[20] = {}; // IDA: smoke_smokeColumnRunnerArr (0x4B1C88)
	int16 _smokeColumnCount = 0;

	ZmbSnoid *_zmbOnCliff[20] = {};           // IDA: smoke_zmbOnCliffArr (0x4B1CB0)

	/**
	 * Compare-slot runners (IDA word_4B1CB2 / word_4B1CB4). When a pair is
	 * under compare inspection, these hold references to the active pair.
	 * Idle animations must exclude these so the compared snoids don't move.
	 */
	ZmbSnoid *_compareSlotRunnerA = nullptr;
	ZmbSnoid *_compareSlotRunnerB = nullptr;

	uint16 _zmbQueue[21] = {};                // IDA: smoke_zmbQueueArr (0x4B1CD8)
	int16 _zmbQueueSize = 0;

	ZmbFeature *_cliffRunners[20] = {};       // IDA: smoke_cliffRunnerArr — type 1
	int16 _cliffRunnerCount = 0;
	ZmbSmokeRunnerState _cliffRunnerStates[20];

	ZmbFeature *_level2Runners[6] = {};       // IDA: smoke_level2RunnerArr — type 2
	int16 _level2RunnerCount = 0;
	ZmbSmokeRunnerState _level2RunnerStates[6];

	ZmbFeature *_gridRunners[9] = {};         // IDA: smoke_gridRunnerArr — type 3
	int16 _gridRunnerCount = 0;               // NOTE: starts at 1 in original!
	ZmbSmokeRunnerState _gridRunnerStates[9];

	ZmbFeature *_exitRunners[4] = {};         // IDA: smoke_exitRunnerArr — type 4
	int16 _exitRunnerCount = 0;
	ZmbSmokeRunnerState _exitRunnerStates[4];

	ZmbFeature *_bottomRunners[2] = {};       // IDA: smoke_bottomRunnerArr — type 5
	int16 _bottomRunnerCount = 0;
	ZmbSmokeRunnerState _bottomRunnerStates[2];

	int16 _dragSlotIdxA = 0;                  // IDA: smoke_dragSlotIdxA — L3-4 drag slot tracking
	int16 _dragSlotIdxB = 0;                  // IDA: smoke_dragSlotIdxB

	ZmbFeature *_targetZmbRunner = nullptr;   // IDA: smoke_targetZmbRunner (grid[7] for L3+)
	ZmbFeature *_sourceZmbRunner = nullptr;   // IDA: smoke_sourceZmbRunner (grid[8] for L3+)

	// === Display runners (L3-4 only) ===

	ZmbFeature *_displayRunnerArr[6] = {};    // IDA: smoke_displayRunnerArr — display runner slot tracking

	// === Attribute grid (L3-4) ===

	uint8 _attrGridPrimary[36] = {};          // IDA: smoke_attrGridPrimary (0x4B1D50)
	uint8 _attrGridSecondary[36] = {};        // IDA: smoke_attrGridSecondary (0x4B1D74)
	uint8 _attrGridMatchFlags[36] = {};       // IDA: smoke_attrGridMatchFlags

	// === Idle animation state ===

	uint32 _lastIdleFrameTime = 0;            // IDA: idle frame timing
	int16 _idleAnimCount = 0;                 // IDA: idle anim count
	bool _bIdleAnimActive = false;            // IDA: smoke_bIdleAnimActive
	uint32 _idlePoolState = 0;                // IDA: smoke_idlePoolState (bitmask for getNonRepeatRandom)

	// === Misc state ===

	int16 _resultHotspotIdx = 0;              // IDA: smoke_resultHotspotIdx
	int16 _savedSFXState = 0;                 // IDA: smoke_savedSFXState
};

} // End of namespace Mohawk

#endif
