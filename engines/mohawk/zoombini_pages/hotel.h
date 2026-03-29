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

#ifndef MOHAWK_ZOOMBINI_PAGES_HOTEL_H
#define MOHAWK_ZOOMBINI_PAGES_HOTEL_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Hotel puzzle page (ZoombiniPageType::kHotel).
 *
 * Route 3, End puzzle: Zoombinis must be assigned to hotel rooms
 * based on attribute matching. Room assignments become more complex
 * at higher difficulty levels with different SCRB sets loaded.
 *
 * IDA entry: hotel_initAndSetupPuzzle (0x41ede4)
 */
class ZoombiniInteractiveHotel : public ZoombiniInteractive {
public:
	ZoombiniInteractiveHotel(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveHotel() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;
	void onEveryFrame() override;

private:
	void loadZoombinisFromPack();

	static const Common::Point kSnoidPositions[20];

	/** Difficulty level. IDA: hotel difficulty */
	int16 _difficultyLevel = 0;
};

} // End of namespace Mohawk

#endif
