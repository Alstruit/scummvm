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

#ifndef MOHAWK_ZOOMBINI_PAGES_SMOKE_H
#define MOHAWK_ZOOMBINI_PAGES_SMOKE_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Smoke puzzle page (ZoombiniPageType::kSmoke).
 *
 * Route 3, Puzzle 3: Zoombinis must be sorted by comparing attributes
 * on a smoke-stack cliffside. The puzzle uses a custom stack-based
 * positioning system instead of the standard walk-in layout.
 * NON-STANDARD: Does not use layoutStaticAndWalkIn().
 *
 * IDA entry: smoke_init (0x44983c)
 */
class ZoombiniInteractiveSmoke : public ZoombiniInteractive {
public:
	ZoombiniInteractiveSmoke(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveSmoke() override;

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

private:
	void loadZoombinisFromPack();

	static const Common::Point kSnoidPositions[20];

	/** DRAW_ON_REG position for answer display. IDA: stru_4A400C */
	static const Common::Point kDrawOnRegPosition;

	/** Route difficulty level + 1 (1-4). IDA: smoke_difficultyLevel */
	int16 _difficultyLevel = 0;

	/**
	 * Orientation flag for runner attribute assignment.
	 * Levels 1-2: 0; Levels 3-4: 2.
	 * IDA: runner+241 in smoke_assignRunnerAttrsForLevel
	 */
	uint8 _orientation = 0;

	/**
	 * Seen attribute history arrays for cross-level persistence.
	 * Cleared at level 1, used for 65% reuse at levels 3-4.
	 * IDA: smoke_seenAttrA, smoke_seenAttrB
	 */
	uint8 _seenAttrA[4] = {};
	uint8 _seenAttrB[4] = {};

	/** SCRB overlay animation (11013 or 11011). IDA: smoke_scrbOverlayAnim */
	ZmbFeature *_overlayAnimFeature = nullptr;
	/** SCRB 11076 — extra feature for diff 1/2 only. IDA: smoke_scrbLevel12Extra */
	ZmbFeature *_level12ExtraFeature = nullptr;
	/** SCRB 11006 — cliff left animation. IDA: smoke_scrbCliffLeft */
	ZmbFeature *_cliffLeftFeature = nullptr;
	/** SCRB 11007 — cliff right animation. IDA: smoke_scrbCliffRight */
	ZmbFeature *_cliffRightFeature = nullptr;
	/** Main animation (diff 1/2: 11024, diff 3/4: 11028). IDA: smoke_scrbMainAnim */
	ZmbFeature *_mainAnimFeature = nullptr;
	/** Smoke stack A animation. IDA: smoke_scrbSmokeStackA */
	ZmbFeature *_smokeStackAFeature = nullptr;
	/** Smoke stack B animation (diff 3/4 only). IDA: smoke_scrbSmokeStackB */
	ZmbFeature *_smokeStackBFeature = nullptr;
	/** Second animation. IDA: smoke_scrbSecondAnim */
	ZmbFeature *_secondAnimFeature = nullptr;
	/** SCRB 11018 — compare A animation. IDA: smoke_scrbCompareA */
	ZmbFeature *_compareAFeature = nullptr;
	/** SCRB 11019 — compare B animation. IDA: smoke_scrbCompareB */
	ZmbFeature *_compareBFeature = nullptr;
	/** SCRB 11009 — background overlay. IDA: smoke_scrbBgOverlay */
	ZmbFeature *_bgOverlayFeature = nullptr;
	/** SCRB 11036 — rejection animation. IDA: smoke_scrbRejection */
	ZmbFeature *_rejectionFeature = nullptr;
	/** SCRB 11008 — background. IDA: smoke_scrbBackground */
	ZmbFeature *_backgroundFeature = nullptr;
	/** SCRB 11002 — answer zone animation. IDA: smoke_scrbAnswerZone */
	ZmbFeature *_answerZoneFeature = nullptr;
	/** SCRB 11077 — holding area. IDA: smoke_scrbHoldingArea */
	ZmbFeature *_holdingAreaFeature = nullptr;
	/** SCRB 11001 — DRAW_ON_REG (diff < 3 only). IDA: scrb_drawOnRegRunnerIdxArr[0] */
	ZmbFeature *_drawOnRegFeature = nullptr;

	// =========================================================================
	// Gameplay methods
	// =========================================================================

	/**
	 * Build smoke column runners for attribute display.
	 * IDA: smoke_buildRunnerStacks (0x44DBE2)
	 * Dispatches to spawnStackRunners with per-difficulty configurations.
	 */
	void buildRunnerStacks();

	/**
	 * Spawn stack runners of a given type.
	 * IDA: smoke_spawnStackRunners (0x44DC7B)
	 * @param count Number of runners to spawn
	 * @param runnerType Runner category (1=cliff, 2=level2, 3=grid, 4=exit, 5=bottom)
	 */
	void spawnStackRunners(int16 count, int16 runnerType);

	/**
	 * Select a question Zoombini as template.
	 * IDA: smoke_selectQuestionZmb (0x44D372)
	 * For levels 1-2: picks one random Zoombini, copies attrs to _questionAttrs.
	 * For levels 3+: copies pair of attrs.
	 */
	void selectQuestionZmb();

	/**
	 * Copy pair of Zoombini attrs to compare buffer.
	 * IDA: smoke_copyPairToCompareBuffer (0x44D459)
	 * @return 0=empty, 1=one present, 2=both present
	 */
	int16 copyPairToCompareBuffer();

	/**
	 * Generate attribute rules for the current runner set.
	 * IDA: smoke_generateAttrGrid (0x44E181) / smoke_assignRunnerAttrsForLevel (0x44D67C)
	 */
	void generateRunnerAttributes();

	/**
	 * Assign attribute values to a single smoke column runner.
	 * IDA: smoke_assignRunnerAttrsForLevel (0x44D67C)
	 * @param runnerIdx Index into the appropriate runner array
	 * @param runnerType Runner type category
	 */
	void assignRunnerAttrs(int16 runnerIdx, int16 runnerType);

	/**
	 * Place a Zoombini on the smoke column.
	 * IDA: smoke_bPlaceZmb handler in smoke_onHover
	 */
	void placeZoombiniOnColumn();

	/**
	 * Play rejection animation for an incorrectly placed Zoombini.
	 * IDA: smoke_playZmbRejectedAnim (0x44CA52)
	 */
	void playRejectedAnimation();

	/**
	 * Start the next comparison display sequence.
	 * IDA: smoke_startNextCompareSequence (0x44C91A)
	 */
	void startNextCompareSequence();

	/**
	 * Reset and reinitialize the level for next round.
	 * IDA: smoke_resetAndReinitLevel (0x44BBF0)
	 */
	void resetAndReinitLevel();

	/**
	 * Handle level 4 multi-phase transitions.
	 * IDA: smoke_handleFrameTransition (0x44D281)
	 */
	void handleFrameTransition();

	/**
	 * Process SCRB animation dispatch events.
	 * Central animation callback for smoke features.
	 * IDA: smoke_scrbAnimDispatch (0x44CB72)
	 * @param feature The feature triggering the event
	 * @param eventCode Frame/event code from SCRB script
	 */
	void processAnimDispatchEvent(ZmbFeature *feature, int16 eventCode);

	/**
	 * Load SCRB onto the answer zone runner.
	 * IDA: smoke_loadSCRBOnAnswerRunner (0x44BA3D)
	 */
	void loadScrbOnAnswerRunner(uint16 scrbId);

	/**
	 * Get drop zone index from click position.
	 * For levels 1-2: single drop zone check.
	 * For levels 3-4: 3x3 grid per smoke stack.
	 * @return drop zone index (>=0), or -1 if none
	 */
	int16 getDropZoneAtPoint(const Common::Point &pos) const;

	/**
	 * Check if a Zoombini matches the target smoke column.
	 * @param zmbIdx Index of Zoombini in queue
	 * @param columnIdx Smoke column to test
	 * @return true if attributes match
	 */
	bool testColumnMatch(int16 zmbIdx, int16 columnIdx) const;

	/** Reload SCRB animation on a feature. */
	void reloadScrbAnimation(ZmbFeature *feature, uint16 scrbId);

	// =========================================================================
	// Gameplay state
	// =========================================================================

	/** Puzzle is active and processing. IDA: smoke_bInitialized (0x4B1E64) */
	bool _puzzleActive = false;

	/** Reentrancy guard for onEveryFrame. */
	bool _processingFrame = false;

	/** Number of Zoombinis in this puzzle round. IDA: smoke_zmbCount (0x4B1D02) */
	int16 _zmbCount = 0;

	/** Index of current Zoombini being processed. IDA: smoke_currentZmbIdx (0x4B1E08) */
	int16 _currentZmbIdx = 0;

	/** Number of Zoombinis placed on cliffs so far. IDA: smoke_placedZmbCount (0x4B1D40) */
	int16 _placedZmbCount = 0;

	/** Number of Zoombinis loaded onto cliff display. IDA: smoke_loadedOnCliffCount (0x4B1D44) */
	int16 _loadedOnCliffCount = 0;

	/** Answer display state: 0=hidden, 1=question, 2=idle. IDA: smoke_answerState (0x4B1E28) */
	int16 _answerState = 2;

	/** Whether answer display is visible. IDA: smoke_bShowAnswer (0x4B1E26) */
	bool _bShowAnswer = false;

	/** Current comparison pair index. IDA: smoke_compareIdx (0x4B1E10) */
	int16 _compareIdx = 0;

	/** Whether comparison order is swapped. IDA: smoke_bCompareSwapped (0x4B1E14) */
	bool _bCompareSwapped = false;

	/** Level 4 transition phase counter (3→0). IDA: smoke_transitionPhase (0x4B1E18) */
	int16 _transitionPhase = 3;

	// --- Event flags (set by anim callback, consumed by frame handler) ---

	/** Trigger Zoombini placement. IDA: smoke_bPlaceZmb (0x4B1E0A) */
	bool _bPlaceZmb = false;

	/** Trigger runner linking. IDA: smoke_bLinkRunners (0x4B1E1A) */
	bool _bLinkRunners = false;

	/** Trigger SCRB reload. IDA: smoke_bReloadScrb (0x4B1E1C) */
	bool _bReloadScrb = false;

	/** Trigger level reset. IDA: smoke_bResetLevel (0x4B1E1E) */
	bool _bResetLevel = false;

	/** Trigger result display. IDA: smoke_bShowResults (0x4B1E24) */
	bool _bShowResults = false;

	/** Trigger main runner reload. IDA: smoke_bReloadMainRunner (0x4B1D18) */
	bool _bReloadMainRunner = false;

	/** Whether celebration animations are active. IDA: smoke_bIdleAnimActive */
	bool _celebrationActive = false;

	/** Interaction lock during animations. */
	bool _interactionLocked = false;

	/** Current Zoombini being dragged/placed. IDA: smoke_currentDragZmb */
	ZmbSnoid *_currentDragZmb = nullptr;

	/** Runner toggle flag for smoke stack A/B switching. IDA: smoke_bRunnerToggle */
	bool _bRunnerToggle = false;

	/** Whether exit gate is enabled. IDA: smoke_bExitGateEnabled */
	bool _bExitGateEnabled = false;

	/** Exit animation active. IDA: smoke_exitAnimActive */
	bool _exitAnimActive = false;

	/** Exit animation step counter. IDA: smoke_exitAnimStep */
	int16 _exitAnimStep = 0;

	/** Exit animation total steps. IDA: smoke_remainingExitSteps */
	int16 _remainingExitSteps = 0;

	/** Celebration animation counter. */
	int16 _celebrationsPlayed = 0;

	/** Max celebration animations. IDA: smoke_idleAnimMax */
	int16 _celebrationTarget = 3;

	/** Celebration frame counter. */
	uint32 _nextCelebrationFrame = 0;

	// --- Question/Answer attributes ---

	/** Question Zoombini attributes (4 bytes: hair, eyes, nose, feet). IDA: stru_4B1D0C[0..7] */
	uint8 _questionAttrs[8] = {};

	/** Result of question Zoombini selection. IDA: smoke_questionResult (0x4B1E0E) */
	int16 _questionResult = 0;

	// --- Runner arrays for stack-based positioning ---

	/** Smoke column display runners (up to 20). IDA: smoke_smokeColumnRunnerArr (0x4B1C88) */
	ZmbFeature *_smokeColumnRunners[20] = {};
	int16 _smokeColumnCount = 0;

	/** Zoombini runners placed on cliff (up to 20). IDA: smoke_zmbOnCliffArr (0x4B1CB0) */
	ZmbSnoid *_zmbOnCliff[20] = {};

	/** Zoombini queue for processing (up to 21). IDA: smoke_zmbQueueArr (0x4B1CD8) */
	uint16 _zmbQueue[21] = {};
	int16 _zmbQueueSize = 0;

	/** Cliff face runners (type 1, up to 20). IDA: smoke_cliffRunnerArr (0x4B1D46) */
	ZmbFeature *_cliffRunners[20] = {};
	int16 _cliffRunnerCount = 0;

	/** Level 2 specific runners (type 2, up to 6). IDA: smoke_level2RunnerArr (0x4B1D6E) */
	ZmbFeature *_level2Runners[6] = {};
	int16 _level2RunnerCount = 0;

	/** Grid runners for levels 3-4 (type 3, up to 9). IDA: smoke_gridRunnerArr (0x4B1D8E) */
	ZmbFeature *_gridRunners[9] = {};
	int16 _gridRunnerCount = 0;

	/** Exit path runners (type 4, up to 4). IDA: smoke_exitRunnerArr (0x4B1D7A) */
	ZmbFeature *_exitRunners[4] = {};
	int16 _exitRunnerCount = 0;

	/** Bottom display runners (type 5, up to 2). IDA: smoke_bottomRunnerArr (0x4B1D80) */
	ZmbFeature *_bottomRunners[2] = {};
	int16 _bottomRunnerCount = 0;

	// --- Attribute grid for levels 3-4 ---

	/** 9x4 grid of primary attributes. IDA: smoke_attrGridHairEyes (0x4B1E9E) */
	uint8 _attrGridPrimary[36] = {};

	/** 9x4 grid of secondary attributes. IDA: smoke_attrGridNoseLegs (0x4B1EE6) */
	uint8 _attrGridSecondary[36] = {};

	/** 9x4 grid of match flags. IDA: smoke_attrGridMatchFlags (0x4B1F2E) */
	uint8 _attrGridMatchFlags[36] = {};

	/** Runner attribute storage: 4 attrs per runner. */
	uint8 _runnerAttrs[20][4] = {};

	/** Match count per runner. */
	uint8 _runnerMatchCount[20] = {};

	/** Has-match flag per runner. */
	bool _runnerHasMatch[20] = {};

	/** Shuffled row order for grid display. IDA: smoke_permutation (0x4B1E3C) */
	int16 _permutation[8] = {};

	/** Level 1 attribute assignment history. IDA: smoke_level1AttrHistory (0x4B1DA8) */
	uint8 _level1AttrHistory[8] = {};

	/** Level 2 attribute assignment history. IDA: smoke_level2AttrHistory (0x4B1DB0) */
	uint8 _level2AttrHistory[8] = {};

	// --- SCRB animation resource IDs (per-difficulty) ---

	/** 4 animation SCRB IDs. IDA: smoke_scrbAnimIdArr (0x4B1DE8) */
	uint16 _scrbAnimIdArr[4] = {};

	/** Zoombini animation base IDs. IDA: smoke_scrbZmbAnimIdArr (0x4B1DF0) */
	uint16 _scrbZmbAnimIdArr[4] = {};

	/** Smoke stack A/B resource IDs. */
	uint16 _scrbSmokeStackResA = 0;
	uint16 _scrbSmokeStackResB = 0;

	// --- Drop zone rects ---

	/** Drop zone rectangles for column placement. */
	Common::Rect _dropZoneRects[20];
	int16 _dropZoneCount = 0;
};

} // End of namespace Mohawk

#endif
