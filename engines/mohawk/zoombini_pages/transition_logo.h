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

#ifndef MOHAWK_ZOOMBINI_PAGES_TRANSITION_LOGO_H
#define MOHAWK_ZOOMBINI_PAGES_TRANSITION_LOGO_H

#include "mohawk/video.h"
#include "mohawk/zoombini_pages/transition_base.h"

#ifdef USE_BINK
namespace Video {

class BinkDecoder;

} // End of namespace Video
#endif

namespace Mohawk {

class ZoombiniTransitionLogo : public ZoombiniTransition {
public:
	ZoombiniTransitionLogo(MohawkEngine_Zoombini *vm);
	~ZoombiniTransitionLogo() override;

	void open() override;
	void loadFeatures() override;
	void onEveryFrame() override;
	void onAnimFrame() override;

	static constexpr const char *VIDEO_PATH_CDTOONS = "DATA/LOGO025.MOV";
	static constexpr const char *VIDEO_PATH_BINK = "DATA/LOGO025.BIK";

protected:
	VideoEntryPtr _cdtoonsVideo = nullptr;
#ifdef USE_BINK
	Video::BinkDecoder *_binkDecoder = nullptr;
#endif
	bool _switchedToTrueColor = false;
};

} // End of namespace Mohawk

#endif
