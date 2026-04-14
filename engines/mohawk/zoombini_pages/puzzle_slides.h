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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_SLIDES_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_SLIDES_H

#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Slides puzzle page (ZoombiniPageType::kSlides).
 * Route 2, Puzzle 3
 *
 * Zoombinis must be placed into slots on a hex grid.
 * Matching attribute patterns let groups glow, which is a visual indicator of a correct arrangement.
 * Only Zoombinis with correct arrangement can proceed to the next page.
 *
 * Grid Structure:
 * - 117 cells (9 wide x 13 tall hex grid), indexed 0-116
 * - 9 fields per cell:
 *   [0] runnerIdx - feature runner handle
 *   [1] state - cell state (500-508)
 *   [2] data - zmb runner idx (for 507/508) or attr type (510-513)
 *   [3-8] links - neighbor cells (NW, W, SW, SE, E, NE), -1 = no neighbor
 *
 * Cell States:
 * - 500: inert/empty (no cell)
 * - 501: walkable path
 * - 502: matched (attribute confirmed)
 * - 504/505: slot base (place zmb here)
 * - 506: connector/empty slot
 * - 507: occupied by zmb
 * - 508: confirmed/locked occupied
 *
 * Attribute Types (for matching):
 * - 510: hair match
 * - 511: eyes match
 * - 512: nose match
 * - 513: legs match
 *
 * IDA entry: puzzleSlides_441F0C
 */
class ZoombiniPuzzleSlides : public ZoombiniPuzzle {
public:
	ZoombiniPuzzleSlides(MohawkEngine_Zoombini *vm);
	~ZoombiniPuzzleSlides() override;

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
	// =========================================================================
	// Cell Grid Constants
	// =========================================================================

	/** Total number of cells in hex grid (9 wide x 13 tall). */
	static const int16 kNumCells = 117;

	/** Number of fields per cell in the grid. */
	static const int16 kFieldsPerCell = 9;

	/** Cell state constants. IDA: hardcoded values in slides_initGridByDifficulty */
	enum CellState : int16 {
		kCellInert = 500,      // Empty/unused cell
		kCellPath = 501,       // Walkable path cell
		kCellMatched = 502,    // Attribute match confirmed
		kCellSlotBase1 = 504,  // Slot base for placing zmb
		kCellSlotBase2 = 505,  // Alternate slot base (level 4)
		kCellConnector = 506,  // Connector cell
		kCellOccupied = 507,   // Occupied by zmb
		kCellLocked = 508      // Locked/confirmed occupied
	};

	/** Attribute type markers for matching. IDA: 510-513 in evalAttrMatchAndAdvance */
	enum AttrType : int16 {
		kAttrHair = 510,
		kAttrEyes = 511,
		kAttrNose = 512,
		kAttrLegs = 513
	};

	/** Adjacency bit flags for 6 hex directions. IDA: slides_adjBitFlags usage */
	enum AdjBit : uint16 {
		kAdjNW = 0x01,
		kAdjW  = 0x02,
		kAdjSW = 0x04,
		kAdjSE = 0x08,
		kAdjE  = 0x10,
		kAdjNE = 0x20
	};

	// =========================================================================
	// Initialization Functions
	// =========================================================================

	/**
	 * Load Zoombinis from active pack at pedestal positions.
	 * IDA: zmb_assignPedestalPositions (implicit in slides_puzzleInit)
	 */
	void loadZoombinisFromPack();

	/**
	 * Initialize the cell grid based on difficulty level.
	 * Sets up cell states, slot positions, and link configurations.
	 * IDA: slides_initGridByDifficulty @ 0x4468F8
	 */
	void initGridByDifficulty();

	/**
	 * Build the hex adjacency table.
	 * Computes neighbor cell indices and adjacency bit flags for all 117 cells.
	 * IDA: slides_buildHexAdjacencyTable @ 0x4436E4
	 */
	void buildHexAdjacencyTable();

	/**
	 * Generate attribute pairings for matching.
	 * Creates attr pairs based on placed Zoombinis.
	 * IDA: slides_generateAttrPairings @ 0x44485A
	 */
	void generateAttrPairings();

	/**
	 * Snapshot Zoombini attributes to per-type arrays.
	 * Fills _zmbHairAttrs, _zmbEyesAttrs, etc.
	 * IDA: slides_snapshotZmbAttrsToArrays @ 0x444EE7
	 */
	void snapshotZmbAttrsToArrays();

	// =========================================================================
	// Input Handling
	// =========================================================================

	/**
	 * End a drag operation and process the drop.
	 * IDA: slides_onClickHandler case 4 (drop logic)
	 * @param dropPos The position where the snoid was dropped.
	 */
	void endDrag(const Common::Point &dropPos);

	/**
	 * Find a snoid whose draw record contains the given point.
	 * Skips template snoids with ID < 10000.
	 * @return The snoid, or nullptr if no snoid at that point.
	 */
	ZmbSnoid *findSnoidAtPoint(const Common::Point &pos) override;

	/**
	 * Return the drag constraint rect for this puzzle.
	 * IDA: slides uses left bank area similar to bridge
	 */
	const Common::Rect &getDragConstraintRect() const override;

	// =========================================================================
	// Grid Cell Functions
	// =========================================================================

	/**
	 * Find which cell contains a given screen position.
	 * IDA: implicit in slides_onClickHandler's hit-test logic
	 * @param pos Screen position to test.
	 * @return Cell index (0-116), or -1 if no cell at position.
	 */
	int16 findCellAtPosition(const Common::Point &pos) const;

	/**
	 * Check if a cell is a valid drop target for a Zoombini.
	 * IDA: slides_onClickHandler case 4 drop validation
	 * @param cellIdx Cell index to check.
	 * @return true if the cell accepts a Zoombini.
	 */
	bool isCellValidDropTarget(int16 cellIdx) const;

	/**
	 * Assign a Zoombini to a slot cell.
	 * Updates cell state and runner references.
	 * IDA: slides_assignZmbToSlot @ 0x447FF9
	 * @param snoid The Zoombini to place.
	 * @param cellIdx Target cell index.
	 */
	void assignZmbToSlot(ZmbSnoid *snoid, int16 cellIdx);

	/**
	 * Move a Zoombini to a cell position.
	 * IDA: slides_moveZmbToCell @ 0x4481FE
	 */
	void moveZmbToCell(ZmbSnoid *snoid, int16 cellIdx);

	/**
	 * Clear a cell to empty state.
	 * IDA: slides_clearCellToEmpty @ 0x448955
	 */
	void clearCellToEmpty(int16 cellIdx);

	/**
	 * Reset a cell to inert state and clear all links.
	 * IDA: slides_resetCellToEmpty @ 0x4496BC
	 */
	void resetCellToEmpty(int16 cellIdx);

	/**
	 * Clear link bits from a cell.
	 * IDA: slides_clearCellLinkBits @ 0x449048
	 */
	void clearCellLinkBits(int16 cellIdx, uint16 bitsToClear);

	/**
	 * Update neighbor flags after cell state change.
	 * IDA: slides_updateNeighborFlags @ 0x449171
	 */
	void updateNeighborFlags(int16 cellIdx);

	// =========================================================================
	// Chain Building and Matching
	// =========================================================================

	/**
	 * Build a chain sequence of linked cells.
	 * IDA: slides_buildChainSequence @ 0x444C16
	 */
	int16 buildChainSequence(int16 startCell, int16 *outChain, int16 maxLen);

	/**
	 * Validate a chain and mark matched cells.
	 * IDA: slides_validateChainAndMarkMatched @ 0x4442A9
	 * @return Number of matches found.
	 */
	int16 validateChainAndMarkMatched();

	/**
	 * Find a runner in locked (508) state.
	 * IDA: slides_findRunnerInState508 @ 0x44481C
	 */
	int16 findRunnerInState508() const;

	/**
	 * Find a runner by matching attribute.
	 * IDA: slides_findRunnerByMatchingAttr @ 0x444DC8
	 */
	int16 findRunnerByMatchingAttr(int16 attrType, int16 attrValue) const;

	/**
	 * Sort Zoombinis by overlap count for optimal pairing.
	 * IDA: slides_sortZmbsByOverlapCount @ 0x444FBF
	 */
	void sortZmbsByOverlapCount();

	/**
	 * Place a matching Zoombini in a cell.
	 * IDA: slides_placeMatchingZmbInCell @ 0x4450A3
	 */
	void placeMatchingZmbInCell(int16 cellIdx, int16 attrType);

	/**
	 * Pick a random matching attribute type.
	 * IDA: slides_pickRandomMatchingAttr @ 0x44533D
	 */
	int16 pickRandomMatchingAttr(int16 snoidIdx) const;

	/**
	 * Activate a link in the chain sequence.
	 * IDA: slides_activateChainLink @ 0x445527
	 */
	void activateChainLink(int16 linkIdx);

	/**
	 * Confirm endpoint matches in a chain.
	 * IDA: slides_confirmEndpointMatches @ 0x445700
	 */
	void confirmEndpointMatches();

	/**
	 * Check first attribute match for a cell.
	 * IDA: slides_checkFirstAttrMatch @ 0x448119
	 */
	bool checkFirstAttrMatch(int16 cellIdx, int16 snoidIdx) const;

	/**
	 * Evaluate attribute match and advance chain.
	 * IDA: slides_evalAttrMatchAndAdvance @ 0x445A1B
	 */
	void evalAttrMatchAndAdvance(int16 cellIdx);

	/**
	 * Evaluate neighbor states for chain propagation.
	 * IDA: slides_evalNeighborStates @ 0x445880
	 */
	void evalNeighborStates(int16 cellIdx);

	/**
	 * Propagate match through the chain.
	 * IDA: slides_propagateMatchChain @ 0x446073
	 */
	void propagateMatchChain();

	/**
	 * Check attribute match outcome.
	 * IDA: slides_checkAttrMatchOutcome @ 0x448D1C
	 */
	int16 checkAttrMatchOutcome(int16 cellIdx, int16 snoidIdx) const;

	// =========================================================================
	// Animation and Travel
	// =========================================================================

	/**
	 * Reset animation states for all features.
	 * IDA: slides_resetAnimStates @ 0x4457C9
	 */
	void resetAnimStates();

	/**
	 * Begin Zoombini travel animation to a cell.
	 * IDA: slides_beginZmbTravel @ 0x4464A3
	 */
	void beginZmbTravel(ZmbSnoid *snoid, int16 targetCell);

	/**
	 * Update water level sound effect.
	 * IDA: slides_updateWaterLevelSFX @ 0x44664B
	 */
	void updateWaterLevelSFX();

	/**
	 * Trigger swap animation between two cells.
	 * IDA: slides_triggerSwapAnimation @ 0x449509
	 */
	void triggerSwapAnimation(int16 cellA, int16 cellB);

	/**
	 * Load SCRB data onto a runner feature.
	 * IDA: slides_loadRunnerSCRB @ 0x44BA68
	 */
	void loadRunnerSCRB(uint16 runnerId, int16 scrbId);

	// =========================================================================
	// Slot Management
	// =========================================================================

	/**
	 * Unlock interactive slots after match.
	 * IDA: slides_unlockInteractiveSlots @ 0x445E20
	 */
	void unlockInteractiveSlots();

	/**
	 * Place next Zoombini in a cell.
	 * IDA: slides_placeNextZmbInCell @ 0x445F75
	 */
	void placeNextZmbInCell(int16 cellIdx);

	/**
	 * Check if there's a pending Zoombini to place.
	 * IDA: slides_hasPendingZmb @ 0x4484AA
	 */
	bool hasPendingZmb() const;

	/**
	 * Scan and reset active cells.
	 * IDA: slides_scanAndResetActiveCells @ 0x4484CF
	 */
	void scanAndResetActiveCells();

	/**
	 * Find a matching Zoombini for a cell.
	 * IDA: slides_findMatchingZmbForCell @ 0x448760
	 */
	int16 findMatchingZmbForCell(int16 cellIdx) const;

	/**
	 * Reassign dead slots.
	 * IDA: slides_reassignDeadSlots @ 0x44899D
	 */
	void reassignDeadSlots();

	/**
	 * Pick next cell for chain linking.
	 * IDA: slides_pickNextCellForLink @ 0x4495C2
	 */
	int16 pickNextCellForLink(int16 currentCell, uint16 dirMask) const;

	/**
	 * Mark matched runners as done.
	 * IDA: slides_markMatchedRunnersDone @ 0x4447E2
	 */
	void markMatchedRunnersDone();

	// =========================================================================
	// Victory Checking
	// =========================================================================

	/**
	 * Check if victory condition is met.
	 * IDA: slides_checkVictoryCondition @ 0x44943A
	 * @return true if all Zoombinis are matched.
	 */
	bool checkVictoryCondition() const;

	// =========================================================================
	// Filter/Callback Functions
	// =========================================================================

	/**
	 * Filter hotspot script for rendering.
	 * IDA: slides_filterHotspotScript @ 0x443D75
	 */
	void filterHotspotScript(ZmbFeature *feature, ZmbHotspotGroup *hsGroup,
	                         Common::Array<ZmbHotspot> &hotspots);

	/**
	 * Filter command by flags.
	 * IDA: slides_filterCommandByFlags @ 0x444028
	 */
	bool filterCommandByFlags(int16 cmd, int16 flags) const;

	/**
	 * Process command queue.
	 * IDA: slides_processCommandQueue @ 0x444144
	 */
	void processCommandQueue();

	/**
	 * Invalidate visual rects for redraw.
	 * IDA: slides_invalidateVisualRects @ 0x4423FD
	 */
	void invalidateVisualRects(uint16 rectIdx, ZmbFeature *feature);

	// =========================================================================
	// Static Data Tables
	// =========================================================================

	/** Pedestal positions for 16 Zoombinis. IDA: 0x4A3CF8 */
	static const Common::Point kSnoidPositions[16];

	/** Cell center positions (117 cells). IDA: 0x4A3B24 */
	static const Common::Point kCellPositions[117];

	/** Primary slot cell indices (26 cells). IDA: 0x4A3D90 */
	static const int16 kSlotCellIndices[26];

	/** Interior/link cell indices (43 cells). IDA: 0x4A3DC4 */
	static const int16 kLinkCellIndices[43];

	/** Even-row link cells (20 cells). IDA: 0x4A3E1A */
	static const int16 kEvenRowLinkCells[20];

	/** Odd-row link cells (20 cells). IDA: 0x4A3E42 */
	static const int16 kOddRowLinkCells[20];

	/** Pair start offsets (16 entries). IDA: 0x4A3ECC */
	static const int16 kPairStartOffsets[16];

	/** Pair spacing array (16 entries). IDA: 0x4A3EE8 */
	static const int16 kPairSpacingArray[16];

	/** Left-arm link cells (18 cells). IDA: 0x4A3F04 */
	static const int16 kLeftArmLinkCells[18];

	/** Right-arm + diagonal link cells (18 cells). IDA: 0x4A3F28 */
	static const int16 kRightArmLinkCells[18];

	/** Left endpoint cells (3 cells). IDA: 0x4A3F4C */
	static const int16 kLeftEndpointCells[3];

	/** Right endpoint cells (3 cells). IDA: 0x4A3F52 */
	static const int16 kRightEndpointCells[3];

	/** Inner link pair cells (12 cells). IDA: 0x4A3F58 */
	static const int16 kInnerLinkPairs[12];

	/** Drag constraint rect. */
	static const Common::Rect kDragConstraint;

	/** Hit-test radius for cell detection. */
	static const int16 kCellHitRadius = 20;

	// =========================================================================
	// Runtime State
	// =========================================================================

	/** Puzzle difficulty level (1-4, 1-based). IDA: slides_difficultyLevel */
	ZmbPuzzleDifficultyLevel _difficultyLevel = kPuzzleDiffLevel1;

	/**
	 * Slot base state for grid initialization.
	 * Default 504, at level 4: 50% chance of 505.
	 * IDA: slides_slotBaseState
	 */
	int16 _slotBaseState = 504;

	/**
	 * Cell spacing for grid positioning.
	 * Default 48, if slotBaseState=505: 24.
	 * IDA: slides_cellSpacing
	 */
	int16 _cellSpacing = 48;

	/**
	 * The hex cell grid.
	 * 117 cells x 9 fields = 1053 int16 values.
	 * IDA: slides_cellGrid @ 0x4B11BE
	 */
	int16 _cellGrid[kNumCells * kFieldsPerCell];

	/**
	 * Adjacency bit flags for each cell.
	 * IDA: slides_adjBitFlags @ 0x4B19F8
	 */
	uint16 _adjBitFlags[kNumCells];

	/**
	 * Mapping from slot index to cell index.
	 * IDA: slides_slotCellMap @ 0x4B1188
	 */
	int16 _slotCellMap[26];

	/**
	 * Number of active slots.
	 * IDA: slides_numSlots @ 0x4B1AE2
	 */
	int16 _numSlots = 0;

	/**
	 * Array of Zoombini runner indices.
	 * IDA: slides_zmbRunnerIdxArr @ 0x4B6D4C
	 */
	int16 _zmbRunnerIdxArr[16];

	/**
	 * Sorted Zoombini indices (by overlap count).
	 * IDA: slides_sortedZmbIndices @ 0x4B1B04
	 */
	int16 _sortedZmbIndices[16];

	/**
	 * Per-Zoombini hair attribute values.
	 * IDA: slides_zmbHairAttrs @ 0x4B1B46
	 */
	int16 _zmbHairAttrs[16];

	/**
	 * Per-Zoombini eyes attribute values.
	 * IDA: slides_zmbEyesAttrs @ 0x4B1B66
	 */
	int16 _zmbEyesAttrs[16];

	/**
	 * Per-Zoombini nose attribute values.
	 * IDA: slides_zmbNoseAttrs @ 0x4B1B86
	 */
	int16 _zmbNoseAttrs[16];

	/**
	 * Per-Zoombini legs attribute values.
	 * IDA: slides_zmbLegsAttrs @ 0x4B1BA6
	 */
	int16 _zmbLegsAttrs[16];

	/**
	 * Used flags for Zoombini placement.
	 * IDA: slides_usedFlags @ 0x4B1BC6
	 */
	int16 _usedFlags[16];

	/**
	 * Pair type array for matching.
	 * IDA: slides_pairTypeArray @ 0x4B1B26
	 */
	int16 _pairTypeArray[16];

	/**
	 * Number of pairs generated.
	 * IDA: slides_numPairs @ 0x4B1B24
	 */
	int16 _numPairs = 0;

	/**
	 * Current match attribute index being evaluated.
	 * IDA: slides_matchAttrIndex @ 0x4B1BEA
	 */
	int16 _matchAttrIndex = 0;

	/**
	 * List of currently active cells (for chain propagation).
	 * IDA: slides_activeCellList @ 0x4B1C18
	 */
	int16 _activeCellList[26];

	/**
	 * Number of active cells in list.
	 */
	int16 _activeCellCount = 0;

	/**
	 * Feature pointers for cells (optional, for quick lookup).
	 * Note: In original, these were implicitly tracked via runner indices.
	 */
	ZmbFeature *_cellFeatures[kNumCells];

	/**
	 * Layer SCRB array for rendering order.
	 * IDA: slides_layerScrbArr
	 */
	int16 _layerScrbArr[9];

	/** Pending body arrangement override (1-4, 0=none). IDA: word_4B110E */
	int16 _pendingBodyArrangement = 0;

	/** Runner ID of Zoombini currently doing a slide travel animation. IDA: word_4B110C */
	uint16 _activeTravelSnoidId = 0;

	/** Travel direction flag (0=finished, 1=traveling). IDA: word_4B1112 */
	int16 _travelState = 0;

	/** Round complete flag for visual updates. IDA: slides_roundComplete (0x4B1006) */
	int16 _roundComplete = 0;

	/** Drag state flag. */
	int16 _isDragging = 0;

	// -------------------------------------------------------------------------
	// Celebration state (IDA: slides_puzzleHoverUpdate @ 0x4427B7)
	// -------------------------------------------------------------------------

	/** One-shot flag: set once celebration starts, never cleared until target reached. IDA: slides_celebrationActive (0x4B1C16) */
	bool _celebrationActive = false;
	/** Number of celebrations played so far. IDA: slides_celebrationIndex (0x4B1C12) */
	int16 _celebrationIndex = 0;
	/** Total celebrations needed before reset (= loaded zmb count). IDA: slides_celebrationTarget (0x4B1C10) */
	int16 _celebrationTarget = 0;
	/** Non-repeat random pool bitmask. IDA: dword_4B1C0C */
	uint32 _celebrationPoolState = 0;
	/** Frame counter of last celebration. IDA: dword_4B1C00 */
	uint32 _celebrationLastFrame = 0;
	/** Number of matched columns (trigger for celebration). IDA: slides_matchCount (0x4B1C14) */
	int16 _matchCount = 0;
	/** Number of loaded Zoombinis (pool size for celebration). IDA: slides_numZoombinis (0x4B1AE8) */
	int16 _loadedZmbCount = 0;
};

} // End of namespace Mohawk

#endif
