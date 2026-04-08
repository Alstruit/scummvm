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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_TUNNELS_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_TUNNELS_H

#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Stone Cold Cave: puzzle page (ZoombiniPageType::kTunnels).
 * Route 1, Puzzle 2
 * 
 * Players must guide Zoombinis through one of four tunnels, by matching their attributes to the correct tunnel entrances.
 *
 * Difficulty levels:
 * - Level 1: 2 tunnels active, single-attribute rule (e.g., "has blue eyes")
 * - Level 2: 4 tunnels active, two single-attribute guards
 * - Level 3: 4 tunnels active, two dual-attribute guards (OR within category)
 * - Level 4: 4 tunnels active, two cross-category dual-attribute guards (AND)
 *
 * IDA entry: puzzleTunnels_459DCB (0x459dcb)
 */
class ZoombiniPuzzleTunnels : public ZoombiniPuzzle {
public:
	ZoombiniPuzzleTunnels(MohawkEngine_Zoombini *vm);
	~ZoombiniPuzzleTunnels() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;
	void onEveryFrame() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;

protected:
	void onGoButtonActivated() override;
	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;
	void endDrag(const Common::Point &dropPos);

private:
	void loadZoombinisFromPack();

	/** Initialize puzzle state variables. IDA: tunnels_initPuzzleState @ 0x459C5C */
	void initPuzzleState();

	/** Generate tunnel rules based on difficulty level. */
	void generateRules();

	/** Level 0: Single attribute rule. IDA: 0x45C859 */
	void setupLevel0_singleAttr();
	/** Level 1: Dual guards, single attribute each. IDA: 0x45CB51 */
	void setupLevel1_dualSingleAttr();
	/** Level 2: Dual guards, two conditions each (OR within same category). IDA: 0x45CCCD */
	void setupLevel2_dualDoubleAttr();
	/** Level 3: Dual guards, cross-category conditions (AND). IDA: 0x45D608 */
	void setupLevel3_crossCategoryAttr();

	/**
	 * Evaluate if a Zoombini matches a tunnel rule for a given zone.
	 * IDA: tunnels_evalAttrRule @ 0x45C65D
	 * @param snoid        The Zoombini to evaluate
	 * @param dropZone     The target tunnel zone (1-4)
	 * @param guardAMatch  [out] Whether guard A's first condition matched
	 * @return true if the Zoombini does NOT match the zone rule (rejection)
	 */
	bool evaluateRule(ZmbSnoid *snoid, int16 dropZone, bool &guardAMatch);

	/** Find which tunnel zone a position corresponds to. @return 1-4 or 0. */
	int16 getDropZone(const Common::Point &pos);

	/**
	 * Build animation queue entry for a Zoombini placement.
	 * IDA: tunnels_funcOnHover case 4 @ 0x45A9CF
	 */
	void handleZoombiniPlacement(ZmbSnoid *snoid, int16 zone,
	                             bool isRejection, bool guardAMatch, bool wasInSlot);

	// ========================================
	// Static Data Tables
	// ========================================

	static const Common::Point kSnoidPositions[16];
	static const Common::Point kTunnelEntryPositions[4];
	static const int16 kDoorIndices[4];
	static const Common::Point kScrsReplayPositions[4];
	static const Common::Point kGatePositions[4][16];
	static const int16 kHoverDataToGateType[8];
	static const int16 kSpawnOriginX[4];
	static const int16 kPreferredSlots[2][2];
	static const int16 kClickZoneRadius = 40;

	static const int16 kWrongPool0[10];
	static const int16 kCorrectHintSmall[11];
	static const int16 kWrongPool1[8];
	static const int16 kWrongPool2[8];
	static const int16 kWrongPool3[7];
	static const int16 kCorrectHintLarge[6];
	static const int16 kRejectPoolGate1[4];
	static const int16 kWrongPool4[6];

	// ========================================
	// Core State Variables
	// ========================================

	int16 _difficultyLevel = 0;      ///< IDA: word_4B7A12
	bool _puzzleActive = false;      ///< IDA: word_4B7A0C
	bool _processingFrame = false;   ///< IDA: word_4A768C
	int16 _enteredCount = 0;         ///< IDA: word_4B7A0E
	int16 _remainingCount = 0;       ///< IDA: word_4B7A14
	int16 _totalZmbCount = 0;        ///< IDA: word_4B7AE8
	int16 _level0GateBias = 0;       ///< IDA: word_4B7A10
	bool _postGameStarted = false;   ///< IDA: word_4B7A22
	bool _goButtonReady = false;     ///< IDA: word_4B7A24
	bool _animLocked = false;        ///< IDA: word_4B7A26
	int16 _setupPhase = 0;          ///< IDA: word_4B7A42

	// ========================================
	// Rule System
	// ========================================

	struct TunnelGuard {
		bool sideFlag = false;
		uint8 condCount = 0;
		uint8 attrType[2] = {};
		uint8 attrValue[2] = {};
	};

	int16 _guardCount = 0;
	TunnelGuard _guards[2];

	// ========================================
	// Per-Gate State
	// ========================================

	int16 _gateCorrectStreak[5] = {};  ///< IDA: word_4B7A38[0..4], indexed by zone 1-4
	int16 _gateOccupancy[4] = {};      ///< IDA: word_4B7AD4/D8/DA/D6 → gates 0-3
	uint16 _gateSlots[4][16] = {};     ///< IDA: word_4B7988/79C8/79E8/79A8
	int16 _wrongCountZone0 = 0;        ///< IDA: word_4B7ADC
	int16 _wrongCountZone2 = 0;        ///< IDA: word_4B7ADE

	// ========================================
	// Animation Queue System
	// ========================================

	struct AnimQueueEntry {
		uint16 runnerIdx = 0;
		int16 isRejection = 0;
		int16 stepCounter = 0;
		Common::Point pos;
		int16 walkScrsId = 0;
		int16 rejectScrsId = 0;
		int16 primaryRunner = 0;
		int16 primaryScrb = 0;
		int16 secondaryScrb = 0;
		int16 secondaryRunner = 0;
		int16 secondaryScrb1 = 0;
		int16 secondaryScrb2 = 0;
		int16 zoneIdx = 0;
	};

	AnimQueueEntry _animQueue[5];
	int16 _animQueueCount = 0;

	uint16 _pendingSoundRunner = 0;
	uint16 _pendingSoundScrbId = 0;
	bool _pendingSoundHasCallback = false;
	int16 _pendingBodyArrangement = 0;

	void appendAnimQueueEntry(const AnimQueueEntry &entry);
	void popAnimQueueEntry();
	void advanceAnimStep();
	void selectLevelRunners(int16 mode);
	int16 assignSlotWithPush(int16 side);
	int16 spawnPendingZoombinis();
	void playAmbientSound();
	void clearGateRenderFlag();

	void processSnoidAnimEvent(ZmbSnoid *snoid, int16 eventCode);
	void processGateAnimEvent(ZmbFeature *feature, int16 eventCode);
	ZmbSnoid *findIdlePackSnoid(uint16 snoidId);

	// ========================================
	// Feature Runners
	// ========================================

	ZmbFeature *_feedbackFeature = nullptr;
	ZmbFeature *_tunnelEntryFeatures[4] = {};
	ZmbFeature *_pathEffectFeature = nullptr;
	ZmbFeature *_doorAnimFeatures[4] = {};
	ZmbFeature *_mainPathFeature = nullptr;

	// ========================================
	// Ambient/Idle Animation
	// ========================================

	uint32 _idleAnimDeadline = 0;
	int16 _celebrationTarget = 0;
	int16 _celebrationsPlayed = 0;
	uint32 _celebrationTimer = 0;
	uint32 _celebrationInterval = 0;
	int16 _countdownVoiceId = 0;
	bool _countdownVoicePlaying = false;
	int16 _zmbEnteredVoiceId = 0;
	int16 _activeSoundResId = 0;
	Audio::SoundHandle _activeSndHandle;

	uint32 _poolStateWrongZone0 = 0;
	uint32 _poolStateCorrectSmall = 0;
	uint32 _poolStateWrongZone1 = 0;
	uint32 _poolStateWrongZone2 = 0;
	uint32 _poolStateWrongZone3 = 0;
	uint32 _poolStateCorrectLarge = 0;
	uint32 _poolStateWrongZone4 = 0;
	uint32 _poolStateRejectGate1 = 0;
	uint32 _poolStateWrongZone4b = 0;
	uint32 _poolStateIdleRunners = 0;
	uint32 _poolStateInitRunners = 0;
	uint32 _poolStateEndGameRunners = 0;
	uint32 _poolStateAdvanceA = 0;
	uint32 _poolStateAdvanceB = 0;
	uint32 _poolStateAdvanceGo = 0;
	uint32 _poolStateCelebration = 0;

	uint16 _sortedRunnerIds[16] = {};

	enum {
		kResSound996_DepartSFX = 996
	};
};

} // End of namespace Mohawk

#endif
