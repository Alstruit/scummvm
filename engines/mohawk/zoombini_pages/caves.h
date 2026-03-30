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

	static const Common::Point kSnoidPositions[20];

	/** DRAW_ON_REG positions for cave entrance features (SCRB 7000-7019). IDA: off_4A09BC+1..+20 */
	static const Common::Point kCaveEntrancePositions[20];

	/** Route difficulty level + 1 (1-4). IDA: word_4AAF00 */
	int16 _difficultyLevel = 0;

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
};

} // End of namespace Mohawk

#endif
