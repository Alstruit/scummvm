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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_LILLY_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_LILLY_H

#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Per-runner pathfinding and movement state for the Lilly puzzle.
 * Maps to the original engine's per-runner struct fields at byte offsets
 * +195..+295.
 */
struct LillyRunnerState {
	byte col = 0;               // Current grid column. IDA: runner+195
	byte row = 0;               // Current grid row. IDA: runner+196
	byte frontierCol = 0;       // BFS frontier column. IDA: runner+199
	byte frontierRow = 0;       // BFS frontier row. IDA: runner+200
	byte direction = 0;         // Movement direction 0-3 cardinal, 4=exit. IDA: runner+213
	byte targetRow = 11;        // Target row for Dijkstra. IDA: runner+214
	int16 stepCount = 1;        // Step cost counter. IDA: runner+215
	byte attrType = 0;          // Attribute constraint type 1/2/3. IDA: runner+222
	byte attrValue = 0;         // Attribute constraint value. IDA: runner+223
	int16 visitGrid[12][13];    // Visit/cost matrix (26-byte stride). IDA: runner+242
	byte dirByte = 0;           // Direction byte for Z-linking. IDA: runner+261
	byte obstRow = 0;           // Obstacle source row. IDA: runner+243
	byte obstCol = 0;           // Obstacle source col. IDA: runner+244
	bool placed = false;        // Has been placed on grid
	bool matched = false;       // Partner matched flag. IDA: runner+295

	// Path interpolation state. IDA: runner+24/25/26/34
	int16 pathStepIdx = 0;      // Path animation step index. IDA: runner[24]
	int16 pathStepDir = 0;      // Path step direction/increment. IDA: runner[34]

	void clear() {
		col = row = frontierCol = frontierRow = 0;
		direction = 0;
		targetRow = 11;
		stepCount = 1;
		attrType = attrValue = 0;
		memset(visitGrid, 0, sizeof(visitGrid));
		dirByte = obstRow = obstCol = 0;
		placed = matched = false;
		pathStepIdx = pathStepDir = 0;
	}
};

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
class ZoombiniInteractiveLilly : public ZoombiniPuzzle {
public:
	ZoombiniInteractiveLilly(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveLilly() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;
	void onEveryFrame() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;
	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;

private:
	// --- Initialization ---
	void loadZoombinisFromPack();
	void setDifficultyParams();
	void initGridWithAttributes();
	void createZoombiniRunners();
	void loadRegsCoordinateTables();

	// --- Queue processing (called from onEveryFrame) ---
	void processEnterQueue();
	void processExitQueue();
	void processCompletedExitRunner();
	void processRotateQueue();
	void processCrossQueue();
	void processCompletedCrossRunner();
	void processDepartQueue();
	void processArriveQueue();
	void processMovePhase();
	void processFreedRunners();

	// --- Pathfinding ---

	/**
	 * Advance a runner one step on the grid.
	 * IDA: fleens_advancePathStep (0x425F3D)
	 * @return SCRB ID for the direction animation, or 10031 for exit.
	 */
	uint16 advancePathOnGrid(int16 runnerIdx);

	/**
	 * Dijkstra-style shortest path fill on 12x12 grid.
	 * IDA: maze_computeShortestPath (0x42990A)
	 */
	void computeShortestPath(byte targetRow, int16 runnerIdx);

	/**
	 * Greedy BFS traversal of the cost matrix.
	 * IDA: lilly_traversePathBFS (0x429C2D)
	 */
	void traversePathBFS(byte targetRow, int16 runnerIdx);

	/**
	 * Initialize full BFS path state for a runner.
	 * IDA: maze_initRunnerBFSPath (0x429440)
	 */
	void initRunnerBFSPath(int16 runnerIdx);

	// --- Per-frame runner movement ---

	/**
	 * One-step path position interpolation.
	 * IDA: maze_advanceRunnerStep (0x425C85)
	 */
	void advanceRunnerStep(int16 runnerIdx);

	// --- Click handling ---

	/**
	 * Handle click on a Zoombini runner — interactive cell selection.
	 * Converted from the original blocking modal loop (0x4286A5) to
	 * a non-blocking state machine polled in onLButtonDown/onEveryFrame.
	 */
	void handleZoombiniClick(ZmbFeature *clickedRunner);

	/**
	 * Find the grid cell containing the given point.
	 * @return Cell index (col * 13 + row style) or -1 if none.
	 */
	int16 findCellAtPoint(const Common::Point &pos, int16 &outCol, int16 &outRow) const;

	/**
	 * Validate that a cell is accessible for a runner's attribute constraint.
	 */
	bool isCellValidForRunner(int16 col, int16 row, int16 runnerIdx) const;

	// --- Callbacks ---

	/**
	 * Runner exit callback. IDA: maze_runnerExitCallback (0x425CCA)
	 */
	void handleRunnerExitCallback(int16 exitCode, int16 runnerIdx);

	/**
	 * Runner arrive/depart callback. IDA: maze_runnerArriveOrDepartCallback (0x424D3E)
	 */
	void handleRunnerArriveOrDepart(int16 eventCode, int16 runnerIdx);

	/**
	 * Script event handler for frog-driven events.
	 * IDA: maze_scriptEventHandler (0x425D55)
	 */
	void handleScriptEvent(int16 eventId, ZmbFeature *eventFeature);

	// --- Helpers ---

	/**
	 * Count matched runners and play appropriate sound.
	 * IDA: fleens_countAttrMatchAndEnqueueSound (0x429395)
	 */
	void countMatchesAndPlaySound();

	/**
	 * Set a runner's click rect to a grid cell position.
	 * IDA: maze_setRunnerClickRect (0x429196)
	 */
	void setRunnerClickRect(int16 col, int16 row, ZmbFeature *feature);

	/**
	 * Swap two grid cells and update affected runners' paths.
	 * IDA: maze_swapCellsAndUpdateRunners (0x4273BC)
	 */
	void swapCellsAndUpdateRunners(int16 colA, int16 rowA, int16 colB, int16 rowB);

	/**
	 * Spawn a new obstacle runner on the grid (difficulty >= 3).
	 * IDA: maze_registerObstacleRunner (0x4267AF)
	 */
	void spawnObstacleRunner();

	// --- Direction SCRB tables ---
	// IDA: word_4A1738/40/48/50 — indexed by previous direction
	static const uint16 kDirScrbUp[4];
	static const uint16 kDirScrbRight[4];
	static const uint16 kDirScrbDown[4];
	static const uint16 kDirScrbLeft[4];

	// IDA: word_4A171E — Y offset per column for cell positions
	static const int16 kColYOffset[13];

	// IDA: word_4A16EC/4A1700 — preset swap pair coordinates
	static const int16 kSwapPairCol[20];
	static const int16 kSwapPairRow[20];

	// IDA: word_4A1832 — zoombini count to required grid row count
	static const int16 kZmbToRowCount[21];

	// =================================================================
	// Member variables
	// =================================================================

	// --- Core puzzle state ---
	int16 _difficultyLevel = 0;
	bool _bPuzzleActive = false;
	bool _bAdvanceEnabled = false;
	bool _bRenderEnabled = false;

	// --- Difficulty parameters ---
	int16 _mudBallCount = 0;
	int16 _obstacleRows = 0;

	// --- Zoombini counts ---
	int16 _totalZmbCount = 0;
	int16 _placedZmbCount = 0;
	int16 _remainingZmbs = 0;

	// --- Grid state ---
	int16 _gridType = 0;

	/** 12x13 occupancy grid. 0=empty, nonzero=occupied. IDA: byte_4AC684 */
	byte _gridOccupancy[12][13];
	/** 12x13 attribute type 1 grid (hair). IDA: byte_4AC685 */
	byte _gridAttr1[12][13];
	/** 12x13 attribute type 2 grid (eyes). IDA: byte_4AC686 */
	byte _gridAttr2[12][13];
	/** 12x13 attribute type 3 grid (nose/feet). IDA: byte_4AC687 */
	byte _gridAttr3[12][13];

	/** Cell bounding rects for hit testing. IDA: word_4AC67C/67E */
	Common::Rect _gridCellRect[12][13];

	/** Cell center positions (pixel coords). Computed from REGS 100 + column offsets. */
	Common::Point _gridCellPos[12][13];

	// --- REGS coordinate tables (loaded from resources) ---
	Common::Array<int16> _regsXTable;    // REGS 100 X coords
	Common::Array<int16> _regsYTable;    // REGS 100 Y coords
	Common::Array<int16> _regsDeltaX;    // REGS 200 X deltas (path interpolation)
	Common::Array<int16> _regsDeltaY;    // REGS 200 Y deltas (path interpolation)

	// --- Per-runner state ---
	static const int kMaxRunners = 21;
	LillyRunnerState _runnerStates[kMaxRunners];
	ZmbFeature *_zmbRunners[kMaxRunners] = {};

	// --- Obstacle runners ---
	int16 _obstacleRunners[kMaxRunners];
	int16 _obstacleRunnerCount = 0;
	int16 _activeObstacles[kMaxRunners];
	int16 _activeObstacleCount = 0;
	int16 _nextObstacleIdx = 0;
	uint32 _nextObstacleTimer = 0;

	// --- Freed runners pending cleanup ---
	int16 _freedRunners[kMaxRunners];
	int16 _freedRunnerCount = 0;

	// --- Animation queues (IDA: word_4AC428 etc.) ---
	static const int kMaxQueueSize = 21;
	static const int kMaxMoveQueueSize = 100;

	int16 _enterQueue[kMaxQueueSize];
	int16 _enterQueueSize = 0;
	int16 _exitQueue[kMaxQueueSize];
	int16 _exitQueueSize = 0;
	int16 _crossQueue[kMaxQueueSize];
	int16 _crossQueueSize = 0;
	int16 _rotateQueue[kMaxQueueSize];
	int16 _rotateQueueSize = 0;
	int16 _arriveQueue[kMaxQueueSize];
	int16 _arriveQueueSize = 0;
	int16 _departQueue[kMaxQueueSize];
	int16 _departQueueSize = 0;
	int16 _readyQueue[kMaxMoveQueueSize];
	int16 _readyQueueSize = 0;
	int16 _moveQueue[kMaxMoveQueueSize];
	int16 _moveQueueSize = 0;
	int16 _pendingMoveQueue[kMaxMoveQueueSize];
	int16 _pendingMoveCount = 0;
	int16 _pathInitQueue[kMaxQueueSize];
	int16 _pathInitQueueSize = 0;

	// --- Active runner tracking (serialized — only one at a time) ---
	int16 _activeEnterRunner = -1;
	int16 _activeExitRunner = -1;
	int16 _activeCrossRunner = -1;
	int16 _completedExitRunner = -1;
	int16 _completedCrossRunner = -1;
	int16 _exitedRunnerIdx = -1;

	// --- Move phase state ---
	int16 _movePhaseFlag = 0;

	// --- Cell selection state (interactive loop) ---
	bool _bCellSelectActive = false;
	int16 _cellSelectState = 0;
	int16 _selectedCellIdx = -1;
	int16 _highlightCol = -1;
	int16 _highlightRow = -1;
	int16 _selectingRunnerIdx = -1;

	// --- Cell swap state ---
	int16 _swapLevel = 0;
	int16 _swapThreshold = 0;
	int16 _swapCounter = 0;
	int16 _swapPairIdx = 0;
	int16 _swapCellACol = 0, _swapCellARow = 0;
	int16 _swapCellBCol = 0, _swapCellBRow = 0;

	// --- Grid rendering features ---
	ZmbFeature *_gridRendererFeature = nullptr;
	ZmbFeature *_cursorRunnerFeature = nullptr;
	ZmbFeature *_cellAnimRunnerA = nullptr;
	ZmbFeature *_cellAnimRunnerB = nullptr;
	ZmbFeature *_overlayFeatures[5] = {};
	ZmbFeature *_frogScrbFeature = nullptr;
	ZmbFeature *_frogRunnerFeature = nullptr;
};

} // End of namespace Mohawk

#endif
