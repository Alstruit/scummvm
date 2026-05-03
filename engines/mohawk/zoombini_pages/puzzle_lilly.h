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
 * Callback mode for per-runner animation event dispatch.
 * Each queue processing function sets the mode on the runner,
 * and onFeatureAnimEvent dispatches based on this mode.
 */
enum LillyCallbackMode {
	kCBLillyNone = 0,
	kCBLillyEnter,        // Enter: event 44 → rotateQueue
	kCBLillyRotate,       // Rotate: event 60 → exitQueue
	kCBLillyExit,         // Exit: event 49 → completedExitRunner
	kCBLillyCross,        // Cross: event 54 → completedCrossRunner
	kCBLillyDepart,       // Depart: maze_runnerExitCallback. event 1→counter, 2→arriveQueue, 3→departQueue
	kCBLillyArrive,       // Arrive: maze_handlePathBuildEvent. event 20 → readyQueue, event 26 → child SCRB
	kCBLillyReadyMove,    // Ready queue normal: steps 10,15 → readyQueue
	kCBLillyReadyExit,    // Ready queue exit: event 30 → enter/crossQueue
	kCBLillyMoveObstacle, // Move queue: event 70 → moveQueue, 80 → freedRunners
	kCBLillyMoveStep,     // Move queue normal step: phase 7 → moveQueue
};

/**
 * Per-runner pathfinding and movement state for the Lilly puzzle.
 * Maps to the original engine's per-runner struct fields at byte offsets
 * +192..+295. All field offsets are relative to the CFeatureRunner base.
 */
struct ZmbLillyRunnerState {
	// --- Grid position ---
	int16 directionMode = 0;    // 0=vertical, 1=horizontal movement. IDA: runner+192
	byte col = 0;               // Current grid column. IDA: runner+195
	byte row = 0;               // Current grid row. IDA: runner+196
	byte frontierCol = 0;       // BFS frontier column. IDA: runner+199
	byte frontierRow = 0;       // BFS frontier row. IDA: runner+200
	byte direction = 0;         // Movement direction preference 0-3. IDA: runner+213
	byte targetRow = 11;        // BFS max progress / target row. IDA: core188+0xD6 (runner+262)
	int16 stepCount = 1;        // Step cost counter. IDA: core188+0xD7 (runner+263)

	// --- Attribute constraint ---
	byte attrType = 0;          // Attribute constraint type 1/2/3. IDA: runner+222 (per-page)
	byte attrValue = 0;         // Attribute constraint value. IDA: runner+223 (per-page)
	byte obsCombinedAttr = 0;   // Obstacle combined attr = attrValue + BFS offset. IDA: runner+224 (per-page)

	// --- Path/visit grids ---
	// IDA fleens_advancePathStep_425F3D writes per-direction visit counts into
	// 4 distinct 12x13 grids at runner offsets +216 (LEFT), +240 (RIGHT),
	// +244 (DOWN), +268 (UP). The main +242 BFS grid records overall cost.
	// Indexed as `26 * row + 2 * col` (= [row][col] in word units).
	int16 visitGrid[12][13];        // Main BFS cost grid. IDA: runner+242
	int16 visitGridLeft[12][13];    // Per-direction LEFT scratch. IDA: runner+216
	int16 visitGridRight[12][13];   // Per-direction RIGHT scratch. IDA: runner+240
	int16 visitGridDown[12][13];    // Per-direction DOWN scratch. IDA: runner+244
	int16 visitGridUp[12][13];      // Per-direction UP scratch. IDA: runner+268

	// --- Animation/movement state ---
	// NOTE: runner+243=row, runner+244=col. Grid indexing: gridBase[169*col + 13*row].
	byte obstRow = 0;           // Current grid row position. IDA: runner+243
	byte obstCol = 0;           // Current grid column position. IDA: runner+244
	byte prevRow = 0;           // Previous grid row. IDA: runner+245
	byte prevCol = 0;           // Previous grid column. IDA: runner+246

	// --- Movement timer and mode ---
	uint32 moveTimer = 0;       // Frame counter threshold for movement. IDA: runner+36 (DWORD)
	int16 advanceMode = 0;      // 0=fresh obstacle(advancePathStepAlt), nonzero=reuse(advanceForwardStep). IDA: runner+98 (WORD)
	int16 bfsReinitFlag = 0;    // When set, obstacle re-enters pathInitQueue next frame. IDA: runner+100 (WORD)
	int16 moveStepPhase = 0;    // Current movement step phase (0-7). IDA: runner+200 (WORD)
	int16 destX = 0;            // Destination pixel X. IDA: runner+253
	int16 destY = 0;            // Destination pixel Y. IDA: runner+255
	int16 stepDeltaX = 0;       // Per-phase step delta X. IDA: runner+257
	int16 stepDeltaY = 0;       // Per-phase step delta Y. IDA: runner+259
	int16 pathStepIdx = 0;      // Current step index for path interpolation
	int16 pathStepDir = 1;      // Step direction (+1 forward, -1 backward)
	byte dirByte = 0;           // Direction byte for SCRB selection. IDA: runner+261
	int16 scrbOrDirKey = 0;     // SCRB id or direction key. IDA: runner+265
	byte crossedCount = 0;      // Times crossed (0=first, 2=done). IDA: runner+267

	// --- Runner linking ---
	int16 entryPointIdx = 0;    // Entry position index for exit path. IDA: runner+238
	byte childRunnerIdx = 0;    // Linked child runner index. IDA: runner+273

	// --- Flags ---
	LillyCallbackMode callbackMode = kCBLillyNone;
	bool placed = false;        // Has been placed on grid. IDA: core188+0xC2 (runner+242)
	bool matched = false;       // Partner matched flag. IDA: runner+295
	bool isObstacle = false;    // True if this is an obstacle runner

	void clear() {
		directionMode = 0;
		col = row = frontierCol = frontierRow = 0;
		direction = 0;
		targetRow = 11;
		stepCount = 1;
		attrType = attrValue = obsCombinedAttr = 0;
		memset(visitGrid, 0, sizeof(visitGrid));
		obstRow = obstCol = prevRow = prevCol = 0;
		moveTimer = 0;
		advanceMode = bfsReinitFlag = moveStepPhase = 0;
		destX = destY = stepDeltaX = stepDeltaY = 0;
		pathStepIdx = 0;
		pathStepDir = 1;
		dirByte = crossedCount = 0;
		scrbOrDirKey = 0;
		entryPointIdx = 0;
		childRunnerIdx = 0;
		callbackMode = kCBLillyNone;
		placed = matched = isObstacle = false;
	}
};

/**
 * Lily Pads puzzle page (ZoombiniPageType::kLilly).
 * Route 2, Puzzle 2
 *
 * Zoombinis must cross a pond by hopping on lily pads, seating on the big toads.
 * The grid of pads has an adjacency-based movement system; the player selects start pads.
 * The player drops a big toad onto the first column of pads, which then hops to the next valid pad.
 * At higher difficulty a crab obstacle appears.
 *
 * IDA entry: lilly_puzzleInit (0x422de4)
 * NOTE: Non-standard layout — does NOT use zmb_layoutStaticAndWalkInGroups.
 */
class ZoombiniPuzzleLilly : public ZoombiniPuzzle {
public:
	ZoombiniPuzzleLilly(MohawkEngine_Zoombini *vm);
	~ZoombiniPuzzleLilly() override;

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
	void createInitialObstacleRunners();
	void loadRegsCoordinateTables();
	void generateChallengePatterns();

	// --- Grid transform helpers ---
	void loadGridPatternRegs(int gridIdx, uint16 resId);
	void rotateGrid(int rotType, int16 grid[12][12]);
	void flipGrid(int flipType, int16 grid[12][12]);

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
	 * Zoombini move finalize step callback. IDA: maze_zmbMoveFinalizeStep (0x424A5B)
	 * Handles events 10-15 from direction SCRBs during readyQueue processing.
	 */
	void handleMoveFinalizeStep(int16 stepIdx, int16 runnerIdx);

	/**
	 * Runner arrival at grid node callback. IDA: maze_runnerArriveAtNode (0x425ADB)
	 * Handles event 30: dispatches to enterQueue or crossQueue.
	 */
	void handleArriveAtNode(int16 runnerIdx);

	/**
	 * Script event handler for frog-driven events.
	 * IDA: maze_scriptEventHandler (0x425D55)
	 */
	void handleScriptEvent(int16 eventId, ZmbFeature *eventFeature);

	// --- Custom render callbacks ---

	/**
	 * Render 12x12 grid cell sprites (lily pad lattice).
	 * IDA: maze_renderAllGridSprites (0x426BFB)
	 * Two layers per cell: attr2-based shape and combinedAttr-based shape.
	 */
	ZmbRenderResult renderGridSprites(ZmbFeature *feature);

	/**
	 * Render cursor highlight indicator on currently selected grid cell.
	 * IDA: maze_renderCursorIndicator (0x426DF9)
	 * 4-frame blink animation overlay on the highlighted cell.
	 */
	ZmbRenderResult renderCursorIndicator(ZmbFeature *feature);

	/**
	 * Pre-render shape callback for obstacle (toad) runners.
	 * IDA: maze_updateMultiLegPath (0x429E72)
	 * Adds the lane index (0-11) to the pattern overlay shape_id,
	 * selecting one of 12 distinct toad back patterns from tBMP 10000.
	 */
	void obstaclePreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);

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
	 * Process cell swap animation tick (blink + swap execution).
	 * Called from onEveryFrame when _cellSelectState == 6.
	 * IDA: maze_cellSwapAnimTick (0x4272B4) + maze_tickCellAnimFrame (0x42720F)
	 */
	void processCellSwapAnimation();

	/**
	 * Reset a cell anim runner position and deactivate rendering.
	 * IDA: maze_initCellRunnerPos (0x429222)
	 */
	void initCellRunnerPosition(int16 col, int16 row, ZmbFeature *feature);

	/**
	 * Spawn a new obstacle runner on the grid (difficulty >= 3).
	 * IDA: maze_registerObstacleRunner (0x4267AF)
	 */
	void spawnObstacleRunner();

	/**
	 * Remove a completed zoombini BFS runner from the live tracking array.
	 * IDA: word_4AE3EC membership compaction from maze_runnerSnapPosAndClearCell.
	 */
	void removeActiveBfsRunner(int16 runnerIdx);

	/**
	 * Advance obstacle runner forward one step (reuse mode).
	 * IDA: maze_zmbAdvanceForwardStep (0x4297D3)
	 * @return SCRB ID for direction animation, 10069 for exit, or 0 for no move.
	 */
	uint16 advanceObstacleForwardStep(int16 runnerIdx);

	/**
	 * Advance obstacle runner via BFS alternative path (fresh mode).
	 * IDA: fleens_advancePathStepAlt (0x42A1E6)
	 * @return SCRB ID for direction animation, 10069 for exit, or 0 for no move.
	 */
	uint16 advanceObstaclePathStepAlt(int16 runnerIdx);

	// --- BFS obstacle pathfinding ---

	/**
	 * Initialize BFS arrays for one attribute value layer.
	 * Seeds from grid cells matching the given attr value, then expands via BFS.
	 * IDA: maze_initBFSGrid (0x4294EB)
	 * @param attrValue The attribute value index (BFS layer).
	 * @param attrType  The attribute type (1=hair, 2=eyes, 3=nose) — used as grid offset.
	 */
	void initBFSGrid(int16 attrValue, int16 attrType);

	/**
	 * Expand one BFS cell in 4 directions (up/right/down/left).
	 * IDA: maze_bfsExpandCell (0x42971D)
	 * @param col       Column of cell to expand.
	 * @param row       Row (1-based) of cell to expand.
	 * @param attrValue BFS layer index.
	 * @param attrType  Attribute type for grid matching.
	 */
	void bfsExpandCell(int16 col, int16 row, int16 attrValue, int16 attrType);

	/**
	 * Read grid attribute value for a given type and 0-based position.
	 * Helper for BFS code that uses attrType as an index.
	 */
	byte getGridAttrByType(int16 attrType, int16 row0, int16 col) const;

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

	// IDA: byte_4A181E — combinedAttr lookup base: combinedAttr = attr1 + kCombinedAttrBase[attr3]
	static const byte kCombinedAttrBase[5];

	// IDA: word_4A185C — row/column validity for pattern placement (0=invalid)
	static const int16 kRowColValidity[13];

	// IDA: word_4A17EA — challenge pattern attr type pool (12 entries)
	static const int16 kPatternAttrType[13];
	// IDA: word_4A1804 — challenge pattern attr value pool (12 entries)
	static const int16 kPatternAttrValue[13];
	// IDA: word_4A14A6 — challenge pattern extra index pool (12 entries)
	static const int16 kPatternAttrExtra[13];

	// IDA: word_4A14C0 — BFS layer offset by attrType.
	// obstacle combinedAttr = attrValue + kPatternAttrExtra[kObstacleBFSOffset[attrType]]
	static const int16 kObstacleBFSOffset[5];

	// IDA: dword_4A1650 — exit/entry positions (packed X,Y per entry)
	static const Common::Point kEntryPositions[12];

	// IDA: unk_4A15A8 — initial staging positions for zoombini runners (20 slots)
	static const Common::Point kInitialPositions[20];

	// =================================================================
	// Member variables
	// =================================================================

	// --- Core puzzle state ---
	ZmbPuzzleDifficultyLevel _difficultyLevel = kPuzzleDiffLevel1;
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

	/** 3 REGS grid patterns (12x12), loaded from REGS 15000-15002. IDA: dword_4AC038/03C/040 */
	int16 _gridPattern[3][12][12];

	/** 12x13 occupancy grid. 0=empty, nonzero=occupied. IDA: byte_4AC684 */
	byte _gridOccupancy[12][13];
	/** 12x13 exit reservation grid for the enter/rotate/cross handoff. IDA: byte_4AC691 */
	byte _gridExitReservation[12][13];
	/** 12x13 attribute type 1 grid (hair). IDA: byte_4AC685 */
	byte _gridAttr1[12][13];
	/** 12x13 attribute type 2 grid (eyes). IDA: byte_4AC686 */
	byte _gridAttr2[12][13];
	/** 12x13 attribute type 3 grid (nose/feet). IDA: byte_4AC687 */
	byte _gridAttr3[12][13];
	/** 12x13 combined attr grid = attr1 + kCombinedAttrBase[attr3]. IDA: unk_4AC688 */
	byte _gridCombinedAttr[12][13];

	/** Cell bounding rects for hit testing. IDA: word_4AC67C/67E/unk_4AC680/682 */
	Common::Rect _gridCellRect[12][13];

	/** Cell center positions (pixel coords). Computed from REGS 100 + column offsets. */
	Common::Point _gridCellPos[12][13];

	// --- Challenge pattern state (fleens_generateChallengePatterns) ---
	int16 _patternType[13] = {};     // word_4AEC9E — attr type per pattern (1/2/3)
	int16 _patternValue[13] = {};    // word_4AECA0 — attr value per pattern
	int16 _patternExtra[13] = {};    // word_4AECA2 — extra index per pattern
	int16 _patternUsageCount[13] = {}; // word_4A1874 — usage count per pattern (max 2)
	int16 _patternMask[13] = {};     // word_4A188C — 0=masked out for this row
	int16 _rowShuffle[13] = {};      // word_4A18A8 — row removal shuffle array

	// --- Obstacle entry point table (for diff 3/4) ---
	int16 _obstacleEntryCols[16] = {};   // word_4AE36E — obstacle entry column
	int16 _obstacleEntryType[16] = {};   // word_4AE370 — obstacle attr type
	int16 _obstacleEntryValue[16] = {};  // word_4AE372 — obstacle attr value
	int16 _obstacleEntryExtra[16] = {};  // word_4AE374 — obstacle extra
	int16 _obstacleEntryCount = 0;

	/** Obstacle placement grid: 1 if an obstacle occupies this cell. IDA: unk_4AE236 */
	byte _obstacleGrid[12][13] = {};

	/** Obstacle attr type used for ALL obstacles (single global value). IDA: word_4AE370 byte access */
	byte _obstacleAttrType = 0;

	// --- BFS obstacle pathfinding arrays ---
	// Per-layer: 507 entries (13 rows * ~39 cols, padded). Layers indexed by attr value.
	// Row indexing is 1-based (1..12) within BFS arrays. Index: 507*layer + 13*row + col.
	static const int kMaxBFSLayers = 5;
	static const int kBFSEntriesPerLayer = 507;
	static const int kMaxBFSEntries = kBFSEntriesPerLayer * kMaxBFSLayers; // 2535

	/** BFS visited count per cell. IDA: word_4AD10C */
	int16 _bfsVisited[kMaxBFSEntries] = {};
	/** BFS direction to reach cell (44 = unvisited). IDA: unk_4ACFBA */
	int16 _bfsDirection[kMaxBFSEntries] = {};
	/** BFS distance from seed cells. IDA: unk_4ACE68 */
	int16 _bfsDistance[kMaxBFSEntries] = {};

	/** BFS expansion queue — (col, row) pairs. IDA: word_4AECF0/word_4AECF2 */
	static const int kBFSQueueMax = 144;
	int16 _bfsQueueCol[kBFSQueueMax] = {};
	int16 _bfsQueueRow[kBFSQueueMax] = {};
	int16 _bfsQueueHead = 0;  // write position. IDA: word_4AEF30
	int16 _bfsQueueTail = 0;  // read position. IDA: word_4AEF32

	// --- REGS coordinate tables (loaded from resources) ---
	Common::Array<int16> _regsXTable;    // REGS 100 X coords
	Common::Array<int16> _regsYTable;    // REGS 100 Y coords
	Common::Array<int16> _regsDeltaX;    // REGS 200 X deltas (path interpolation)
	Common::Array<int16> _regsDeltaY;    // REGS 200 Y deltas (path interpolation)

	// --- Per-runner state ---
	static const int kMaxRunners = 36;
	ZmbLillyRunnerState _runnerStates[kMaxRunners];
	ZmbFeature *_zmbRunners[kMaxRunners] = {};
	int16 _activeBfsRunners[kMaxRunners] = {};
	int16 _activeBfsRunnerCount = 0;

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
	int16 _pendingReadyQueue[kMaxMoveQueueSize];
	int16 _pendingReadyCount = 0;
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
	int16 _arrivedCount = 0;   // IDA: lilly_stateVar8. Counts zmb node arrivals.
	int16 _completedZmbCount = 0; // IDA: word_4AE772. Counts zmbs that fully completed exit.

	// --- Move phase state ---
	int16 _movePhaseFlag = 0;

	// --- Cell selection state (interactive loop) ---
	bool _bCellSelectActive = false;
	int16 _cellSelectState = 0;
	int16 _selectedCellIdx = -1;
	int16 _highlightCol = -1;
	int16 _highlightRow = -1;
	int16 _cursorBlinkFrame = 0;     // IDA: word_4A17DE — 4-frame blink cycle index (0-3)
	uint32 _cursorBlinkTimer = 0;    // IDA: dNextRenderFrame of cursor runner
	int16 _selectingRunnerIdx = -1;

	// --- Cell swap state ---
	int16 _swapLevel = 0;           // IDA: fleens_unlockProgress. Frog unlock progress 0..6.
	int16 _swapThreshold = 0;       // IDA: fleens_swapUnlockThreshold. Swaps needed per level.
	int16 _swapCounter = 0;         // IDA: fleens_swapCounter. Current swap count within level.
	int16 _swapPairIdx = 0;
	int16 _swapCellACol = 0, _swapCellARow = 0;
	int16 _swapCellBCol = 0, _swapCellBRow = 0;
	int16 _swapBlinkMax = 3;        // IDA: lilly_stateVar6. Blink cycle count (3 or 5).
	int16 _swapBlinkFrame = 0;      // IDA: lilly_stateVar7. Current blink cycle counter.
	int16 _swapSoundIdx = 0;        // IDA: word_4A18C4. Sound selection index (0-3).

	// --- Grid rendering features ---
	ZmbFeature *_gridRendererFeature = nullptr;
	ZmbFeature *_cursorRunnerFeature = nullptr;
	ZmbFeature *_cellAnimRunnerA = nullptr;
	ZmbFeature *_cellAnimRunnerB = nullptr;
	ZmbFeature *_overlayFeatures[5] = {};
	ZmbFeature *_frogScrbFeature = nullptr;
	ZmbFeature *_frogRunnerFeature = nullptr;

	// --- Exit animation features (one per zoombini) ---
	// IDA: Separate "child runner" features for exit animation SCRB 10129+row.
	// In the original engine, these are the child runners (word_4AE3C2[]),
	// which also served as the main grid traversal features. Since our
	// _zmbRunners[] merge snoid+child roles, we need separate features
	// for exit animations that play concurrently with cross/enter queue SCRBs.
	ZmbFeature *_exitAnimFeatures[kMaxRunners] = {};
};

} // End of namespace Mohawk

#endif
