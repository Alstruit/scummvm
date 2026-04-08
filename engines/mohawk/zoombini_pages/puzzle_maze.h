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
 * Maze puzzle page (ZoombiniPageType::kMaze).
 *
 * Route 2, Puzzle 2: Zoombinis navigate a grid maze with attribute-based
 * path selection. The maze has a large grid runner setup with multiple
 * feature groups and complex spatial relationships.
 * Uses MAZE2.MHK archive.
 *
 * IDA entry: puzzleMaze2_42E47C (0x42e47c)
 */
class ZoombiniInteractiveMaze : public ZoombiniPuzzle {
public:
	ZoombiniInteractiveMaze(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveMaze() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;
	void onEveryFrame() override;

private:
	void loadZoombinisFromPack();
	
	/**
	 * Load REGS configuration for the current difficulty level.
	 * IDA: maze_loadRegsConfigByLevel (0x4319C9)
	 * Selects REGS resource 16600-16609 based on level and variant.
	 */
	void loadRegsConfigByLevel();
	
	/**
	 * Load and parse REGS data for creature/obstacle configuration.
	 * Parses the REGS resource and populates _creatureSlots[].
	 */
	void loadAndParseRegsData();
	
	/**
	 * Create creature feature runners based on parsed REGS data.
	 * IDA: Multiple loops in puzzleMaze2_42E47C creating creature runners.
	 */
	void createCreatureFeatures();

	static const Common::Point kSnoidPositions[20];
	
	// --- Creature slot lookup tables (IDA: word_4A1CB4, word_4A1CD0, word_4A1CEC) ---
	
	/** Has shadow flag for each slot (0-13). IDA: word_4A1CB4 */
	static const int16 kCreatureHasShadow[14];
	
	/** Creature type ID for each slot (0-13): 0=base, 1=type1, 2=type2. IDA: word_4A1CD0 */
	static const int16 kCreatureTypeId[14];
	
	/** SCRB resource ID for each slot (0-13). IDA: word_4A1CEC */
	static const int16 kCreatureScrbId[14];

	/** Route difficulty level (0-3). IDA: maze_difficultyLevel */
	int16 _difficultyLevel = 0;
	
	// --- REGS config state (IDA: maze_loadRegsConfigByLevel globals) ---
	
	/** REGS resource ID for current level (16600-16609). IDA: computed from maze_levelVariantIdx* */
	int16 _regsResourceId = 16600;
	
	/** Level variant index for randomization. IDA: word_4A1ACC */
	int16 _levelVariantIdx = 0;
	
	// --- Parsed REGS data (IDA: maze_regsDataPtr contents) ---
	
	/** Total creature count from REGS[0]. IDA: word_4AFF80 */
	int16 _totalCreatureCount = 0;
	
	/** Creature slot index for each maze column (1-9). 0 = no creature. */
	int16 _creatureSlots[10] = {};
	
	/** Parsed REGS data buffer. */
	Common::Array<int16> _regsData;

	// --- Feature runners from IDA puzzleMaze2_42E47C ---

	/** IDA: word_4AF2FA. SCRB 12001, OVERLAY|LOOP_ANIM|DEFER_ANIM|PLAY_ONCE. */
	ZmbFeature *_overlayAnimFeature = nullptr;

	/** IDA: word_4AF3F6[0]. SCRB 9005, DEFER_ANIM|PLAY_ONCE|LOOP_ANIM. */
	ZmbFeature *_creatureBaseFeature = nullptr;
	
	/**
	 * Creature feature runners per slot (0-2). IDA: word_4AF3F6[1..2].
	 * Created based on creature type from _creatureSlots[].
	 */
	ZmbFeature *_creatureSlotFeatures[3] = {};
	
	/**
	 * Grid cell creature features (0-13). IDA: word_4AF362[].
	 * Each grid column can have one creature feature.
	 */
	ZmbFeature *_gridCreatureFeatures[14] = {};
	
	/**
	 * Creature obstacle features (0-13). IDA: word_4AF3FC[].
	 * Secondary features for creatures with special behaviors.
	 */
	ZmbFeature *_creatureObstacleFeatures[14] = {};
	
	/**
	 * Shadow features for creatures (0-13). IDA: word_4AF418[].
	 * Created when kCreatureHasShadow[slot] is true.
	 */
	ZmbFeature *_creatureShadowFeatures[14] = {};

	/**
	 * IDA: word_4AF45C[12]. NoOp layer placeholders (SCRB 8011) for render Z-ordering.
	 * Original engine uses noOp callbacks (net_stubNoOp2/net_stubNoOp1).
	 */
	ZmbFeature *_noopFeatures[12] = {};

	/** IDA: SCRB 8004, OVERLAY. Final overlay layer. */
	ZmbFeature *_finalOverlayA = nullptr;

	/** IDA: SCRB 8000, OVERLAY. Final overlay layer. */
	ZmbFeature *_finalOverlayB = nullptr;

	// --- Animation event state ---

	/** Pending body arrangement shape (1-4, or 0 for none). IDA: word_4AB7C6 */
	int16 _pendingBodyArrangement = 0;

	// Celebration state (IDA: maze2_onHover_frameUpdate @ 0x42FF46)
	/** Victory flag: set when all zoombinis reach the end. IDA: word_4B040C */
	bool _celebrationTrigger = false;
	/** Number of celebrations played. IDA: word_4B040A */
	int16 _celebrationsPlayed = 0;
	/** Target celebration count. IDA: word_4B0408 */
	int16 _celebrationTarget = 0;
	/** Non-repeat random pool bitmask. IDA: dword_4B0404 */
	uint32 _celebrationPoolState = 0;
	/** Frame counter of last celebration. IDA: dword_4B0400 */
	uint32 _celebrationLastFrame = 0;
	/** Number of loaded Zoombinis (pool size). IDA: word_4AF306 */
	int16 _loadedZmbCount = 0;
};

} // End of namespace Mohawk

#endif
