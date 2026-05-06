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

#include "common/scummsys.h"
#include "common/archive.h"
#ifdef USE_BINK
#include "video/bink_decoder.h"
#endif

#include "mohawk/video.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_pages/transition_logo.h"

namespace Mohawk {

ZoombiniTransitionLogo::ZoombiniTransitionLogo(MohawkEngine_Zoombini *vm) : ZoombiniTransition(vm, ZoombiniPageType::kLogo) {
}

ZoombiniTransitionLogo::~ZoombiniTransitionLogo() {
	if (_cdtoonsVideo)
		_vm->_video->removeEntry(_cdtoonsVideo);

#ifdef USE_BINK
	if (_binkDecoder) {
		delete _binkDecoder;
		_binkDecoder = nullptr;
	}
#endif

	if (_switchedToTrueColor)
		_vm->_gfx->reinitGraphics(false);
}

void ZoombiniTransitionLogo::open() {
}

void ZoombiniTransitionLogo::loadFeatures() {
	_vm->setNextPage(ZoombiniPageType::kPicker);

	// 1.x: LOGO025.MOV (CDToons)
	// 2.0: LOGO025.BIK (Bink)
	if (_vm->isGameVariant(GF_ZMB_TLC)) {
#ifdef USE_BINK
		// Bink requires a true-color pixel format (2 or 4 bpp).
		// Switch OSystem and internal buffers to true-color for the duration
		// of logo playback; reinitGraphics(false) in the destructor restores CLUT8.
		_vm->_gfx->reinitGraphics(true);
		_switchedToTrueColor = true;
		_vm->_gfx->clearScreens();

		_binkDecoder = new Video::BinkDecoder();
		_binkDecoder->setSoundType(Audio::Mixer::kSFXSoundType);
		if (_binkDecoder->loadFile(VIDEO_PATH_BINK)) {
			_binkDecoder->setOutputPixelFormat(_vm->_system->getScreenFormat());
			_binkDecoder->start();
		} else {
			delete _binkDecoder;
			_binkDecoder = nullptr;

			warning("Failed to load bink video [%s], skip", VIDEO_PATH_BINK);
			close();
			return;
		}
#endif
	} else {
		_vm->_gfx->clearScreens();
		_cdtoonsVideo = _vm->_video->playMovie(VIDEO_PATH_CDTOONS, Audio::Mixer::kSFXSoundType);
		if (!_cdtoonsVideo) {
			warning("Failed to open the CDToons video [%s], skip", VIDEO_PATH_CDTOONS);
			close();
			return;
		}

		_cdtoonsVideo->center();
	}
}

void ZoombiniTransitionLogo::onEveryFrame() {
	if (_vm->isGameVariant(GF_ZMB_TLC)) {
#ifdef USE_BINK
		if (!_binkDecoder || _binkDecoder->endOfVideo()) {
			close();
			return;
		}

		if (_binkDecoder->needsUpdate()) {
			const Graphics::Surface *frame = _binkDecoder->decodeNextFrame();
			if (frame) {
				// Write the decoded frame into the graphics compositor's shape
				// screen so that the normal flushScreens() pipeline delivers it
				// to the display.  The video is 640×480, matching the game screen.
				_vm->_gfx->getShapeScreen()->copyRectToSurface(
					*frame, 0, 0, Common::Rect(frame->w, frame->h));
				_vm->_gfx->setDirty();
			}
		}
#endif
	} else {
		if (!_cdtoonsVideo || _cdtoonsVideo->endOfVideo()) {
			close();
			return;
		}

		_vm->_video->updateMovies();
	}
}

void ZoombiniTransitionLogo::onAnimFrame() {
}

} // End of namespace Mohawk
