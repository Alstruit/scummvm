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

#ifndef MOHAWK_ZOOMBINI_PAGES_MAZE_H
#define MOHAWK_ZOOMBINI_PAGES_MAZE_H

#include "mohawk/zoombini_pages/interactive_base.h"

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
class ZoombiniInteractiveMaze : public ZoombiniInteractive {
public:
	ZoombiniInteractiveMaze(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveMaze() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;

private:
	void loadZoombinisFromPack();

	static const Common::Point kSnoidPositions[20];

	/** Route difficulty level (0-3). IDA: maze_difficultyLevel */
	int16 _difficultyLevel = 0;

	// --- Feature runners from IDA puzzleMaze2_42E47C ---

	/** IDA: word_4AF2FA. SCRB 12001, OVERLAY|LOOP_ANIM|DEFER_ANIM|PLAY_ONCE. */
	ZmbFeature *_overlayAnimFeature = nullptr;

	/** IDA: word_4AF3F6[0]. SCRB 9005, DEFER_ANIM|PLAY_ONCE|LOOP_ANIM. */
	ZmbFeature *_creatureBaseFeature = nullptr;

	/**
	 * IDA: word_4AF45C[12]. NoOp layer placeholders (SCRB 8011) for render Z-ordering.
	 * Original engine uses noOp callbacks (net_stubNoOp2/net_stubNoOp1).
	 */
	ZmbFeature *_noopFeatures[12] = {};

	/** IDA: SCRB 8004, OVERLAY. Final overlay layer. */
	ZmbFeature *_finalOverlayA = nullptr;

	/** IDA: SCRB 8000, OVERLAY. Final overlay layer. */
	ZmbFeature *_finalOverlayB = nullptr;
};

} // End of namespace Mohawk

#endif
