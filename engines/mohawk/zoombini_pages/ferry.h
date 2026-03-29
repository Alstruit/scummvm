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

#ifndef MOHAWK_ZOOMBINI_PAGES_FERRY_H
#define MOHAWK_ZOOMBINI_PAGES_FERRY_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Ferry puzzle page (ZoombiniPageType::kFerry).
 *
 * Route 1, Puzzle 2: Zoombinis must board boats by matching attributes.
 * The captain requires passengers with matching trait combinations.
 *
 * IDA entry: ferry_funcInit (0x41a394)
 */
class ZoombiniInteractiveFerry : public ZoombiniInteractive {
public:
	ZoombiniInteractiveFerry(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveFerry() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;

private:
	void loadZoombinisFromPack();

	static const Common::Point kSnoidPositions[20];

	/** Route difficulty level. IDA: word_4AB112 */
	int16 _difficultyLevel = 0;
};

} // End of namespace Mohawk

#endif
