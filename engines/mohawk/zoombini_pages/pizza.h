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

#ifndef MOHAWK_ZOOMBINI_PAGES_PIZZA_H
#define MOHAWK_ZOOMBINI_PAGES_PIZZA_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Pizza puzzle page (ZoombiniPageType::kPizza).
 *
 * Route 1, End puzzle: Zoombinis order pizzas with specific toppings.
 * The player must figure out which toppings each Zoombini wants by
 * trial and error. Correct toppings make the Zoombini happy.
 *
 * IDA entry: puzzlePizza_43B394
 */
class ZoombiniInteractivePizza : public ZoombiniInteractive {
public:
	ZoombiniInteractivePizza(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractivePizza() override;

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

	/** Difficulty level (0-3). IDA: pizza_difficultyLevel */
	int16 _difficultyLevel = 0;

	enum {
		kResSound996_DepartSFX = 996
	};
};

} // End of namespace Mohawk

#endif
