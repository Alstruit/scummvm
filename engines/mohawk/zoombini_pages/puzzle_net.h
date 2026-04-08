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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_NET_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_NET_H

#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Net puzzle page (ZoombiniPageType::kNet).
 *
 * Route 4, Puzzle 3: Zoombinis must navigate a sorting net with
 * attribute-based column filters. The puzzle uses 2-3 columns
 * depending on difficulty, with animated column slots.
 *
 * IDA entry: puzzleNet_4361D4 (0x4361d4)
 */
class ZoombiniInteractiveNet : public ZoombiniPuzzle {
public:
	ZoombiniInteractiveNet(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveNet() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;
	void onEveryFrame() override;
	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;

private:
	void loadZoombinisFromPack();
	void registerColumnRunners();

	// --- Core puzzle logic ---

	/** Generate attribute rule grids and slot assignments. IDA: net_generateAttrRules (0x437A36) */
	void generateAttrRules();
	/** Compute column sizes from zoombini count. IDA: net_computeColumnSizes (0x4393C4) */
	void computeColumnSizes();
	/** Find slot matching current column offsets. IDA: net_findSlotByAttrColumns (0x438C47) */
	int16 findSlotByAttrColumns();
	/** Update column offset and trigger animations. IDA: net_updateAttrColumnOffset (0x438108) */
	void updateAttrColumnOffset(int16 value, int16 columnGroup);
	/** Assign next zoombini to an open column. IDA: net_assignNextZmbToColumn (0x438017) */
	void assignNextZmbToColumn();
	/** Register zoombini display at a net slot. IDA: net_registerZmbAtSlot (0x438A84) */
	void registerZmbAtSlot(int16 slotIndex);
	/** Spawn zoombini SCRB at slot with bounce. IDA: net_spawnZmbAtSlot (0x439489) */
	void spawnZmbAtSlot(int16 slotIndex);

	// --- Animation event dispatch ---

	/** SCRB feature animation events. IDA: net_scrbAnimCallback (0x43105B) */
	void processScrbAnimEvent(ZmbFeature *feature, int16 eventCode);
	/** Zoombini snoid animation events. IDA: net_zmbAnimCallback (0x438EA1) */
	void processSnoidAnimEvent(ZmbFeature *feature, int16 eventCode);
	/** Events from SCRB features using the zmb callback. IDA: net_zmbAnimCallback routed via SCRB */
	void processZmbScrbAnimEvent(ZmbFeature *feature, int16 eventCode);

	// --- Render callbacks ---

	bool attrSlots_preRender(ZmbFeature *feature);
	ZmbRenderResult attrSlots_render(ZmbFeature *feature);

	// --- Static data tables ---

	static const Common::Point kSnoidPositions[16];
	static const Common::Point kSlotPositionsLow[25];   ///< IDA: word_4A2586, diff<=1
	static const Common::Point kSlotPositionsHigh[125];  ///< IDA: word_4A25EA, diff>1
	static const Common::Point kExitPositions[16];       ///< IDA: dword_4A27DE
	static const Common::Point kEntryStartPositions[3];  ///< IDA: dword_4A28E8 (event 4)
	static const Common::Point kEntryExitPositions[3];   ///< IDA: dword_4A28F4 (event 30)
	static const int16 kColOffsets1[5];                  ///< IDA: unk_4A28D4: {2,3,0,1,4}
	static const int16 kColOffsets2[5];                  ///< IDA: unk_4A28DE: {4,0,2,1,3}

	// --- Puzzle configuration ---

	/** Route difficulty level (0-3). IDA: net_difficultyLevel */
	int16 _difficultyLevel = 0;
	/** Total slots: 25 (diff<=1) or 125 (diff>1). IDA: net_totalSlotCount */
	int16 _totalSlotCount = 25;

	// --- Rule grids (puzzle solution tables) ---

	int16 _ruleGridA[125] = {};  ///< IDA: net_ruleGridA (0x4B0548)
	int16 _ruleGridB[125] = {};  ///< IDA: net_ruleGridB (0x4B0642)
	int16 _ruleGridC[125] = {};  ///< IDA: net_ruleGridC (0x4B073C)
	int16 _slotColumnAssign[125] = {};  ///< IDA: net_slotColumnAssign (0x4B087A)
	int16 _columnSizes[12] = {};  ///< IDA: net_columnSizes (0x4B0852)
	int16 _columnCount = 2;  ///< IDA: net_columnCount (0x4B0546)

	// --- Attribute labels ---

	int16 _attrRowLabel = 0;  ///< IDA: net_attrRowLabel (0x4B0848)
	int16 _attrColLabel = 0;  ///< IDA: net_attrColLabel (0x4B084A)
	int16 _seed = 0;          ///< IDA: seed (third label for diff>2)
	int16 _attrPermutationIdx = 0;  ///< IDA: net_attrPermutationIdx (0x4B0B38)

	// --- Attribute column offsets (player selection) ---

	int16 _randAttrColOffset[3] = {0, 0, 0};    ///< IDA: net_randAttrColOffset0/1/2
	int16 _prevAttrColOffset[3] = {-1, -1, -1};  ///< IDA: net_prevAttrColOffset0/1/2

	// --- Feature runners ---

	ZmbFeature *_columnScrbFeatures[5] = {};    ///< IDA: net_columnScrbRunners
	ZmbFeature *_entryScrbFeature = nullptr;    ///< IDA: net_entryScrbRunner
	ZmbFeature *_labelScrbFeature = nullptr;    ///< IDA: net_labelScrbRunner
	ZmbFeature *_attrAnimScrbFeature = nullptr; ///< IDA: net_attrAnimScrbRunner
	ZmbFeature *_feedbackScrbFeature = nullptr; ///< IDA: net_feedbackScrbRunner
	ZmbFeature *_attrColScrbFeatures[3] = {};   ///< IDA: net_attrCol0/1/2ScrbRunner
	ZmbFeature *_exitScrbFeature = nullptr;     ///< IDA: net_exitScrbRunner

	// --- Slot display tracking ---

	ZmbFeature *_slotScrbFeatures[125] = {};       ///< IDA: net_slotScrbRunners
	ZmbFeature *_activeSlotFeatures[16] = {};      ///< IDA: net_activeSlotRunners
	int16 _slotRunnerCount = 0;                    ///< IDA: net_slotRunnerCount

	// --- Animation state machine ---

	bool _exitAnimActive = false;      ///< IDA: net_exitAnimActive
	int16 _exitAnimStep = 0;           ///< IDA: net_exitAnimStep
	int16 _remainingExitSteps = 0;     ///< IDA: net_remainingExitSteps
	bool _exitRunnerActive = false;    ///< Tracks exit SCRB animation completion
	bool _labelAnimRunning = false;    ///< Tracks label SCRB animation completion
	bool _sortAnimRunning = false;     ///< IDA: net_sortAnimRunner != 0
	int16 _pendingColumnSetup = 0;     ///< IDA: net_pendingColumnSetup
	bool _pendingAttrRunning = false;  ///< IDA: net_pendingAttrRunner != 0
	bool _activeAttrRunning = false;   ///< IDA: net_activeAttrRunner != 0
	bool _activeAttrAnim1Running = false;  ///< IDA: net_activeAttrAnim1
	bool _activeAttrAnim2Running = false;  ///< IDA: net_activeAttrAnim2
	bool _activeAttrAnim3Running = false;  ///< IDA: net_activeAttrAnim3
	int16 _columnAnimDone = 0;         ///< IDA: net_columnAnimDone
	bool _columnOpenAnimRunning = false;   ///< IDA: net_columnOpenAnimRunner
	bool _zmbEntryAnimRunning = false; ///< IDA: net_zmbEntryAnimRunner

	// --- Column/walk tracking ---

	uint16 _columnSlotSnoidIds[3] = {};  ///< IDA: net_columnSlotRunners
	uint16 _walkSlotSnoidIds[3] = {};    ///< IDA: net_walkSlotRunners
	int16 _activeColumnIdx = 0;    ///< IDA: net_activeColumnIdx
	int16 _pendingZmbIndex = -1;   ///< IDA: net_pendingZmbIndex
	int16 _nextZmbToAssign = 0;    ///< IDA: net_nextZmbToAssign
	int16 _activeWalkCount = 0;    ///< IDA: net_activeWalkCount
	uint16 _activeZmbSnoidId = 0;  ///< IDA: net_activeZmbRunner
	uint16 _lastLinkedSnoidId = 0; ///< IDA: net_lastLinkedRunner
	uint16 _exitingZmbSnoidId = 0; ///< IDA: net_exitingZmbRunner

	// --- Bounce animation ---

	int16 _bounceX = 0;       ///< IDA: net_bounceX
	int16 _bounceY = 0;       ///< IDA: net_bounceY
	int16 _bounceDeltaX = 0;  ///< IDA: net_bounceDeltaX
	int16 _bounceDeltaY = 0;  ///< IDA: net_bounceDeltaY
	int16 _bounceCounter = 0; ///< IDA: net_bounceCounter

	// --- Scoring and progress ---

	int16 _columnMatchCount = 0;   ///< IDA: net_columnMatchCount
	int16 _sortedZmbCount = 0;     ///< IDA: net_sortedZmbCount
	int16 _rejectedCount = 0;      ///< IDA: net_rejectedCount
	int16 _submitCount = 0;        ///< IDA: net_submitCount
	int16 _submitActiveFlag = 0;   ///< IDA: net_submitActiveFlag
	int16 _currentSlotIndex = 0;   ///< IDA: net_currentSlotIndex

	// --- Flags ---

	bool _inputLocked = false;      ///< IDA: net_inputLocked
	bool _firstRoundFlag = false;   ///< IDA: net_firstRoundFlag
	bool _noMatchFlag = false;      ///< IDA: net_noMatchFlag
	int16 _exitScrbOffset = 0;      ///< IDA: net_exitScrbOffset
	int16 _hotspotPositionFlag = 0; ///< IDA: net_hotspotPositionFlag
	int16 _zmbsAtColumns = 0;       ///< IDA: net_zmbsAtColumns
	int16 _allColumnsExhausted = 0; ///< IDA: net_allColumnsExhausted
	int16 _zmbQueueCount = 0;       ///< IDA: net_zmbQueueCount
	int16 _zmbReadyCount = 0;       ///< IDA: net_zmbReadyCount
	bool _zmbWalkPending = false;   ///< IDA: net_zmbWalkPending
	bool _attrColumnsReady = false; ///< IDA: net_attrColumnsReady
	bool _bAdvanceReady = false;    ///< IDA: net_advanceReady

	// --- Exit/sort tracking ---

	int16 _exitPositionIdx = 0;    ///< IDA: net_exitPositionIdx
	int16 _sortAnimType = 0;       ///< IDA: MEMORY[0x4A2860] sort SCRB lookup
	bool _hintPending = false;     ///< IDA: word_4A22B4

	// --- Timing ---

	uint32 _lastSubmitFrame = 0;  ///< IDA: dword_4B0B40

	// --- Dirty flags for rendering ---

	bool _advanceButtonDirty = false;  ///< IDA: unk_4A28AC
	bool _columnLabelDirty = false;    ///< IDA: unk_4A28AE

	// --- Pending body arrangement ---

	int16 _pendingBodyArrangement = 0;  ///< IDA: net_pendingAnimShape

	// --- Idle animation state ---

	bool _idleAnimTrigger = false;  ///< IDA: net_idleAnimTrigger
	int16 _idleAnimCount = 0;      ///< IDA: net_idleAnimCount
	int16 _idleAnimMax = 0;        ///< IDA: net_idleAnimMax
	uint32 _idleAnimPoolState = 0; ///< IDA: dword_4B0B44
	uint32 _idleAnimLastFrame = 0; ///< IDA: dword_4B0B3C
	int16 _loadedZmbCount = 0;     ///< IDA: net_zoombiniCount
	bool _roundCompletedFlag = false;  ///< IDA: net_roundCompletedFlag

	enum {
		kResSound996_DepartSFX = 996
	};
};

} // End of namespace Mohawk

#endif
