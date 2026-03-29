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
 * Route 1, Puzzle 2: Zoombinis must navigate through tunnels,
 * matching their attributes to the correct tunnel entrance.
 * Number of tunnels varies by difficulty (16/18/20/22).
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

protected:
	void onGoButtonActivated() override;
	void onEveryFrame() override;

private:
	void loadZoombinisFromPack();

	static const Common::Point kSnoidPositions[16];

	/** Route difficulty level (0-3). IDA: word_4B7A12 */
	int16 _difficultyLevel = 0;

	enum {
		kResSound996_DepartSFX = 996
	};
};

} // End of namespace Mohawk

#endif
