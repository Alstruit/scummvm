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

#ifndef MOHAWK_ZOOMBINI_PAGES_FLEENS_H
#define MOHAWK_ZOOMBINI_PAGES_FLEENS_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Fleens puzzle page (ZoombiniPageType::kFleens).
 *
 * Route 2, Puzzle 2: Zoombinis must avoid Fleens (floating creatures)
 * that capture them if they have wrong attributes. The player launches
 * Zoombinis on a raft while managing Fleen patrol patterns.
 *
 * IDA entry: fleens_initAndSetupPuzzle (0x41C3DC)
 */
class ZoombiniInteractiveFleens : public ZoombiniInteractive {
public:
	ZoombiniInteractiveFleens(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveFleens() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;

private:
	void loadZoombinisFromPack();

	static const Common::Point kSnoidPositions[16];

	/** Raft DRAW_ON_REG position. IDA: dword_4A1028 */
	static const Common::Point kRaftPosition;

	/** Route difficulty level. IDA: fleens_routeLevel */
	int16 _difficultyLevel = 0;

	// Puzzle-specific feature runners
	/** Animation runner (SCRB 1000). IDA: word_4AB1A4 */
	ZmbFeature *_animFeature = nullptr;
	/** Raft DRAW_ON_REG runner (SCRB 1100). IDA: scrb_drawOnRegRunnerIdxArr[0] */
	ZmbFeature *_raftFeature = nullptr;
	/** 7 overlay runners (SCRB 1200-1206). IDA: word_4AA848[] */
	ZmbFeature *_overlayFeatures[7] = {};
};

} // End of namespace Mohawk

#endif
