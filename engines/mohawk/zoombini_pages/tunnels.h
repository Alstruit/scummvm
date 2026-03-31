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

#ifndef MOHAWK_ZOOMBINI_PAGES_TUNNELS_H
#define MOHAWK_ZOOMBINI_PAGES_TUNNELS_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Tunnels puzzle page (ZoombiniPageType::kTunnels).
 *
 * Route 1, Puzzle 2: Deep Dark Forest - Zoombinis must navigate through tunnels,
 * matching their attributes to the correct tunnel entrance.
 *
 * Difficulty levels:
 * - Level 0: 2 tunnels, single-attribute rule (e.g., "has blue eyes")
 * - Level 1: 4 tunnels, two single-attribute guards
 * - Level 2: 4 tunnels, two dual-attribute guards (OR within category)
 * - Level 3: 4 tunnels, two cross-category dual-attribute guards (AND)
 *
 * IDA entry: puzzleTunnels_459DCB (0x459dcb)
 */
class ZoombiniInteractiveTunnels : public ZoombiniInteractive {
public:
	ZoombiniInteractiveTunnels(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveTunnels() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;
	void onEveryFrame() override;

protected:
	void onGoButtonActivated() override;
	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;

private:
	void loadZoombinisFromPack();

	/**
	 * Initialize puzzle state variables.
	 * IDA: tunnels_initPuzzleState @ 0x459C5C
	 */
	void initPuzzleState();

	/**
	 * Generate tunnel rules based on difficulty level.
	 * Calls the appropriate level-specific rule generator.
	 */
	void generateRules();

	/**
	 * Level 0: Single attribute rule.
	 * IDA: tunnels_setupLevel1_singleAttr @ 0x45C859
	 */
	void setupLevel0_singleAttr();

	/**
	 * Level 1: Dual guards, single attribute each.
	 * IDA: tunnels_setupLevel2_dualSingleAttr @ 0x45CB51
	 */
	void setupLevel1_dualSingleAttr();

	/**
	 * Evaluate if a Zoombini matches a tunnel rule.
	 * IDA: tunnels_evalAttrRule @ 0x45C65D
	 *
	 * @param snoid The Zoombini to evaluate
	 * @param dropZone The target tunnel zone (1-4)
	 * @return true if the Zoombini matches the tunnel's rule
	 */
	bool evaluateRule(ZmbSnoid *snoid, int16 dropZone);

	/**
	 * Find which tunnel zone a position corresponds to.
	 * @param pos Screen position
	 * @return Zone number (1-4) or 0 if outside all zones
	 */
	int16 getDropZone(const Common::Point &pos);

	/**
	 * Handle Zoombini placement in a tunnel.
	 * @param snoid The Zoombini being placed
	 * @param zone The target tunnel zone
	 * @param isCorrect Whether the placement matches the rule
	 */
	void handleZoombiniPlacement(ZmbSnoid *snoid, int16 zone, bool isCorrect);

	static const Common::Point kSnoidPositions[16];

	/** 4 entry positions for DRAW_ON_REG tunnel entrance runners. IDA: unk_4A7674 */
	static const Common::Point kTunnelEntryPositions[4];

	/** Door index mapping {1, 2, 0, 3}. IDA: dword_4A7684 */
	static const int16 kDoorIndices[4];

	/** Click zone radius for tunnel entrances. IDA: 40 */
	static const int16 kClickZoneRadius = 40;

	// ========================================
	// Core State Variables
	// ========================================

	/** Route difficulty level (0-3). IDA: word_4B7A12 */
	int16 _difficultyLevel = 0;

	/** Puzzle active flag. IDA: word_4B7A0C */
	bool _puzzleActive = false;

	/** Processing frame flag to prevent reentry. */
	bool _processingFrame = false;

	/** Count of Zoombinis that have entered a gate. IDA: word_4B7A0E */
	int16 _enteredCount = 0;

	/** Remaining Zoombini count. IDA: word_4B7A14 */
	int16 _remainingCount = 0;

	/** Total Zoombini count for this level. */
	int16 _totalZmbCount = 0;

	/** Random seed for level-0 gate bias. IDA: word_4B7A10 */
	int16 _level0GateBias = 0;

	/** Post-game flag (all Zoombinis placed). IDA: word_4B7A22 */
	bool _allPlaced = false;

	/** Frame counter for timing. */
	uint32 _lastFrameSnapshot = 0;

	// ========================================
	// Rule System
	// ========================================

	/**
	 * Rule guard structure.
	 * IDA: struct starting at 0x4B796C (13 bytes stride per guard)
	 */
	struct TunnelGuard {
		bool sideFlag = false;     ///< 0=left/bottom, 1=right/top
		uint8 condCount = 0;       ///< Number of conditions (1 or 2)
		uint8 attrType[2] = {};    ///< Attribute category (1=hair, 2=eyes, 3=nose, 4=feet)
		uint8 attrValue[2] = {};   ///< Attribute value (1-5)
	};

	/** Number of active guards (1 or 2). IDA: tunnels_ruleGuardCount */
	int16 _guardCount = 0;

	/** Guard rules (up to 2). IDA: tunnels_rule0/rule1 structs */
	TunnelGuard _guards[2];

	// ========================================
	// Per-Gate State
	// ========================================

	/** Per-gate wrong-attempt counter. IDA: word_4B7A38[4] */
	int16 _wrongAttempts[4] = {};

	/** Per-gate occupancy count. IDA: word_4B7AD4/D6/D8/DA */
	int16 _gateOccupancy[4] = {};

	/** Zoombini IDs at each gate slot. IDA: word_4B7988/79C8/79E8/79A8 (16 slots per gate) */
	uint16 _gateSlots[4][16] = {};

	// ========================================
	// Puzzle-specific feature runners
	// ========================================

	/** Feedback animation runner (SCRB 9000). IDA: word_4B7AE0 */
	ZmbFeature *_feedbackFeature = nullptr;
	/** 4 tunnel entrance DRAW_ON_REG runners (SCRB 5000-5003). IDA: scrb_drawOnRegRunnerIdxArr */
	ZmbFeature *_tunnelEntryFeatures[4] = {};
	/** Path effect runner (SCRB 7001). IDA: word_4B7AE6 */
	ZmbFeature *_pathEffectFeature = nullptr;
	/** 4 door animation runners (SCRB 6000-6003). IDA: word_4B7A18[] */
	ZmbFeature *_doorAnimFeatures[4] = {};
	/** Main path runner (SCRB 7000). IDA: word_4B7A16 */
	ZmbFeature *_mainPathFeature = nullptr;

	enum {
		kResSound996_DepartSFX = 996
	};
};

} // End of namespace Mohawk

#endif
