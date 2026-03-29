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

#ifndef MOHAWK_ZOOMBINI_PAGES_SLIDES_H
#define MOHAWK_ZOOMBINI_PAGES_SLIDES_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Mudball Wall / Slides puzzle page (ZoombiniPageType::kSlides).
 *
 * Route 2, End puzzle: Zoombinis must be placed into slots on a
 * hex grid. Matching attribute patterns let groups slide down.
 * At highest difficulty, NODE/PATH walking is enabled.
 *
 * IDA entry: puzzleSlides_441F0C
 */
class ZoombiniInteractiveSlides : public ZoombiniInteractive {
public:
	ZoombiniInteractiveSlides(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveSlides() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;

private:
	void loadZoombinisFromPack();

	static const Common::Point kSnoidPositions[16];

	/** Difficulty level (0-3). IDA: slides_difficultyLevel */
	int16 _difficultyLevel = 0;
};

} // End of namespace Mohawk

#endif
