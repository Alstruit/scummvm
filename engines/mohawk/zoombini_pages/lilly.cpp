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

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_pages/lilly.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

ZoombiniInteractiveLilly::ZoombiniInteractiveLilly(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kLilly) {
}

ZoombiniInteractiveLilly::~ZoombiniInteractiveLilly() {
}

void ZoombiniInteractiveLilly::open() {
	openArchive(ZMB_MHK_LILLY);
}

void ZoombiniInteractiveLilly::setBackgroundMusic() {
	// IDA: diff == 2 -> random(20076,20077); diffLevel <= 1 -> 20075; else random(20075,20077)
	if (_difficultyLevel <= 1)
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20075), Audio::Mixer::kMusicSoundType);
	else
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20075), Audio::Mixer::kMusicSoundType);
}

void ZoombiniInteractiveLilly::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniInteractiveLilly::loadFeatures() {
	// IDA: lilly_puzzleInit (0x422de4)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel() + 1;

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(&stru_4A1594, 0x1B58u) — shapes at tBMP 7000
	_vm->_gfx->preloadImage(7000);

	// IDA: shape_loadSubShapesFromArchive(&stru_4A14CC, 0x32C8u) — shapes at tBMP 13000
	_vm->_gfx->preloadImage(13000);

	// Load feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 11000)
	// IDA: scrb_useFeatureGroup(0, 1, 14000)
	// IDA: scrb_useFeatureGroup(0, 2, 10000)

	// Load main features: 1 SCRB at 11000
	// IDA: scrb_loadMainFeatureSet(1, 11000)
	ZmbFeature *mainFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 11000, 0,
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 5, 0x36B0) — 5 subs at 14000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 5; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 7000), 14000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 167, 0x2710) — 167 subs at 10000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 167; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 7000), 10000 + i);
		}
	}

	// Load REGS resources
	// IDA: maze_loadTwoREGS(0x64) — REGS 100
	// IDA: maze_loadTwoREGS(0x2710) — REGS 10000
	// IDA: maze_loadTwoREGS(0xC8) — REGS 200
	// IDA: maze_loadAndSwapREGS(0x3A98) — REGS 15000
	// IDA: maze_loadAndSwapREGS(0x3A99) — REGS 15001
	// IDA: maze_loadAndSwapREGS(0x3A9A) — REGS 15002

	// NOTE: Lilly does NOT use zmb_layoutStaticAndWalkInGroups.
	// It positions Zoombinis manually on lily pads.

	// IDA: zmb_loadAnimationsFromActivePack(0)
	// IDA: lilly_totalZmbCount = *(_WORD *)puzzle_collectAllZmbTraitBytes()
	loadZoombinisFromPack();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(7000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagLilly);

	// IDA: sound_activeHandle = 20075 — lilly narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20075);
}

void ZoombiniInteractiveLilly::onGoButtonActivated() {
	// IDA: lilly_onClickHandler case 2
	// Route 2: Lilly -> Basecamp2 (via Xfer)
	_vm->_xferSrcSiPage = ZMB_SI_LILLY_08;
	_vm->setNextPage(ZoombiniPageType::kXfer);
	close();
}

void ZoombiniInteractiveLilly::loadZoombinisFromPack() {
	ZmbStateFile &f = _vm->_state->_f;

	// IDA: All snoids are placed offscreen at (680, 220) with render disabled.
	// They are animated onto the grid during gameplay.
	const Common::Point offscreenPos(680, 220);
	uint16 posIdx = 0;

	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
		ZmbStateActiveEntry &entry = f._zmbPackActive._entries[i];
		if (!entry._bIsOccupied)
			continue;

		uint16 snoidId = 10000 + posIdx;

		ZmbSnoid *snoid = loadSnoidFromPack(snoidId, offscreenPos,
		                                    ZmbFeature::FLAG_00000001_TYPE_SNOID);
		if (snoid) {
			snoid->_trait = entry._traits;
			snoid->_name = entry.getU32Name(_vm);
			snoid->_packIsOccupied = true;
			// IDA: wBoolDoRender = 0 — hidden until grid placement
			snoid->deactivateRender();
		}
		posIdx++;
	}

	_totalZmbCount = posIdx;
}

} // End of namespace Mohawk
