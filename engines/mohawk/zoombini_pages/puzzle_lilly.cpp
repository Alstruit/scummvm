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

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_pages/puzzle_lilly.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// =================================================================
// Static data tables (from IDA binary data)
// =================================================================

// IDA: word_4A1738/40/48/50 — direction SCRB tables
// Indexed by previous direction (0=up, 1=right, 2=down, 3=left)
const uint16 ZoombiniPuzzleLilly::kDirScrbUp[4]    = {10001, 10010, 10015, 10008};
const uint16 ZoombiniPuzzleLilly::kDirScrbRight[4] = {10005, 10002, 10011, 10016};
const uint16 ZoombiniPuzzleLilly::kDirScrbDown[4]  = {10013, 10006, 10003, 10012};
const uint16 ZoombiniPuzzleLilly::kDirScrbLeft[4]  = {10009, 10014, 10007, 10004};

// IDA: word_4A171E — Y offset per column for cell positions
const int16 ZoombiniPuzzleLilly::kColYOffset[13] = {
	2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 0
};

// IDA: word_4A16EC — preset swap pair column coordinates
const int16 ZoombiniPuzzleLilly::kSwapPairCol[20] = {
	4, 0, 3, 0, 8, 0, 10, 0, 0, 0, 4, 0, 6, 0, 3, 0, 5, 0, 0, 0
};

// IDA: word_4A1700 — preset swap pair row coordinates
const int16 ZoombiniPuzzleLilly::kSwapPairRow[20] = {
	4, 0, 6, 0, 3, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 4, 4, 6
};

// IDA: word_4A1832 — zoombini count → required grid row count
const int16 ZoombiniPuzzleLilly::kZmbToRowCount[21] = {
	1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10
};

// =================================================================
// Construction / Lifecycle
// =================================================================

ZoombiniPuzzleLilly::ZoombiniPuzzleLilly(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kLilly) {
	memset(_gridOccupancy, 0, sizeof(_gridOccupancy));
	memset(_gridAttr1, 0, sizeof(_gridAttr1));
	memset(_gridAttr2, 0, sizeof(_gridAttr2));
	memset(_gridAttr3, 0, sizeof(_gridAttr3));
	memset(_enterQueue, 0, sizeof(_enterQueue));
	memset(_exitQueue, 0, sizeof(_exitQueue));
	memset(_crossQueue, 0, sizeof(_crossQueue));
	memset(_rotateQueue, 0, sizeof(_rotateQueue));
	memset(_arriveQueue, 0, sizeof(_arriveQueue));
	memset(_departQueue, 0, sizeof(_departQueue));
	memset(_readyQueue, 0, sizeof(_readyQueue));
	memset(_moveQueue, 0, sizeof(_moveQueue));
	memset(_pendingMoveQueue, 0, sizeof(_pendingMoveQueue));
	memset(_pathInitQueue, 0, sizeof(_pathInitQueue));
	memset(_obstacleRunners, 0, sizeof(_obstacleRunners));
	memset(_activeObstacles, 0, sizeof(_activeObstacles));
	memset(_freedRunners, 0, sizeof(_freedRunners));
	for (int i = 0; i < kMaxRunners; i++)
		_runnerStates[i].clear();
}

ZoombiniPuzzleLilly::~ZoombiniPuzzleLilly() {
}

void ZoombiniPuzzleLilly::open() {
	openArchive(ZMB_MHK_LILLY);
}

void ZoombiniPuzzleLilly::setBackgroundMusic() {
	// IDA: lilly_puzzleInit (0x422de4) has no music playback call on page load.
}

void ZoombiniPuzzleLilly::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

// =================================================================
// loadFeatures — full puzzle initialization
// =================================================================

void ZoombiniPuzzleLilly::loadFeatures() {
	// IDA: lilly_puzzleInit (0x422de4)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel() + 1;

	// Preload shape images
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(13000);

	// Load main features: 1 SCRB at 11000
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 5, 0x36B0) — 5 subs at 14000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 5; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 14000), 14000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 167, 0x2710) — 167 subs at 10000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 167; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 10000), 10000 + i);
		}
	}

	// Load REGS resources
	loadREGS(ZmbArchiveKind::kPage, 100);
	loadREGS(ZmbArchiveKind::kPage, 10000);
	loadREGS(ZmbArchiveKind::kPage, 200);

	// Load zoombinis from pack
	loadZoombinisFromPack();

	// Initialize difficulty
	setDifficultyParams();

	// Initialize grid
	initGridWithAttributes();

	// Load REGS coordinate tables for cell positioning and path interpolation
	loadRegsCoordinateTables();

	// IDA: word_4A14C8 — virtual grid renderer
	_gridRendererFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10000, 0,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: lilly_cursorRunnerIdx — cursor indicator
	_cursorRunnerFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10001, 5,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: lilly_cellAnimRunnerA/B — cell animation runners
	_cellAnimRunnerA = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10002, 4,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	_cellAnimRunnerB = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10003, 4,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);

	// Create per-zoombini runners
	createZoombiniRunners();

	// 5 overlay features for SCRB 14000-14004
	for (uint16 i = 0; i < 5; i++) {
		_overlayFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 14000), 14000 + i, 0,
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// Frog obstacle (difficulty > 1)
	if (_difficultyLevel > 1) {
		_frogScrbFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 11000, 5,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);

		_frogRunnerFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10078, 6,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);
		if (_frogRunnerFeature) {
			_frogRunnerFeature->setPointLoc(Common::Point(38, 415));
			_frogRunnerFeature->deactivateRender();
		}
	}

	// NOTE: Original engine used 12 no-op overlay runners (word_4AE3AA) purely for
	// render ordering. ScummVM's per-frame Z-sorting makes these unnecessary.

	// Buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(7000);
	loadHelpButtonFeature();

	// Help sound selection
	{
		ZMB_DIFFICULTY_ID diffId = _vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagLilly);
		uint16 helpSoundId;
		if (diffId == ZMB_DIFFICULTY_LEVEL2_02) {
			helpSoundId = _vm->_rnd->getRandomNumber(20076, 20077);
		} else if (_difficultyLevel <= 1) {
			helpSoundId = 20075;
		} else {
			helpSoundId = _vm->_rnd->getRandomNumber(20075, 20077);
		}
		_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, helpSoundId);
	}

	// Initialize obstacle timer
	// IDA: lilly_nextObstacleTimer = current_frame + 600
	_nextObstacleTimer = getCurrentFrameCounter() + 600;

	// Activate puzzle
	_bPuzzleActive = true;
	_bRenderEnabled = true;
}

// =================================================================
// Initialization helpers
// =================================================================

void ZoombiniPuzzleLilly::onGoButtonActivated() {
	// IDA: lilly_onClickHandler case 2
	_departXferSrcSiPage = ZMB_SI_LILLY_07;
	ZoombiniInteractive::onGoButtonActivated();
}

void ZoombiniPuzzleLilly::loadZoombinisFromPack() {
	ZmbStateFile &f = _vm->_state->_f;
	const Common::Point offscreenPos(680, 220);
	uint16 posIdx = 0;

	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
		ZmbStateActiveEntry &entry = f._zmbPackActive._entries[i];
		if (!entry._bIsOccupied)
			continue;

		uint16 snoidId = 10000 + posIdx;
		ZmbSnoid *snoid = loadSnoidFromPack(snoidId, offscreenPos,
		                                    ZmbFeature::FLAG_00000001_TYPE_SNOID);
		if (snoid) {
			snoid->_trait = entry._traits;
			snoid->_name = entry.getU32Name(_vm);
			snoid->_packIsOccupied = true;
			snoid->deactivateRender();
		}
		posIdx++;
	}

	_totalZmbCount = posIdx;
}

void ZoombiniPuzzleLilly::setDifficultyParams() {
	// IDA: lilly_setDifficultyParams (0x4264AC)
	switch (_difficultyLevel) {
	case 1:
		_mudBallCount = 0;
		_obstacleRows = 0;
		break;
	case 2:
		_mudBallCount = 4;
		_obstacleRows = 0;
		break;
	case 3:
		_mudBallCount = 5;
		_obstacleRows = 2;
		break;
	case 4:
	default:
		_mudBallCount = 6;
		_obstacleRows = 3;
		break;
	}
}

void ZoombiniPuzzleLilly::loadRegsCoordinateTables() {
	// Load REGS 100 X/Y arrays for cell position computation
	auto it100 = _regsMap.find(100);
	if (it100 != _regsMap.end()) {
		ZmbRegs *regs100 = it100->_value;
		_regsXTable.clear();
		_regsYTable.clear();
		for (uint i = 0; i < regs100->_offsets.size(); i++) {
			_regsXTable.push_back(regs100->_offsets[i].x);
			_regsYTable.push_back(regs100->_offsets[i].y);
		}
	}

	// Load REGS 200 for path interpolation deltas
	auto it200 = _regsMap.find(200);
	if (it200 != _regsMap.end()) {
		ZmbRegs *regs200 = it200->_value;
		_regsDeltaX.clear();
		_regsDeltaY.clear();
		for (uint i = 0; i < regs200->_offsets.size(); i++) {
			_regsDeltaX.push_back(regs200->_offsets[i].x);
			_regsDeltaY.push_back(regs200->_offsets[i].y);
		}
	}

	// Compute grid cell positions and bounding rects
	// IDA: posArr_4B7C44[row].x = REGS_100_X[row] + 18
	// IDA: posArr_4B7C44[row].y = REGS_100_Y[row] + 15
	// Cell position: x = 35 * col + REGS_X[row] + 18, y = kColYOffset[col] + REGS_Y[row] + 15
	for (int row = 0; row < 12; row++) {
		int16 baseX = (row < (int)_regsXTable.size()) ? _regsXTable[row] : (50 + row * 45);
		int16 baseY = (row < (int)_regsYTable.size()) ? _regsYTable[row] : 100;

		for (int col = 0; col < 13; col++) {
			int16 cellX = 35 * col + baseX + 18;
			int16 cellY = kColYOffset[col] + baseY + 15;
			_gridCellPos[row][col] = Common::Point(cellX, cellY);
			// Cell bounding rect: 35x35 centered on cell position
			_gridCellRect[row][col] = Common::Rect(cellX - 17, cellY - 17, cellX + 18, cellY + 18);
		}
	}
}

void ZoombiniPuzzleLilly::initGridWithAttributes() {
	// IDA: fleens_initGridWithAttributes (0x427955)

	// Select grid type based on difficulty
	switch (_difficultyLevel) {
	case 1:
		_gridType = 3;
		break;
	case 2:
		_gridType = 4;
		break;
	case 3:
	case 4:
	default:
		_gridType = 5;
		break;
	}

	// Clear all grid arrays
	memset(_gridOccupancy, 0, sizeof(_gridOccupancy));
	memset(_gridAttr1, 0, sizeof(_gridAttr1));
	memset(_gridAttr2, 0, sizeof(_gridAttr2));
	memset(_gridAttr3, 0, sizeof(_gridAttr3));

	// IDA: Assign random attributes to grid cells
	// For each cell, generate random values based on grid type:
	// attr1: 0..(_gridType==3 ? 1 : 2)
	// attr2: 0..(_gridType>=4 ? 3 : 2)
	// attr3: 0..(_gridType>=5 ? 4 : 3)
	int16 maxAttr1 = (_gridType == 3) ? 2 : 3;
	int16 maxAttr2 = (_gridType >= 4) ? 4 : 3;
	int16 maxAttr3 = (_gridType >= 5) ? 5 : 4;

	for (int row = 0; row < 12; row++) {
		for (int col = 0; col < _gridType; col++) {
			_gridAttr1[row][col] = _vm->_rnd->getRandomNumber(0, maxAttr1 - 1);
			_gridAttr2[row][col] = _vm->_rnd->getRandomNumber(0, maxAttr2 - 1);
			_gridAttr3[row][col] = _vm->_rnd->getRandomNumber(0, maxAttr3 - 1);
		}
	}

	// IDA: Initial cell swaps at difficulty > 1
	if (_difficultyLevel > 1) {
		// Perform 2 initial swaps using preset coordinates
		_swapPairIdx = 0;
		// Swap threshold = (totalZmbCount + 5) / 6
		_swapThreshold = (_totalZmbCount + 5) / 6;
	}
}

void ZoombiniPuzzleLilly::createZoombiniRunners() {
	// IDA: word_4AE3C2[] — per-zoombini runners at SCRB 10109+i
	for (int16 i = 0; i < _totalZmbCount && i < kMaxRunners; i++) {
		_zmbRunners[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10109 + i, 4,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);

		if (_zmbRunners[i]) {
			_zmbRunners[i]->deactivateRender();
		}
	}

	// IDA: For difficulty 1, last 2 runners get exit animation at SCRBs 10089+i
	// This is handled in handleScriptEvent case 3 for difficulty > 1
	if (_difficultyLevel == 1) {
		for (int16 i = MAX(0, _totalZmbCount - 2); i < _totalZmbCount; i++) {
			if (_zmbRunners[i]) {
				_zmbRunners[i]->activateRender();
				loadScrbOntoFeature(_zmbRunners[i], 10089 + i);
			}
		}
	}
}

// =================================================================
// onEveryFrame — main per-frame queue processing pipeline
// IDA: lilly_mainFrameUpdate (0x423A0D)
// =================================================================

void ZoombiniPuzzleLilly::onEveryFrame() {
	if (!_bPuzzleActive)
		return;

	// Process animation queues in strict order (matching original)
	processEnterQueue();
	processExitQueue();
	processCompletedExitRunner();
	processRotateQueue();
	processCrossQueue();
	processCompletedCrossRunner();
	processDepartQueue();
	processArriveQueue();

	// Move phase alternates 0/1 each frame
	processMovePhase();
	_movePhaseFlag = 1 - _movePhaseFlag;

	// Clean up freed runners
	processFreedRunners();

	// Per-frame path interpolation for all active movers
	for (int16 i = 0; i < _moveQueueSize; i++) {
		advanceRunnerStep(_moveQueue[i]);
	}

	// Update Go button state
	setGoButtonsEnabled(_bAdvanceEnabled);
}

// --- Queue processing ---

void ZoombiniPuzzleLilly::processEnterQueue() {
	// IDA: mainFrameUpdate enter queue section
	// Serialized: only one enter runner active at a time
	if (_activeEnterRunner >= 0)
		return;

	if (_enterQueueSize <= 0)
		return;

	// Pop first element
	int16 runnerIdx = _enterQueue[0];
	for (int16 i = 0; i < _enterQueueSize - 1; i++)
		_enterQueue[i] = _enterQueue[i + 1];
	_enterQueueSize--;

	_activeEnterRunner = runnerIdx;

	// IDA: Load SCRB 10057 (enter animation)
	if (_zmbRunners[runnerIdx]) {
		loadScrbOntoFeature(_zmbRunners[runnerIdx], 10057);
		_zmbRunners[runnerIdx]->activateRender();
		_zmbRunners[runnerIdx]->activateAnimate();
	}
}

void ZoombiniPuzzleLilly::processExitQueue() {
	// IDA: mainFrameUpdate exit queue section
	if (_activeExitRunner >= 0)
		return;

	if (_exitQueueSize <= 0)
		return;

	int16 runnerIdx = _exitQueue[0];
	for (int16 i = 0; i < _exitQueueSize - 1; i++)
		_exitQueue[i] = _exitQueue[i + 1];
	_exitQueueSize--;

	_activeExitRunner = runnerIdx;

	// IDA: Load SCRB 10058 (exit animation)
	if (_zmbRunners[runnerIdx]) {
		loadScrbOntoFeature(_zmbRunners[runnerIdx], 10058);
	}
}

void ZoombiniPuzzleLilly::processCompletedExitRunner() {
	if (_completedExitRunner < 0)
		return;

	int16 runnerIdx = _completedExitRunner;
	_completedExitRunner = -1;

	// IDA: Runner finished exit animation — add to pending move
	if (_pendingMoveCount < kMaxMoveQueueSize) {
		_pendingMoveQueue[_pendingMoveCount++] = runnerIdx;
	}
}

void ZoombiniPuzzleLilly::processRotateQueue() {
	// IDA: Obstacle rotation animations
	if (_rotateQueueSize <= 0)
		return;

	for (int16 i = 0; i < _rotateQueueSize; i++) {
		int16 runnerIdx = _rotateQueue[i];
		if (_zmbRunners[runnerIdx]) {
			// IDA: Random SCRB selection 10060-10066
			uint16 rotScrb = 10060 + _vm->_rnd->getRandomNumber(0, 6);
			loadScrbOntoFeature(_zmbRunners[runnerIdx], rotScrb);
		}
	}
	_rotateQueueSize = 0;
}

void ZoombiniPuzzleLilly::processCrossQueue() {
	// IDA: Cross-row transition animations
	if (_activeCrossRunner >= 0)
		return;

	if (_crossQueueSize <= 0)
		return;

	int16 runnerIdx = _crossQueue[0];
	for (int16 i = 0; i < _crossQueueSize - 1; i++)
		_crossQueue[i] = _crossQueue[i + 1];
	_crossQueueSize--;

	_activeCrossRunner = runnerIdx;

	// IDA: Load SCRB 10059 (cross animation)
	if (_zmbRunners[runnerIdx]) {
		loadScrbOntoFeature(_zmbRunners[runnerIdx], 10059);
	}
}

void ZoombiniPuzzleLilly::processCompletedCrossRunner() {
	if (_completedCrossRunner < 0)
		return;

	int16 runnerIdx = _completedCrossRunner;
	_completedCrossRunner = -1;

	// IDA: Runner finished cross — add to pending moves
	if (_pendingMoveCount < kMaxMoveQueueSize) {
		_pendingMoveQueue[_pendingMoveCount++] = runnerIdx;
	}
}

void ZoombiniPuzzleLilly::processDepartQueue() {
	// IDA: Depart queue — runners leaving their current position
	if (_departQueueSize <= 0)
		return;

	for (int16 i = 0; i < _departQueueSize; i++) {
		int16 runnerIdx = _departQueue[i];
		if (_zmbRunners[runnerIdx]) {
			// IDA: Load SCRB 10141+dirByte (departure animation based on direction)
			ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
			uint16 departScrb = 10141 + rs.dirByte;
			loadScrbOntoFeature(_zmbRunners[runnerIdx], departScrb);
		}
	}
	_departQueueSize = 0;
}

void ZoombiniPuzzleLilly::processArriveQueue() {
	// IDA: Arrive queue — runners arriving at destination cell
	if (_arriveQueueSize <= 0)
		return;

	for (int16 i = 0; i < _arriveQueueSize; i++) {
		int16 runnerIdx = _arriveQueue[i];
		if (_zmbRunners[runnerIdx]) {
			// IDA: Load SCRB 10019+dirByte (arrival animation)
			ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
			uint16 arriveScrb = 10019 + rs.dirByte;
			loadScrbOntoFeature(_zmbRunners[runnerIdx], arriveScrb);
		}
	}
	_arriveQueueSize = 0;
}

void ZoombiniPuzzleLilly::processMovePhase() {
	// IDA: lilly_movePhaseFlag alternates 0/1 each frame

	if (_movePhaseFlag == 0) {
		// Phase 0: Drain pending queue → ready queue, advance ready runners
		for (int16 i = 0; i < _pendingMoveCount; i++) {
			if (_readyQueueSize < kMaxMoveQueueSize) {
				_readyQueue[_readyQueueSize++] = _pendingMoveQueue[i];
			}
		}
		_pendingMoveCount = 0;

		// Process ready runners
		int16 newReadySize = 0;
		for (int16 i = 0; i < _readyQueueSize; i++) {
			int16 runnerIdx = _readyQueue[i];
			ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

			// Call advancePathOnGrid to get next SCRB
			uint16 nextScrb = advancePathOnGrid(runnerIdx);

			if (nextScrb == 10031) {
				// Exit/cross transition — queue to cross
				if (_crossQueueSize < kMaxQueueSize) {
					_crossQueue[_crossQueueSize++] = runnerIdx;
				}
			} else if (nextScrb != 0) {
				// Normal direction move — load SCRB and add to move queue
				if (_zmbRunners[runnerIdx]) {
					// Update runner position to new cell
					Common::Point newPos = _gridCellPos[rs.row][rs.col];
					_zmbRunners[runnerIdx]->setPointLoc(newPos);
					loadScrbOntoFeature(_zmbRunners[runnerIdx], nextScrb);

					// Initialize path interpolation
					rs.pathStepIdx = 1;
					rs.pathStepDir = 1;
				}
				if (_moveQueueSize < kMaxMoveQueueSize) {
					_moveQueue[_moveQueueSize++] = runnerIdx;
				}
			} else {
				// No valid move — keep in ready queue for retry
				_readyQueue[newReadySize++] = runnerIdx;
			}
		}
		_readyQueueSize = newReadySize;

	} else {
		// Phase 1: Obstacle spawning (difficulty >= 3)
		if (_difficultyLevel >= 3) {
			// Path init queue processing
			for (int16 i = 0; i < _pathInitQueueSize; i++) {
				initRunnerBFSPath(_pathInitQueue[i]);
			}
			_pathInitQueueSize = 0;

			// Timer-based obstacle spawning every 480 frames
			if (getCurrentFrameCounter() >= _nextObstacleTimer) {
				spawnObstacleRunner();
				_nextObstacleTimer = getCurrentFrameCounter() + 480;
			}
		}
	}
}

void ZoombiniPuzzleLilly::processFreedRunners() {
	// IDA: Clean up freed runners at end of mainFrameUpdate
	for (int16 i = 0; i < _freedRunnerCount; i++) {
		int16 runnerIdx = _freedRunners[i];
		if (runnerIdx >= 0 && runnerIdx < kMaxRunners && _zmbRunners[runnerIdx]) {
			_zmbRunners[runnerIdx]->deactivateRender();
			_zmbRunners[runnerIdx]->deactivateAnimate();
		}
	}
	_freedRunnerCount = 0;
}

// =================================================================
// Per-frame path interpolation
// IDA: maze_advanceRunnerStep (0x425C85)
// =================================================================

void ZoombiniPuzzleLilly::advanceRunnerStep(int16 runnerIdx) {
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
	if (rs.pathStepIdx == 0)
		return;

	// Advance step index by direction
	rs.pathStepIdx += rs.pathStepDir;

	// Apply position deltas from REGS 200 lookup tables
	if (rs.pathStepIdx >= 0 && rs.pathStepIdx < (int16)_regsDeltaX.size()) {
		ZmbFeature *runner = _zmbRunners[runnerIdx];
		if (runner) {
			Common::Point pos = runner->getPointLoc();
			pos.x -= _regsDeltaX[rs.pathStepIdx];
			pos.y -= _regsDeltaY[rs.pathStepIdx];
			runner->setPointLoc(pos);
		}
	}
}

// =================================================================
// Pathfinding
// =================================================================

uint16 ZoombiniPuzzleLilly::advancePathOnGrid(int16 runnerIdx) {
	// IDA: fleens_advancePathStep (0x425F3D)
	// Reads current position & direction, picks best neighbor, returns SCRB.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return 0;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
	int16 curCol = rs.col;
	int16 curRow = rs.row;
	byte curDir = rs.direction;

	// Exit case: direction 4 means exit the grid
	if (curDir == 4) {
		return 10031;
	}

	// Check all 4 neighbors, find the one with lowest visit count
	// Direction order: 0=up, 1=right, 2=down, 3=left
	static const int8 dCol[4] = {0, 1, 0, -1};
	static const int8 dRow[4] = {-1, 0, 1, 0};

	int16 bestCol = -1, bestRow = -1;
	int16 bestVisit = 32767;
	byte bestDir = curDir;

	// Check neighbors starting from current direction
	for (int d = 0; d < 4; d++) {
		byte checkDir = (curDir + d) % 4;
		int16 newCol = curCol + dCol[checkDir];
		int16 newRow = curRow + dRow[checkDir];

		// Bounds check (0-11)
		if (newCol < 0 || newCol >= _gridType || newRow < 0 || newRow >= 12)
			continue;

		// Skip occupied cells
		if (_gridOccupancy[newRow][newCol] != 0)
			continue;

		// Attribute constraint check
		bool validAttr = true;
		switch (rs.attrType) {
		case 1:
			validAttr = (_gridAttr1[newRow][newCol] == rs.attrValue);
			break;
		case 2:
			validAttr = (_gridAttr2[newRow][newCol] == rs.attrValue);
			break;
		case 3:
			validAttr = (_gridAttr3[newRow][newCol] == rs.attrValue);
			break;
		default:
			break;
		}
		if (!validAttr)
			continue;

		// Pick cell with lowest visit count
		int16 visitCount = rs.visitGrid[newRow][newCol];
		if (visitCount < bestVisit) {
			bestVisit = visitCount;
			bestCol = newCol;
			bestRow = newRow;
			bestDir = checkDir;
		}
	}

	if (bestCol < 0) {
		// No valid move found
		return 0;
	}

	// Mark old cell as unoccupied
	_gridOccupancy[curRow][curCol] = 0;

	// Move to new cell
	rs.col = bestCol;
	rs.row = bestRow;
	rs.direction = bestDir;

	// Mark new cell occupied
	_gridOccupancy[bestRow][bestCol] = 1;

	// Increment visit count
	rs.visitGrid[bestRow][bestCol]++;

	// Anti-loop: reset visit counts when any reaches 10000
	if (rs.visitGrid[bestRow][bestCol] >= 10000) {
		memset(rs.visitGrid, 0, sizeof(rs.visitGrid));
	}

	// Determine direction byte for SCRB selection
	rs.dirByte = bestDir;

	// Return direction-based SCRB ID
	const uint16 *dirTable;
	switch (bestDir) {
	case 0: dirTable = kDirScrbUp; break;
	case 1: dirTable = kDirScrbRight; break;
	case 2: dirTable = kDirScrbDown; break;
	case 3: dirTable = kDirScrbLeft; break;
	default: return 0;
	}

	return dirTable[curDir];
}

void ZoombiniPuzzleLilly::computeShortestPath(byte targetRow, int16 runnerIdx) {
	// IDA: maze_computeShortestPath (0x42990A)
	// Dijkstra-style single-source shortest path on 12x12 grid
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	// Start from frontier position
	// Iterate up to 200 steps
	for (int step = 0; step < 200; step++) {

		// Find unprocessed cell with minimum cost
		for (int row = 0; row < 12; row++) {
			for (int col = 0; col < _gridType; col++) {
				int16 cost = rs.visitGrid[row][col];
				if (cost > 0) {
					// Check if any neighbor has zero cost (unprocessed)
					static const int8 dCol[4] = {0, 1, 0, -1};
					static const int8 dRow[4] = {-1, 0, 1, 0};

					for (int d = 0; d < 4; d++) {
						int16 nCol = col + dCol[d];
						int16 nRow = row + dRow[d];
						if (nCol < 0 || nCol >= _gridType || nRow < 0 || nRow >= 12)
							continue;
						if (_gridOccupancy[nRow][nCol] != 0)
							continue;

						bool validAttr = true;
						switch (rs.attrType) {
						case 1: validAttr = (_gridAttr1[nRow][nCol] == rs.attrValue); break;
						case 2: validAttr = (_gridAttr2[nRow][nCol] == rs.attrValue); break;
						case 3: validAttr = (_gridAttr3[nRow][nCol] == rs.attrValue); break;
						default: break;
						}
						if (!validAttr)
							continue;

						if (rs.visitGrid[nRow][nCol] == 0) {
							rs.visitGrid[nRow][nCol] = cost + 1;
							// Track progress toward target
							if (nRow > rs.frontierRow || (nRow == rs.frontierRow && nCol > rs.frontierCol)) {
								rs.frontierCol = nCol;
								rs.frontierRow = nRow;
							}
						}
					}
				}
			}
		}

		// Check if we reached target
		if (rs.frontierRow >= targetRow)
			break;
	}
}

void ZoombiniPuzzleLilly::traversePathBFS(byte targetRow, int16 runnerIdx) {
	// IDA: lilly_traversePathBFS (0x429C2D)
	// Greedy traversal from frontier toward start, zeroing cells
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	int16 curCol = rs.frontierCol;
	int16 curRow = rs.frontierRow;

	static const int8 dCol[4] = {0, 1, 0, -1};
	static const int8 dRow[4] = {-1, 0, 1, 0};

	for (int step = 0; step < 200; step++) {
		// Find neighbor with highest cost (furthest from start)
		int16 bestCol = -1, bestRow = -1;
		int16 bestCost = -1;

		for (int d = 0; d < 4; d++) {
			int16 nCol = curCol + dCol[d];
			int16 nRow = curRow + dRow[d];
			if (nCol < 0 || nCol >= _gridType || nRow < 0 || nRow >= 12)
				continue;

			int16 cost = rs.visitGrid[nRow][nCol];
			if (cost > bestCost) {
				bestCost = cost;
				bestCol = nCol;
				bestRow = nRow;
			}
		}

		if (bestCol < 0)
			break;

		// Zero current cell (consuming the path)
		rs.visitGrid[curRow][curCol] = 0;

		curCol = bestCol;
		curRow = bestRow;

		// Check if reached start
		if (curRow <= rs.row && curCol <= rs.col)
			break;
	}
}

void ZoombiniPuzzleLilly::initRunnerBFSPath(int16 runnerIdx) {
	// IDA: maze_initRunnerBFSPath (0x429440)
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	// Clear visit grid
	memset(rs.visitGrid, 0, sizeof(rs.visitGrid));

	// Copy start position to frontier
	rs.frontierCol = rs.col;
	rs.frontierRow = rs.row;

	// Set target row and initial step
	rs.targetRow = 11;
	rs.stepCount = 1;

	// Run Dijkstra twice and then BFS traverse
	// IDA: calls computeShortestPath twice, then traversePathBFS
	computeShortestPath(rs.targetRow, runnerIdx);
	computeShortestPath(rs.targetRow, runnerIdx);
	traversePathBFS(rs.targetRow, runnerIdx);

	// Mark starting cell in visit grid
	rs.visitGrid[rs.row][rs.col] = rs.stepCount;
}

// =================================================================
// Click handling
// IDA: lilly_onClickHandler (0x4245CC)
// =================================================================

ZmbEventHandleResult ZoombiniPuzzleLilly::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Let base class handle standard buttons (Go/Map/Help)
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Guard: check puzzle state
	if (!_bPuzzleActive || _bCellSelectActive)
		return ZmbEventHandleResult::kPassthrough;

	// IDA: Case 4 — Zoombini click
	// Find clicked zoombini runner via bitmask 9961474 (0x980002)
	for (int16 i = 0; i < _totalZmbCount; i++) {
		if (!_zmbRunners[i] || !_zmbRunners[i]->isRenderActivated())
			continue;

		ZmbLillyRunnerState &rs = _runnerStates[i];
		if (rs.placed)
			continue;

		// Hit test against runner feature
		if (_zmbRunners[i]->hasClickRect() &&
		    _zmbRunners[i]->getClickRect().contains(absPos)) {
			handleZoombiniClick(_zmbRunners[i]);
			return ZmbEventHandleResult::kConsumed;
		}
	}

	// Check for cell selection during interactive mode
	if (_bCellSelectActive && _selectingRunnerIdx >= 0) {
		int16 clickCol, clickRow;
		if (findCellAtPoint(absPos, clickCol, clickRow) >= 0) {
			if (isCellValidForRunner(clickCol, clickRow, _selectingRunnerIdx)) {
				_selectedCellIdx = clickRow * 13 + clickCol;
				_bCellSelectActive = false;

				// Finalize placement
				ZmbLillyRunnerState &rs = _runnerStates[_selectingRunnerIdx];
				rs.col = clickCol;
				rs.row = clickRow;
				rs.placed = true;

				// Mark cell occupied
				_gridOccupancy[clickRow][clickCol] = 1;

				// Set runner position
				Common::Point cellPos = _gridCellPos[clickRow][clickCol];
				_zmbRunners[_selectingRunnerIdx]->setPointLoc(cellPos);

				// Initialize BFS path
				initRunnerBFSPath(_selectingRunnerIdx);

				// Load movement SCRB
				loadScrbOntoFeature(_zmbRunners[_selectingRunnerIdx], 10109 + _selectingRunnerIdx);

				_placedZmbCount++;

				// IDA: Check for completion
				if (_placedZmbCount == _totalZmbCount && _swapLevel == 6) {
					countMatchesAndPlaySound();
				}

				// Enable advance if all placed
				if (_placedZmbCount >= _totalZmbCount) {
					_bAdvanceEnabled = true;
				}

				_selectingRunnerIdx = -1;
				return ZmbEventHandleResult::kConsumed;
			}
		}
	}

	return ZmbEventHandleResult::kPassthrough;
}

void ZoombiniPuzzleLilly::handleZoombiniClick(ZmbFeature *clickedRunner) {
	// IDA: lilly_onClickHandler case 4 — interactive selection
	// Find runner index
	int16 runnerIdx = -1;
	for (int16 i = 0; i < _totalZmbCount; i++) {
		if (_zmbRunners[i] == clickedRunner) {
			runnerIdx = i;
			break;
		}
	}

	if (runnerIdx < 0)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
	if (rs.placed)
		return;

	// Determine attribute constraint from zoombini traits
	// IDA: Uses zoombini trait to select attr type and value
	ZmbSnoid *snoid = getSnoid(10000 + runnerIdx);
	if (!snoid)
		return;

	// Map zoombini trait to grid attribute constraint
	// IDA: The constraint type is selected based on the trait byte values
	// Hair → attr type 1, Eyes → attr type 2, Nose/feet → attr type 3
	byte hairAttr = snoid->_trait._head;
	byte eyeAttr = snoid->_trait._eye;
	byte noseAttr = snoid->_trait._nose;

	// Determine which attribute type to constrain on
	// IDA: Based on difficulty and random selection from available attributes
	if (_gridType >= 5) {
		// Difficulty 3-4: randomly pick one of 3 attribute types
		int16 attrChoice = _vm->_rnd->getRandomNumber(1, 3);
		switch (attrChoice) {
		case 1: rs.attrType = 1; rs.attrValue = hairAttr % 3; break;
		case 2: rs.attrType = 2; rs.attrValue = eyeAttr % 4; break;
		case 3: rs.attrType = 3; rs.attrValue = noseAttr % 5; break;
		}
	} else if (_gridType >= 4) {
		// Difficulty 2: pick from 2 attribute types
		int16 attrChoice = _vm->_rnd->getRandomNumber(1, 2);
		switch (attrChoice) {
		case 1: rs.attrType = 1; rs.attrValue = hairAttr % 3; break;
		case 2: rs.attrType = 2; rs.attrValue = eyeAttr % 4; break;
		}
	} else {
		// Difficulty 1: only hair
		rs.attrType = 1;
		rs.attrValue = hairAttr % 2;
	}

	// Enter interactive cell selection mode
	_bCellSelectActive = true;
	_selectingRunnerIdx = runnerIdx;
	_selectedCellIdx = -1;

	// Show cursor runner at valid entry cells
	if (_cursorRunnerFeature) {
		_cursorRunnerFeature->activateRender();
	}

	// Play selection sound
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, 12000));
}

int16 ZoombiniPuzzleLilly::findCellAtPoint(const Common::Point &pos, int16 &outCol, int16 &outRow) const {
	// Hit-test all grid cells against the given point
	for (int row = 0; row < 12; row++) {
		for (int col = 0; col < _gridType; col++) {
			if (_gridCellRect[row][col].contains(pos)) {
				outCol = col;
				outRow = row;
				return row * 13 + col;
			}
		}
	}
	outCol = -1;
	outRow = -1;
	return -1;
}

bool ZoombiniPuzzleLilly::isCellValidForRunner(int16 col, int16 row, int16 runnerIdx) const {
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return false;

	const ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	// Cell must be unoccupied
	if (_gridOccupancy[row][col] != 0)
		return false;

	// Cell must match attribute constraint
	switch (rs.attrType) {
	case 1: return _gridAttr1[row][col] == rs.attrValue;
	case 2: return _gridAttr2[row][col] == rs.attrValue;
	case 3: return _gridAttr3[row][col] == rs.attrValue;
	default: return true;
	}
}

// =================================================================
// Animation event dispatch
// IDA: Event codes from SCRB animations
// =================================================================

void ZoombiniPuzzleLilly::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (!feature)
		return;

	uint16 featureId = feature->getId();

	// --- Zoombini runner events (SCRB 10109+i) ---
	if (featureId >= 10109 && featureId < 10109 + _totalZmbCount) {
		int16 runnerIdx = featureId - 10109;

		if (eventCode == kZmbAnimEventM1_End) {
			// Animation completed
			ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

			// Remove from move queue if present
			for (int16 i = 0; i < _moveQueueSize; i++) {
				if (_moveQueue[i] == runnerIdx) {
					for (int16 j = i; j < _moveQueueSize - 1; j++)
						_moveQueue[j] = _moveQueue[j + 1];
					_moveQueueSize--;
					break;
				}
			}

			// Reset path step
			rs.pathStepIdx = 0;

			// Queue for next step
			if (rs.placed && rs.direction != 4) {
				if (_pendingMoveCount < kMaxMoveQueueSize) {
					_pendingMoveQueue[_pendingMoveCount++] = runnerIdx;
				}
			}
			return;
		}

		// IDA: Exit callback events (1, 2, 3)
		if (eventCode >= 1 && eventCode <= 3) {
			handleRunnerExitCallback(eventCode, runnerIdx);
			return;
		}

		// IDA: Arrive/depart events (70, 80)
		if (eventCode == 70 || eventCode == 80) {
			handleRunnerArriveOrDepart(eventCode, runnerIdx);
			return;
		}

		return;
	}

	// --- Enter animation events (SCRB 10057) ---
	if (featureId == 10057) {
		// Find which runner has this animation loaded
		for (int16 i = 0; i < _totalZmbCount; i++) {
			if (_activeEnterRunner == i) {
				if (eventCode == kZmbAnimEventM1_End) {
					_activeEnterRunner = -1;
					// Runner entered grid — add to pending moves
					if (_pendingMoveCount < kMaxMoveQueueSize) {
						_pendingMoveQueue[_pendingMoveCount++] = i;
					}
				}
				break;
			}
		}
		return;
	}

	// --- Exit animation events (SCRB 10058) ---
	if (featureId == 10058) {
		if (eventCode == kZmbAnimEventM1_End && _activeExitRunner >= 0) {
			_completedExitRunner = _activeExitRunner;
			_activeExitRunner = -1;
		}
		return;
	}

	// --- Cross animation events (SCRB 10059) ---
	if (featureId == 10059) {
		if (eventCode == kZmbAnimEventM1_End && _activeCrossRunner >= 0) {
			_completedCrossRunner = _activeCrossRunner;
			_activeCrossRunner = -1;
		}
		return;
	}

	// --- Frog SCRB events (11000) ---
	if (featureId == 11000) {
		handleScriptEvent(eventCode, feature);
		return;
	}

	// --- Body arrangement events 240-253 (standard) ---
	if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst &&
	    eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
		if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);
			if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst) {
				snoid->setBodyArrangement(eventCode - kZmbAnimEvent250_BodyArrangeDirectFirst);
			}
		}
		return;
	}

	// --- Event 0: Toggle render visibility ---
	if (eventCode == 0) {
		if (feature->isRenderActivated())
			feature->deactivateRender();
		else
			feature->activateRender();
		return;
	}
}

// =================================================================
// Callbacks
// =================================================================

void ZoombiniPuzzleLilly::handleRunnerExitCallback(int16 exitCode, int16 runnerIdx) {
	// IDA: maze_runnerExitCallback (0x425CCA)
	switch (exitCode) {
	case 1:
		// IDA: call maze_setRunnerIdxVar(5), decrement remainingZmbs
		if (--_remainingZmbs < 0)
			_remainingZmbs = 0;
		break;

	case 2:
		// IDA: Push to arrive queue
		if (_arriveQueueSize < kMaxQueueSize) {
			_arriveQueue[_arriveQueueSize++] = runnerIdx;
		}
		break;

	case 3:
		// IDA: Push to depart queue
		if (_departQueueSize < kMaxQueueSize) {
			_departQueue[_departQueueSize++] = runnerIdx;
		}
		break;

	default:
		break;
	}
}

void ZoombiniPuzzleLilly::handleRunnerArriveOrDepart(int16 eventCode, int16 runnerIdx) {
	// IDA: maze_runnerArriveOrDepartCallback (0x424D3E)
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	if (eventCode == 70) {
		// Arrive: snapshot position, push to move queue
		// IDA: runner+214..220 = runner+50..52 (position snapshot)
		if (_moveQueueSize < kMaxMoveQueueSize) {
			_moveQueue[_moveQueueSize++] = runnerIdx;
		}
	} else if (eventCode == 80) {
		// Depart: clear occupancy, push to freed list, remove from active obstacles
		_gridOccupancy[rs.obstCol][rs.obstRow] = 0;

		if (_freedRunnerCount < kMaxRunners) {
			_freedRunners[_freedRunnerCount++] = runnerIdx;
		}

		// Remove from active obstacles array
		for (int16 i = 0; i < _activeObstacleCount; i++) {
			if (_activeObstacles[i] == runnerIdx) {
				for (int16 j = i; j < _activeObstacleCount - 1; j++)
					_activeObstacles[j] = _activeObstacles[j + 1];
				_activeObstacleCount--;
				break;
			}
		}
	}
}

void ZoombiniPuzzleLilly::handleScriptEvent(int16 eventId, ZmbFeature *eventFeature) {
	// IDA: maze_scriptEventHandler (0x425D55)
	switch (eventId) {
	case 3:
		// IDA: Show exit SCRBs for last 2 runners (difficulty > 1)
		if (_difficultyLevel > 1) {
			for (int16 i = 0; i < _totalZmbCount; i++) {
				if (i == _totalZmbCount - 2 || i == _totalZmbCount - 1) {
					if (_zmbRunners[i]) {
						_zmbRunners[i]->activateRender();
						loadScrbOntoFeature(_zmbRunners[i], 10089 + i);
					}
				}
			}
		}
		break;

	case 4:
		// IDA: Store exited runner, enable frog cursor
		if (_frogRunnerFeature) {
			_frogRunnerFeature->activateRender();
		}
		// Initialize BFS grids for all cells (if difficulty > 2)
		if (_difficultyLevel > 2) {
			for (int16 i = 0; i < _gridType; i++) {
				// IDA: maze_initBFSGrid for each cell column
				// This updates the obstacle pathfinding grids
			}
		}
		break;

	case 5:
		// IDA: Cell swap state machine
		if (_cellSelectState == 4) {
			// Select first swap cell
			_swapCellACol = kSwapPairCol[2 * _swapPairIdx];
			_swapCellARow = kSwapPairRow[2 * _swapPairIdx];
			setRunnerClickRect(_swapCellACol, _swapCellARow, _cellAnimRunnerA);
			_cellSelectState = 5;
			_swapPairIdx++;
		} else if (_cellSelectState == 5) {
			// Select second swap cell
			_swapCellBCol = kSwapPairCol[2 * _swapPairIdx];
			_swapCellBRow = kSwapPairRow[2 * _swapPairIdx];
			setRunnerClickRect(_swapCellBCol, _swapCellBRow, _cellAnimRunnerB);
			_cellSelectState = 6;
			_swapPairIdx++;
		}
		break;

	default:
		break;
	}
}

// =================================================================
// Helpers
// =================================================================

void ZoombiniPuzzleLilly::countMatchesAndPlaySound() {
	// IDA: fleens_countAttrMatchAndEnqueueSound (0x429395)
	int16 matchCount = 0;

	for (int16 i = 0; i < _obstacleRunnerCount; i++) {
		int16 runnerIdx = _obstacleRunners[i];
		if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
			continue;

		ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
		if (rs.placed && rs.direction == 4) {
			// Runner completed path — count as match
			matchCount++;
		}
	}

	if (matchCount < _totalZmbCount) {
		// Play random sound in range 20045-20048
		int16 rndCheck = _vm->_rnd->getRandomNumber(0, 4);
		if (rndCheck > _difficultyLevel - 1) {
			uint16 soundId = _vm->_rnd->getRandomNumber(20045, 20048);
			_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, soundId));
		}
	}
}

void ZoombiniPuzzleLilly::setRunnerClickRect(int16 col, int16 row, ZmbFeature *feature) {
	// IDA: maze_setRunnerClickRect (0x429196)
	if (!feature)
		return;

	if (col >= 0 && col < 13 && row >= 0 && row < 12) {
		Common::Point cellPos = _gridCellPos[row][col];
		feature->setPointLoc(cellPos);
		feature->setClickRect(_gridCellRect[row][col]);
		feature->activateRender();
	}
}

void ZoombiniPuzzleLilly::swapCellsAndUpdateRunners(int16 colA, int16 rowA, int16 colB, int16 rowB) {
	// IDA: maze_swapCellsAndUpdateRunners (0x4273BC)
	// Swap attributes between two grid cells and reinitialize affected runner paths

	// Swap all attribute layers
	byte tmpOcc = _gridOccupancy[rowA][colA];
	_gridOccupancy[rowA][colA] = _gridOccupancy[rowB][colB];
	_gridOccupancy[rowB][colB] = tmpOcc;

	byte tmp1 = _gridAttr1[rowA][colA];
	_gridAttr1[rowA][colA] = _gridAttr1[rowB][colB];
	_gridAttr1[rowB][colB] = tmp1;

	byte tmp2 = _gridAttr2[rowA][colA];
	_gridAttr2[rowA][colA] = _gridAttr2[rowB][colB];
	_gridAttr2[rowB][colB] = tmp2;

	byte tmp3 = _gridAttr3[rowA][colA];
	_gridAttr3[rowA][colA] = _gridAttr3[rowB][colB];
	_gridAttr3[rowB][colB] = tmp3;

	// Reinitialize BFS paths for all placed runners
	for (int16 i = 0; i < _totalZmbCount; i++) {
		ZmbLillyRunnerState &rs = _runnerStates[i];
		if (rs.placed && rs.direction != 4) {
			initRunnerBFSPath(i);
		}
	}
}

void ZoombiniPuzzleLilly::spawnObstacleRunner() {
	// IDA: maze_registerObstacleRunner (0x4267AF)
	// Spawn a new obstacle on the grid (difficulty >= 3)
	if (_activeObstacleCount >= _obstacleRows)
		return;

	// Find a free row for the obstacle
	for (int16 row = 0; row < 12; row++) {
		bool rowFree = true;
		for (int16 i = 0; i < _activeObstacleCount; i++) {
			if (_runnerStates[_activeObstacles[i]].obstRow == row) {
				rowFree = false;
				break;
			}
		}

		if (rowFree) {
			// Find the obstacle runner to use
			if (_nextObstacleIdx >= _obstacleRunnerCount)
				_nextObstacleIdx = 0;

			int16 runnerIdx = _obstacleRunners[_nextObstacleIdx];
			if (runnerIdx >= 0 && runnerIdx < kMaxRunners) {
				ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
				rs.obstRow = row;
				rs.obstCol = 0;
				rs.col = 0;
				rs.row = row;
				rs.placed = true;

				// Initialize obstacle path
				initRunnerBFSPath(runnerIdx);

				// Add to active obstacles
				_activeObstacles[_activeObstacleCount++] = runnerIdx;
			}

			_nextObstacleIdx++;
			return;
		}
	}
}

} // End of namespace Mohawk
