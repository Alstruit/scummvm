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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_MAZE_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_MAZE_H

#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Per-runner pathfinding and movement state for the Maze puzzle.
 * Maps to the original engine's per-runner struct fields at byte offsets
 * +88 through +295.
 */
struct ZmbMazeRunnerState {
	int16 col = 0;              // Current grid column. IDA: runner[58] / runner+116
	int16 row = 0;              // Current grid row. IDA: runner[57] / runner+114
	int16 oldCol = 0;           // Previous grid column. IDA: runner[56] / runner+112
	int16 oldRow = 0;           // Previous grid row. IDA: runner[55] / runner+110
	int16 direction = 0;        // Movement direction 0-3. IDA: runner+88 / runner[44]
	int16 columnIdx = -1;       // Column group for Z-ordering. IDA: runner[28] / runner+56
	int16 pixelX = 0;           // Pixel position X. IDA: runner+214
	int16 pixelY = 0;           // Pixel position Y. IDA: runner+216
	int16 companionIdx = -1;    // Companion runner index. IDA: runner+130 / runner[65]
	int16 overlayIdx = -1;      // Overlay runner index. IDA: runner+132 / runner[66]
	int16 seatIdx = -1;         // Which seat (node) the runner entered from. IDA: runner[68]
	int16 nodeIdx = -1;         // Node index for SCRB lookup. IDA: runner[69]
	int16 snoidAnimBase = 0;    // Base SCRS for movement. IDA: runner+138 / runner[69]
	byte footTrait = 0;         // Foot attribute value. IDA: *(runner+239) / runner+191
	int16 cellTypeAtPos = 0;    // Cell type at current position. IDA: runner[100]
	int16 traversalData = 0;    // Traversal parameter. IDA: runner[111]
	int16 waveGroup = 0;        // Wave group (1-8) for this grid node. IDA: runner->hsArr[11].shapeid
	int16 linkedZmbRunnerIdx = -1; // For hitchhiker cells (type 5): waiting zmb's runner idx. IDA: hikerRunner->hsArr[14].pos.x
	bool placed = false;        // Has been placed on grid
	bool moving = false;        // Currently moving through grid
	bool arrived = false;       // Reached exit
	bool exiting = false;       // Playing exit SCRS (14006/14007), waiting for kZmbAnimEventM1_End

	// SCRS animation table (per-direction and special). IDA: runner core188+40..+90
	int16 scrsTable[12];        // [0-3]=dir walk, [4-7]=dir alt, [8]=idle, [9]=special1, [10]=special2, [11]=footIdx

	void clear() {
		col = row = oldCol = oldRow = 0;
		direction = 0;
		columnIdx = -1;
		pixelX = pixelY = 0;
		companionIdx = overlayIdx = -1;
		seatIdx = nodeIdx = -1;
		snoidAnimBase = 0;
		footTrait = 0;
		cellTypeAtPos = 0;
		traversalData = 0;
		waveGroup = 0;
		linkedZmbRunnerIdx = -1;
		placed = moving = arrived = exiting = false;
		memset(scrsTable, 0, sizeof(scrsTable));
	}
};

/**
 * Bubblewonder Abyss puzzle page (ZoombiniPageType::kMaze).
 * Route 4, Puzzle 3
 *
 * Zoombinis navigate a grid maze, riding in the bubble, with attribute-based path selection.
 * The maze has a 13x13 grid, SCRB-driven movement, 9 deferred processing queues,
 * an obstacle system, and grid transformations.
 *
 * IDA entry: puzzleMaze2_42E47C (0x42e47c)
 * IDA hover: maze2_onHover_frameUpdate (0x42F899)
 * IDA click: maze_onClickHandler (0x4301EF)
 */
class ZoombiniPuzzleMaze : public ZoombiniPuzzle {
public:
	ZoombiniPuzzleMaze(MohawkEngine_Zoombini *vm);
	~ZoombiniPuzzleMaze() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;
	Common::String debugGetAnswer() const override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;
	void onEveryFrame() override;
	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onMouseMove(const Common::Point &absPos, const Common::Point &relPos) override;

private:
	// --- Initialization ---
	void loadZoombinisFromPack();
	void loadRegsConfigByLevel();
	void loadAndParseRegsData();
	void createCreatureFeatures();
	void loadRegsCoordinateTables();

	/**
	 * First-click initialization: generate grid, select paths, init grid runners.
	 * IDA: net_initGridAndSelectPaths (0x431A88)
	 */
	void initGridAndSelectPaths();

	/**
	 * Place base nodes on empty grid from static tables.
	 * IDA: Part of net_initGridAndSelectPaths (0x431A88)
	 */
	void generateBaseNodes();

	/**
	 * Initialize the SCRS animation table for a runner based on foot trait.
	 * IDA: net_initRunnerAnimTable (0x434528)
	 */
	void initRunnerAnimTable(int16 runnerIdx);

	/** Initialize all runner anim tables (called on first click). */
	void initAllRunnerAnimTables();

	// --- Path selection (IDA: net_* functions at 0x432052..0x434159) ---

	/** Collect zoombini trait bytes into _zmbTraitAssign. IDA: net_collectZmbAttrs (0x432052) */
	void collectZmbAttrs();

	/** Count attr matches; rebuilds _slotScores. IDA: net_countZmbAttrMatches (0x43217C) */
	int16 countZmbAttrMatches(int16 attrIdx);

	/** Collect only zmbs matching attrIdx. IDA: net_collectMatchingZmbAttrs (0x4320C1) */
	int16 collectMatchingZmbAttrs(int16 attrIdx);

	/** Init connection table from free slots. IDA: net_initConnectionTable (0x433218) */
	void initConnectionTable();

	/** Rebuild reachability list. IDA: net_rebuildReachabilityList (0x433249) */
	int16 rebuildReachabilityList();

	/** Fallback: all slots reachable. IDA: net_initAllSlotsReachable (0x433338) */
	void initAllSlotsReachable();

	/** Find best slot in score range. IDA: net_findBestAttrSlotInRange (0x432AEC) */
	int16 findBestAttrSlotInRange(int16 minScore, int16 maxScore);

	/** Find highest-scored slot of same category. IDA: net_findHighestScoredSlotInRange (0x4331A3) */
	int16 findHighestScoredSlotInRange(int16 excludeSlot, int16 minScore, int16 maxScore);

	/** Find highest-scored slot excluding one. IDA: net_findHighestScoredSlot (0x432359) */
	int16 findHighestScoredSlot(int16 excludeSlot);

	/** Get score for slot. IDA: net_getAttrMatchCount (0x432A94) */
	int16 getAttrMatchCount(int16 idx) const;

	/** Count slots with non-zero score. IDA: net_countReachableSlots (0x433184) */
	int16 countReachableSlots();

	/** Init free column list from zero-score slots. IDA: net_initFreeColumnList (0x4332A8) */
	void initFreeColumnList();

	/** Collect active columns. IDA: net_collectActiveColumns (0x4332D9) */
	int16 collectActiveColumns();

	/** Find best next slot with uniqueness. IDA: net_findBestNextSlot (0x4327D6) */
	int16 findBestNextSlot(int16 searchIdx);

	/** Commit best slot. IDA: net_commitBestAttrSlot (0x432B44) */
	int16 commitBestAttrSlot(int16 maxThreshold, int16 minThreshold);

	/** Find and commit next slot. IDA: net_findAndCommitNextSlot (0x4323DF) */
	int16 findAndCommitNextSlot(int16 slotIdx, int16 direction);

	/** Find and commit new attr slot. IDA: net_findAndCommitNewAttrSlot (0x432E6B) */
	int16 findAndCommitNewAttrSlot(int16 maxThreshold, int16 minThreshold);

	// Path selection algorithms (dispatched by difficulty)
	void buildZmbAssignmentAlt2();    // Difficulty 0. IDA: 0x43335F
	void buildZmbAssignmentAlt();     // Difficulty 1 variant 0. IDA: 0x4335EF
	void selectPathSlots2();          // Difficulty 1v1 / 2v0. IDA: 0x4338A1
	void selectPathSlots();           // Difficulty 2v1 / 4. IDA: 0x433D30
	void buildZmbAssignmentList();    // Difficulty 3. IDA: 0x434159

	// --- Grid runner init (IDA: net_initGridRunners 0x431C3A, net_registerGridRunner 0x431D02) ---
	void initGridRunners();
	void registerGridRunner();

	// --- Queue processing (called from onEveryFrame) ---

	void processQueues();
	void processSetupNodeQueue();
	void processMoveQueue();
	void processLinkQueue();
	void processColumnLinkQueue();
	void processScrsPlayQueue();
	void processReorderFlags();
	void processArrivalQueue();
	void processRowChangeQueue();
	void processCrossAssignQueue();

	// --- Queue helpers ---

	/** Load SCRB onto runner at grid node. IDA: net_setupNodeScrb (0x430707) */
	void setupNodeScrb(int16 nodeIdx);

	/** Move runner one cell on grid. IDA: fleens_moveZmbOnRaft (0x434C7C) */
	void moveZmbOnGrid(int16 runnerIdx);

	/** Dispatch based on cell type after row change. IDA: row-change switch in hover handler */
	void handleRowChange(int16 cellType, int16 runnerIdx);

	/** Process runner arrival at exit. IDA: arrival queue processing */
	void handleArrival(int16 direction, int16 runnerIdx);

	/** Assign crossing SCRB pairs. IDA: net_assignCrossRunnerScrbs (0x43462E) */
	void assignCrossRunnerScrbs(int16 runner1Idx, int16 runner2Idx);

	// --- Cell type routing (row-change handler) ---

	/** Turn node arrival. IDA: net_zmbArriveAtNode (0x430049) */
	void zmbArriveAtNode(int16 cellType, int16 runnerIdx);

	/** Straight node arrival (alt). IDA: net_zmbArriveAtNodeAlt (0x434F8B) */
	void zmbArriveAtNodeAlt(int16 nodeRunnerIdx, int16 runnerIdx);

	/** Intersection movement step. IDA: net_moveRunnerStep (0x435290) */
	void moveRunnerStep(int16 nodeRunnerIdx, int16 runnerIdx);

	/** Attribute matching movement step. IDA: net_moveRunnerStepAlt (0x4350B0) */
	void moveRunnerStepAlt(int16 nodeRunnerIdx, int16 runnerIdx);

	/** Setup collision tracking at hitchhiker. IDA: net_zmbSetupCollisionTracking (0x4354D8) */
	void zmbSetupCollisionTracking(int16 nodeRunnerIdx, int16 runnerIdx);

	/** Finalize runner at exit slot. IDA: net_finalizeRunnerAtSlot (0x434E1D) */
	void finalizeRunnerAtSlot(int16 nodeRunnerIdx);

	// --- Click handling ---

	/** Find which seat (0-13) contains the given point, or -1. */
	int16 findSeatAtPoint(const Common::Point &pos) const;

	/** Find an idle snoid at the given point for drag start. */
	ZmbSnoid *findIdleSnoidAtPoint(const Common::Point &pos) const;

	/** Drop a zoombini onto a grid seat and start moving. */
	void handleGridDrop(int16 seatIdx, ZmbSnoid *snoid);

	// --- Obstacle system ---

	/** Spawn obstacle at creature slot. IDA: maze_spawnMovingObstacle (0x42E05D) */
	void spawnObstacle(int16 slotIdx);

	/** Move all active obstacles one step. IDA: maze_updateObstaclePosition (0x42DC56) */
	void moveObstacles();

	/**
	 * Spawn a player-fired projectile in the current launcher direction.
	 * IDA `maze_spawnProjectile @ 0x42D8E9`. 14 u/tick, 8-directional.
	 */
	void spawnProjectile();

	/**
	 * Advance the active projectile one tick. IDA `maze_tickProjectilePosition @ 0x42E2F2`.
	 * Deactivates at frame>15 or on-hit via checkObstacleCollisions.
	 */
	void tickProjectile();

	/** Update score display via SCRB 8011. IDA `maze_updateScoreDigits @ 0x42D04D`. */
	void updateScoreDigits();

	/** Check obstacle collision with runners. IDA: maze_projectileTickAndCollide (0x42DE69) */
	void checkObstacleCollisions();

	// --- SCRB animation dispatch ---

	/** SCRB animation event handler. IDA: net_scrbAnimCallback (0x43105B) */
	void processScrbAnimEvent(ZmbFeature *feature, int16 eventCode);

	/** Collision tracking callback. IDA: net_trackRunnerCollisions (0x431354) */
	void handleCollisionTracking(int16 eventCode, int16 runnerIdx);

	// --- Celebration / idle ---
	void processIdleAnimations();

	// --- Helpers ---

	/** Get trait value by category (1=head, 2=eye, 3=nose, 4=foot). */
	static byte getTraitByCategory(const ZmbTrait &trait, int16 category);

	/** Map a snoid foot trait to SCRS animation base. */
	int16 getRunnerAnimBase(byte footTrait) const;

	/** Get MazeRunnerState for a runner index. */
	ZmbMazeRunnerState *getRunnerState(int16 idx);

	/** Find runner index by snoid ID. */
	int16 findRunnerBySnoidId(uint16 snoidId) const;

	/** Find runner index by feature ID. */
	int16 findRunnerByFeatureId(uint16 featureId) const;

	/** Find seat index by creature obstacle/shadow feature ID. */
	int16 findSeatByFeatureId(uint16 featureId) const;

	// =================================================================
	// Static data tables
	// =================================================================

	/** Pedestal positions for zoombini lineup. IDA: word_4A1F58 */
	static const Common::Point kSnoidPositions[20];

	/** Has shadow flag per creature slot (0-13). IDA: word_4A1CB4 */
	static const int16 kCreatureHasShadow[14];

	/** Creature type ID per slot: 0=base, 1=type1, 2=type2. IDA: word_4A1CD0 */
	static const int16 kCreatureTypeId[14];

	/** SCRB resource ID per creature slot. IDA: word_4A1CEC */
	static const int16 kCreatureScrbId[14];

	/** Pixel position per seat (x, y). IDA: word_4A1BD4/4A1BD6 (14 packed pairs) */
	static const Common::Point kSeatPositions[14];

	/** Grid coordinates per seat (row, col). IDA: word_4A1D46/4A1D48 */
	static const Common::Point kSeatGridCoords[14];

	/** Facing direction per seat (0-3). IDA: word_4A1C0C */
	static const int16 kSeatDirection[14];

	/** Movement entry direction per seat (0-3). IDA: word_4A1C60 */
	static const int16 kSeatMoveDirection[14];

	/** Animation shape per seat. IDA: word_4A1C44 */
	static const int16 kSeatAnimShape[14];

	/** Base node cell types (18 entries). IDA: word_4A1D7E */
	static const int16 kBaseNodeTypes[18];

	/** Base node coordinates (row, col) — 18 pairs. IDA: word_4A1DA2 */
	static const Common::Point kBaseNodeCoords[18];

	/** Attribute offset table: {0, 5, 10, 15} for Hair/Eyes/Nose/Feet. IDA: word_4A2018 */
	static const int16 kAttrOffsets[4];

	/** Arrival positions per direction (4 dirs × 20 positions). IDA: dword_4A1DEA */
	static const Common::Point kArrivalPositions[80];

	/** Creature SCRB IDs for type 0/1/2. IDA: word_4A1D08[0..2] */
	static const int16 kCreatureTypeScrbs[3];

	/** Path selection threshold table. IDA: word_4A204A */
	static const int16 kPathSelectThresholds[20];

	/** Slot-to-category mapping: 0=none, 1=hair, 2=eyes, 3=nose, 4=feet. IDA: word_4A2020 */
	static const int16 kSlotToCategory[21];

	/** Score-to-loop-count mapping. IDA: word_4A208E */
	static const int16 kScoreToLoopCount[17];

	/** Static path pool (shuffled base). IDA: word_4A1FAE */
	static const int16 kStaticPathPool[11];

	/** Seat flag value per seat. IDA: word_4A1C7C */
	static const int16 kSeatFlagValue[14];

	/**
	 * Attribute slot mapping: maps slot index (0-20) to trait category offset.
	 * Usage: kAttrSlotType[slotIdx] maps to ZmbTrait::TraitCategory.
	 * IDA: word_4A1FC4[2*i]
	 */
	static const int16 kAttrSlotType[21];

	/**
	 * Attribute slot mapping: maps slot index (0-20) to trait value (1-5).
	 * IDA: word_4A1FC6[2*i]
	 */
	static const int16 kAttrSlotValue[21];

	// =================================================================
	// Member variables
	// =================================================================

	// --- Difficulty and variant ---
	ZmbPuzzleDifficultyLevel _difficultyLevel = kPuzzleDiffLevel1; ///< Puzzle difficulty level (1-4, 1-based)
	int16 _regsResourceId = 16600;
	int16 _levelVariantIdx = 0;

	// --- Variant cycling (persistent across plays). IDA: maze_levelVariantIdx0..3 ---
	static int16 s_variantIdx0;
	static int16 s_variantIdx1;
	static int16 s_variantIdx2;
	static int16 s_variantIdx3;

	// --- REGS data ---
	int16 _totalCreatureCount = 0;
	int16 _creatureSlots[10] = {};
	Common::Array<int16> _regsData;

	// --- REGS coordinate tables ---
	Common::Array<int16> _regsGridX;
	Common::Array<int16> _regsGridY;
	Common::Array<int16> _regsPathDeltaX;
	Common::Array<int16> _regsPathDeltaY;

	// --- Grid state (13x13) ---
	static const int kGridRows = 13;
	static const int kGridCols = 13;

	int16 _cellTypes[kGridRows][kGridCols];         // IDA: word_4AFCEA[169]
	int16 _cellRunnerIdx[kGridRows][kGridCols];     // IDA: word_4AFB98[169]
	int16 _cellAttrType[kGridRows][kGridCols];      // Per-cell attribute category (1-4), 0=none
	int16 _cellAttrValue[kGridRows][kGridCols];     // Per-cell attribute value (1-5), 0=none
	int16 _nodeDirFlags[kGridRows][kGridCols][4];   // Per-node direction availability (0/1 per dir 0-3). IDA: core_word[34..37]
	int16 _nodeDirection[kGridRows][kGridCols];     // Per-node current direction output. IDA: core_word[38]
	int16 _nodeCycleFlag[kGridRows][kGridCols];     // Per-node cycling flag (1=cycle on pass). IDA: core_word[39]
	Common::Rect _gridCellRect[kGridRows][kGridCols];
	Common::Point _gridCellPos[kGridRows][kGridCols];

	// --- Collision tracking grid. IDA: unk_4AF7A2 (counter), word_4AF7A4 (first runner) ---
	int16 _collisionCount[kGridRows][kGridCols];
	int16 _collisionRunnerIdx[kGridRows][kGridCols];

	// --- Per-runner state ---
	static const int kMaxRunners = 21;
	ZmbMazeRunnerState _runnerStates[kMaxRunners];
	uint16 _zmbRunnerSnoidIds[kMaxRunners];
	int16 _runnerCount = 0;

	// --- Wave group runner arrays (8 activation waves). IDA: word_4B00E0..word_4B036A ---
	// Each grid runner belongs to one of 8 wave groups. When an exit runner finalizes,
	// all runners in its wave group are processed (intersections advance, hitchhikers eject).
	static const int kMaxWaveGroups = 8;
	int16 _waveGroupRunners[kMaxWaveGroups][kMaxRunners]; // IDA: word_4B00E0[0], word_4B023E[2], ...
	int16 _waveGroupCount[kMaxWaveGroups];                // IDA: word_4B03CE, word_4B03D0, ...

	// --- SCRS animation table (foot-trait indexed). IDA: net_scrsAnimTable */
	int16 _scrsAnimTable[96];

	// --- Path selection state ---
	int16 _zmbTraitAssign[80];       // IDA: net_zmbSlotAssignArr_4AF52A[]. 4 per zmb, up to 20
	int16 _slotScores[21];           // IDA: net_slotScoreArr_4AF66A[21]
	int16 _connectionTable[21];      // IDA: word_4AF694[21]
	int16 _freeColumnList[21];       // IDA: word_4AF6BE[21]
	int16 _reachableSlots[21];       // IDA: net_slotReachableArr_4AF6E8[21], 1-indexed
	int16 _activeColumns[21];        // IDA: word_4AF712[21], 1-indexed
	int16 _selectedPathSlots[20];    // IDA: word_4AF766[20]. OUTPUT of path selection
	int16 _pathSlotWriteIdx = 0;     // IDA: word_4AF78E
	int16 _pathSlotReadIdx = 0;      // IDA: word_4AF792
	int16 _reachableSlotCount = 0;   // IDA: net_slotColumnArr_4AF796
	int16 _activeColumnCount = 0;    // IDA: word_4AF79A
	int16 _activeColumnsExist = 0;   // IDA: word_4AF79C
	int16 _committedTraitCount = 0;  // IDA: net_slotAttrMask_4AF7A0
	int16 _uniqueCheckArr[80];       // IDA: word_4AFE40[80]. 4 per entry, 20 max
	int16 _committedTraitArr[80];    // IDA: word_4AFEE0[80]. 4 per entry, 20 max
	int16 _shuffledPathPool[11];     // IDA: local pathSlots[] in initGridAndSelectPaths
	int16 _unknownFlag = 0;          // IDA: unk_4AF302

	// --- Grid runner init ---
	int16 _gridRegsReadIdx = 0;      // IDA: word_4AFF84

	// --- Queues (IDA: 9 deferred processing queues) ---
	static const int kMaxQueueSize = 21;
	static const int kMaxCrossQueueSize = 42;

	int16 _setupNodeQueue[kMaxQueueSize];
	int16 _setupNodeQueueSize = 0;

	int16 _moveQueue[kMaxQueueSize];
	int16 _moveQueueSize = 0;

	int16 _linkQueue[kMaxQueueSize];
	int16 _linkQueueSize = 0;

	int16 _columnLinkQueue[kMaxQueueSize];
	int16 _columnLinkQueueSize = 0;

	int16 _scrsPlayQueue[kMaxQueueSize];
	int16 _scrsPlayQueueSize = 0;

	bool _reorderFlag0 = false;
	bool _reorderFlag1 = false;

	int16 _arrivalQueue[kMaxQueueSize];
	int16 _arrivalQueueSize = 0;
	int16 _rowChangeQueue[kMaxQueueSize];
	int16 _rowChangeQueueSize = 0;

	int16 _crossAssignQueue[kMaxCrossQueueSize];
	int16 _crossAssignQueueSize = 0;

	// --- Arrival position counters (per direction 0-3). IDA: word_4AF522/524/526/528 ---
	int16 _arrivalPosCounter[4] = {};

	// --- Placed runner tracking. IDA: word_4AF31A[10], word_4AF330, word_4AF33A ---
	int16 _placedRunnerIds[10] = {};
	int16 _placedRunnerCount = 0;
	bool _runnersArePlaced = false;

	// --- Seat assignment. IDA: word_4AF33C[14] ---
	int16 _seatAssignment[14] = {};

	// --- Obstacle state ---
	static const int kMaxObstacles = 6;
	struct ObstacleSlot {
		bool active = false;
		int16 row = 0;
		int16 col = 0;
		int16 direction = 0;
		int16 speed = 0;
		int16 timer = 0;
		int16 creatureSlotIdx = -1;
		ZmbFeature *feature = nullptr;
		/** SCRB resource ID — drives tier scoring on hit (1000=1pt, 1005=2, 1016=3, 1021=4). */
		int16 scrbId = 0;
	};
	ObstacleSlot _obstacles[kMaxObstacles];
	int16 _activeObstacleCount = 0;

	// --- Score state (IDA: word_4AF242, word_4AF244, word_4AF246) ---
	int16 _score = 0;
	int16 _scoreThreshold = 100;
	int16 _lives = 0;
	int16 _scoreCounter = 0;

	/**
	 * IDA maze_obstacleScore (cumulative obstacle-hit score) and
	 * maze_bonusCounter (combo/bonus tracking). Each obstacle SCRB tier scores:
	 *   SCRB 1000 = 1 point   (basic obstacle)
	 *   SCRB 1005 = 2 points  (medium)
	 *   SCRB 1016 = 3 points  (hard)
	 *   SCRB 1021 = 4 points  (boss)
	 * `_bonusCounter` increments on consecutive hits, resets on miss.
	 */
	int16 _obstacleScore = 0;
	int16 _bonusCounter = 0;

	/**
	 * IDA dword_4AF264[6]: per-runner sized rect pool used for collision
	 * testing. Each entry is an active runner's bounding rect; the obstacle
	 * collision check intersects against the union of these rects.
	 */
	Common::Rect _activeRunnerRects[6] = {};
	int16 _activeRunnerRectCount = 0;

	// --- Drag state ---
	bool _isDragging = false;
	ZmbSnoid *_dragSnoid = nullptr;
	int16 _dragSourceSlotIdx = -1;
	Common::Point _dragSavedPos;

	// --- Feature runners ---
	ZmbFeature *_overlayAnimFeature = nullptr;
	ZmbFeature *_creatureBaseFeature = nullptr;

	/**
	 * Player-controlled launcher at screen center (320, 240).
	 * IDA `maze_initObstacleRunner @ 0x42D86F` registers SCRB 1010 here.
	 * Points in one of 8 directions; fires a projectile at `projectileSpeed`.
	 */
	ZmbFeature *_launcherFeature = nullptr;
	int16 _launcherDirection = 0; // 0-7 (N, NE, E, SE, S, SW, W, NW)

	/**
	 * Active projectile state. IDA `maze_spawnProjectile @ 0x42D8E9` +
	 * `maze_tickProjectilePosition @ 0x42E2F2`. Projectile moves at 14 u/tick
	 * in the launcher's 8-direction; deactivates at frame>15 or on collision.
	 */
	struct ProjectileState {
		bool active = false;
		int16 x = 0, y = 0;
		int16 dx = 0, dy = 0;
		int16 lifeFrames = 0; // auto-deactivate at >15
	};
	ProjectileState _projectile;

	/** IDA `maze_updateScoreDigits @ 0x42D04D`: score display runner (SCRB 8011). */
	ZmbFeature *_scoreDigitFeature = nullptr;
	ZmbFeature *_creatureSlotFeatures[3] = {};
	ZmbFeature *_gridCreatureFeatures[14] = {};
	ZmbFeature *_creatureObstacleFeatures[14] = {};
	ZmbFeature *_creatureShadowFeatures[14] = {};
	ZmbFeature *_noopFeatures[12] = {};
	ZmbFeature *_finalOverlayA = nullptr;
	ZmbFeature *_finalOverlayB = nullptr;

	// --- Puzzle phase ---
	bool _gridInitialized = false;  // First click triggers grid generation
	bool _puzzleReady = false;      // IDA: word_4AF338
	bool _reentryGuard = false;     // IDA: word_4A1FAC
	bool _animTablesInitialized = false;  // IDA: unk_4AF308

	// --- Animation event state ---
	int16 _pendingBodyArrangement = 0;
	int16 _soundAlternator = 0;          // IDA: word_4A1AD6. Alternates 0/1 for 5101/5102 sounds

	// --- Celebration state (IDA: maze2_onHover_frameUpdate @ 0x42FF46) ---
	bool _celebrationTrigger = false;
	int16 _celebrationsPlayed = 0;
	int16 _celebrationTarget = 0;
	uint32 _celebrationPoolState = 0;
	uint32 _celebrationLastFrame = 0;

	// --- Zoombini count ---
	int16 _loadedZmbCount = 0;
	int16 _arrivedZmbCount = 0;

	/**
	 * Go (advance) button enabled flag.
	 * IDA: lilly_bAdvanceEnabled (0x4AE9F2) — shared with the Lilly engine.
	 * maze_runnerArriveAtNode_425ADB @ 0x425b65 sets it to 1 when the FIRST
	 * Zoombini reaches a path node (++lilly_stateVar8 == 1).
	 * maze_invalidateVisualRects @ 0x4238bf renders the Go button enabled only
	 * while this flag is set, so the button stays disabled until one Zoombini
	 * has begun crossing the maze.
	 */
	bool _bAdvanceEnabled = false;

	/** Count of Zoombinis that have arrived at a path node. IDA: lilly_stateVar8. */
	int16 _nodeArrivalCount = 0;
};

} // End of namespace Mohawk

#endif
