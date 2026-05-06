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

#include "mohawk/zoombini_pages/puzzle_base.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

ZoombiniPuzzle::ZoombiniPuzzle(MohawkEngine_Zoombini *vm, ZoombiniPageType pageType)
	: ZoombiniInteractive(vm, pageType) {
}

ZoombiniPuzzle::~ZoombiniPuzzle() {
}

bool ZoombiniPuzzle::confirmMapTransition() {
	// IDA `dlg_askReturnToMap` @ 0x462885 opens MHK_DIALOG_ASK_04.
	// Per-puzzle frame handlers poll `dlg_wResult` and on YES (=3) clear
	// every snoid's occupied bit before invoking shared cleanup.
	const bool isPractice = _vm->_state->inPracticeMode();
	if (!isPractice) {
		ZoombiniDialogResult result = _vm->openMsgBoxDialog(ZoombiniMsgBoxType::kAskGoMapWillLost);
		if (result != ZoombiniDialogResult::kYes)
			return false;

		for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
			ZmbSnoid *snoid = *it;
			if (snoid && snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
				snoid->_packIsOccupied = false;
		}
	}
	return true;
}

void ZoombiniPuzzle::saveStateBeforeMapTransition() {
	// IDA: puzzleDispatch_sharedCleanup -> save_updateZmbPacksOnPuzzleComplete(0, 1).
	saveSnoidsToPack();
	routeNonOccupiedToRestingPack();
}

} // End of namespace Mohawk
