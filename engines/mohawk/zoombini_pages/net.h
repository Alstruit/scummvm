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

#ifndef MOHAWK_ZOOMBINI_PAGES_NET_H
#define MOHAWK_ZOOMBINI_PAGES_NET_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Net puzzle page (ZoombiniPageType::kNet).
 *
 * Route 4, Puzzle 3: Zoombinis must navigate a sorting net with
 * attribute-based column filters. The puzzle uses 2-3 columns
 * depending on difficulty, with animated column slots.
 *
 * IDA entry: puzzleNet_4361D4 (0x4361d4)
 */
class ZoombiniInteractiveNet : public ZoombiniInteractive {
public:
	ZoombiniInteractiveNet(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveNet() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;
	void onEveryFrame() override;

private:
	void loadZoombinisFromPack();

	static const Common::Point kSnoidPositions[16];

	/** Route difficulty level (0-3). IDA: net_difficultyLevel */
	int16 _difficultyLevel = 0;

	enum {
		kResSound996_DepartSFX = 996
	};
};

} // End of namespace Mohawk

#endif
