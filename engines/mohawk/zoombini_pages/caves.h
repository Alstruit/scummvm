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

#ifndef MOHAWK_ZOOMBINI_PAGES_CAVES_H
#define MOHAWK_ZOOMBINI_PAGES_CAVES_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Caves puzzle page (ZoombiniPageType::kCaves).
 *
 * Route 4, Puzzle 1: Zoombinis must enter the correct cave based on
 * hieroglyph patterns. Each cave entrance has a pattern that the player
 * must match with Zoombini attributes.
 *
 * IDA entry: caves_funcInit (0x416978)
 */
class ZoombiniInteractiveCaves : public ZoombiniInteractive {
public:
	ZoombiniInteractiveCaves(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveCaves() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;

private:
	void loadZoombinisFromPack();
	
	/**
	 * Initialize difficulty parameters for caves puzzle.
	 * IDA: caves_initDifficultyParams_41896E
	 * Sets up entrance count and SCRB IDs based on difficulty level.
	 */
	void initDifficultyParams();
	
	/**
	 * Setup glyph patterns for cave entrances.
	 * IDA: caves_glyphSetupDispatch_418A6E
	 * Initializes entrance attribute patterns, counts distribution, builds timing table.
	 */
	void setupEntranceGlyphs();
	
	/**
	 * Initialize random entrance attribute patterns using Fisher-Yates shuffle.
	 * IDA: caves_initEntranceAttrPattern_418A7E
	 */
	void initEntranceAttrPattern();
	
	/**
	 * Count glyph distribution across loaded Zoombinis.
	 * IDA: caves_countGlyphDistribution_418BFE
	 */
	void countGlyphDistribution();
	
	/**
	 * Build glyph animation timing table.
	 * IDA: caves_buildGlyphTimingTable_418F6C
	 */
	void buildGlyphTimingTable();
	
	/**
	 * Distribute attributes to cave entrances.
	 * IDA: caves_entranceAttrDist_418CB1
	 */
	void distributeEntranceAttributes();

	static const Common::Point kSnoidPositions[20];

	/** DRAW_ON_REG positions for cave entrance features (SCRB 7000-7019). IDA: off_4A09BC+1..+20 */
	static const Common::Point kCaveEntrancePositions[20];

	/** Route difficulty level + 1 (1-4). IDA: word_4AAF00 */
	int16 _difficultyLevel = 0;
	
	// --- Glyph system state (IDA: caves_initDifficultyParams globals) ---
	
	/** Number of active entrances (4-7 based on difficulty). IDA: unk_4A08F2 */
	int16 _entranceCount = 4;
	
	/** SCRB ID for glyph panel (6003-6006 based on difficulty). IDA: unk_4A08F0 */
	int16 _glyphPanelScrbId = 6006;
	
	/** Entrance attribute requirements for slots 0-10. IDA: caves_entranceAttrReq[] */
	uint8 _entranceAttrReq[11] = {};
	
	/** Entrance attribute offsets for slots 0-10. IDA: caves_entranceAttrOffset[] */
	uint8 _entranceAttrOffset[11] = {};
	
	/** Glyph timing table for animations. IDA: built by caves_buildGlyphTimingTable_418F6C */
	int16 _glyphTimingTable[11] = {};
	
	/** Current hovered entrance slot. IDA: word_4AAEF4 */
	int16 _hoveredEntranceSlot = 0;
	
	// --- Extended glyph system state (IDA: caves_initEntranceAttrPattern globals) ---
	
	/** Guard complexity (1 or 2). IDA: unk_4A08E4 */
	int16 _guardComplexity = 1;
	
	/** Number of attribute columns used. IDA: word_4A08E2 (typically 5-6) */
	int16 _attrColumnCount = 5;
	
	/** Base attribute types [2]. IDA: word_4AAF02[] */
	int16 _baseAttrTypes[2] = {};
	
	/** Base attribute for second guard. IDA: caves_entranceAttrBase */
	int16 _entranceAttrBase = 0;
	
	/** Attribute columns [2 rows x 5 cols]. IDA: word_4AAF06[5*row+col] */
	int16 _attrColumns[10] = {};
	
	/** Glyph distribution counts [6x6]. IDA: word_4AAF1A[6*i+j] */
	int16 _glyphDistribution[36] = {};
	
	/** Frame-to-slot map for animations. IDA: word_4AAF9E[] */
	int16 _frameToSlotMap[21] = {};
	
	/** Cross-product timing table. IDA: word_4AAFC8[] */
	int16 _crossProductTable[21] = {};
	
	/** Loaded Zoombini trait count. IDA: unk_4A0904 */
	int16 _loadedZmbCount = 0;

	/** 3 entrance animation features. IDA: word_4AB078/7A/7C */
	ZmbFeature *_entranceAnimFeatures[3] = {};
	/** 20 DRAW_ON_REG features for cave doors. IDA: scrb_drawOnRegRunnerIdxArr[0..19] + word_4B7B60[0..3] */
	ZmbFeature *_doorDrawOnRegFeatures[20] = {};
	/** 4 door panel animation features (SCRB 9011-9014). IDA: word_4AB00A-4AB010 */
	ZmbFeature *_doorPanelFeatures[4] = {};
	/** Glyph overlay features for slots 5-11 (SCRB 9004-9010). IDA: word_4AAFF2[5-11] */
	ZmbFeature *_glyphOverlayFeatures[7] = {};
	/** Extra glyph overlay features for slots 16-20 (SCRB 9015-9019). IDA: word_4AAFF2[16-20] */
	ZmbFeature *_extraGlyphOverlayFeatures[5] = {};
	/** SCRB 9014 — main glyph panel animation. IDA: word_4AB010 */
	ZmbFeature *_mainGlyphPanelFeature = nullptr;
	
	/** Glyph panel overlay feature (SCRB 6012). IDA: word_4AB080 (first registration) */
	ZmbFeature *_glyphPanelOverlayFeature = nullptr;
	
	/** Glyph panel REGION_TRACK feature. IDA: word_4AB080 (second registration) */
	ZmbFeature *_glyphPanelRegionFeature = nullptr;
	
	/**
	 * Virtual glyph renderer feature.
	 * Renders entrance glyphs via custom render callback.
	 * IDA: unk_4A090C (uses caves_clearAndInvalidateRect/caves_renderAllEntranceGlyphs_41846A)
	 */
	ZmbFeature *_virtualGlyphRenderer = nullptr;
};

} // End of namespace Mohawk

#endif
