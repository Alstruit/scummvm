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

#include "mohawk/console.h"
#include "mohawk/mohawk.h"
#include "mohawk/sound.h"
#include "mohawk/video.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_pages/dialog_base.h"

namespace Mohawk {

ZoombiniDialog::ZoombiniDialog(MohawkEngine_Zoombini *vm, ZoombiniPageType pageType) : ZoombiniPage(vm, ZoombiniPageCategory::kDialog, pageType) {
	_useFadeEffect = false;

	// Backup current back screen
	_vm->_gfx->createScreen(_capturedBackScreen);
	_vm->_gfx->captureScreen(ZoombiniGraphics::kBackScreen, &_capturedBackScreen);
}

ZoombiniDialog::~ZoombiniDialog() {
	// Restore captured back screen
	_vm->_gfx->copyToScreen(ZoombiniGraphics::kBackScreen, _capturedBackScreen);
}

void ZoombiniDialog::setBackgroundBitmap() {
	// Capture current composed screen as a background
	_vm->_gfx->captureComposedScreen(ZoombiniGraphics::kBackScreen);
	scheduleForceRedraw();
}

} // End of namespace Mohawk
