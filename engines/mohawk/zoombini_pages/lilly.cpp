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
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 5, 0x36B0) — 5 subs at 14000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 5; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 14000), 14000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 167, 0x2710) — 167 subs at 10000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 167; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 10000), 10000 + i);
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

	// IDA: lilly_setDifficultyParams
	// Initialize obstacle configuration based on difficulty.
	setDifficultyParams();
	
	// IDA: fleens_initGridWithAttributes (0x427955)
	// Initialize the 12x12 grid with attribute patterns
	initGridWithAttributes();
	
	// IDA: word_4A14C8 — virtual grid renderer with custom callbacks
	// maze_clearAndInvalidateRect as preRender, maze_renderAllGridSprites as render
	_gridRendererFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10000, 0,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	
	// IDA: lilly_cursorRunnerIdx — cursor indicator with custom render
	_cursorRunnerFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10001, 5,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	
	// IDA: lilly_cellAnimRunnerA, lilly_cellAnimRunnerB — cell animation runners
	_cellAnimRunnerA = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10002, 4,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	_cellAnimRunnerB = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10003, 4,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	
	// IDA: word_4AE3C2[] — per-zoombini runners at SCRB 10109+i
	createZoombiniRunners();

	// IDA: 5 overlay features for SCRB 14000-14004, interval=0, flags=OVERLAY
	for (uint16 i = 0; i < 5; i++) {
		_overlayFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 14000), 14000 + i, 0,
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// IDA: lilly_frogScrbIdx — frog event SCRB 11000, diff > 1, interval=5
	if (_difficultyLevel > 1) {
		_frogScrbFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 11000, 5,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);
		
		// IDA: lilly_frogRunnerIdx — frog obstacle runner at SCRB 10078
		// Position (38, 415), deferred anim, initially hidden
		_frogRunnerFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10078, 6,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);
		if (_frogRunnerFeature) {
			_frogRunnerFeature->setPointLoc(Common::Point(38, 415));
			_frogRunnerFeature->deactivateRender();
		}
	}
	
	// NOTE: Original engine used 12 no-op overlay runners (word_4AE3AA) purely for
	// render ordering in its persistent linked-list renderer. ScummVM's per-frame
	// Z-sorting makes these unnecessary.

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
	_departXferSrcSiPage = ZMB_SI_LILLY_08;
	ZoombiniInteractive::onGoButtonActivated();
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

void ZoombiniInteractiveLilly::setDifficultyParams() {
	// IDA: lilly_setDifficultyParams (0x4264AC)
	// Initialize difficulty parameters based on route level.
	// These determine obstacle configuration for the lily pad grid.
	
	switch (_difficultyLevel) {
	case 1:
		_mudBallCount = 0;
		_obstacleRows = 0;
		break;
	case 2:
		_mudBallCount = 4;
		_obstacleRows = 0;
		break;
	case 3:
		_mudBallCount = 5;
		_obstacleRows = 2;
		break;
	case 4:
	default:
		_mudBallCount = 6;
		_obstacleRows = 3;
		break;
	}
}

void ZoombiniInteractiveLilly::initGridWithAttributes() {
	// IDA: fleens_initGridWithAttributes (0x427955)
	// Initialize the 12x12 grid with attribute patterns.
	// This is shared logic with the Fleens puzzle.
	
	// Select grid type based on difficulty
	// IDA: word_4AE9CA = 3/4/5 for different difficulty levels
	switch (_difficultyLevel) {
	case 1:
	case 2:
		_gridType = 3;
		break;
	case 3:
		_gridType = 4;
		break;
	case 4:
	default:
		_gridType = 5;
		break;
	}
	
	// Initialize grid positions from REGS data
	// IDA: posArr_4B7C44[i] = (REGS_100_X[i] + 18, REGS_100_Y[i] + 15)
	// The 12 grid cell positions are computed from REGS 100 offsets
	// For now, use placeholder positions - actual REGS parsing would load these
	for (int i = 0; i < 12; i++) {
		_gridCellPositions[i] = Common::Point(50 + i * 45, 100);
	}
	
	// Clear grid attribute arrays
	for (int row = 0; row < 12; row++) {
		for (int col = 0; col < 13; col++) {
			_gridPrimaryAttr[row][col] = 0;
			_gridSecondaryAttr[row][col] = 0;
		}
	}
}

void ZoombiniInteractiveLilly::createZoombiniRunners() {
	// IDA: word_4AE3C2[] — per-zoombini runners at SCRB 10109+i
	// Creates a runner feature for each loaded Zoombini with position tracking.
	
	ZmbStateFile &f = _vm->_state->_f;
	int16 runnerIdx = 0;
	
	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount && runnerIdx < 21; i++) {
		ZmbStateActiveEntry &entry = f._zmbPackActive._entries[i];
		if (!entry._bIsOccupied)
			continue;
		
		// IDA: SCRB 10109+i with DEFER_ANIM | PLAY_ONCE | OVERLAY flags
		// These runners track zoombini movement on the grid
		_zmbRunners[runnerIdx] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10109 + runnerIdx, 4,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);
		
		if (_zmbRunners[runnerIdx]) {
			// IDA: Initially render disabled; position set during grid placement
			_zmbRunners[runnerIdx]->deactivateRender();
			
			// IDA: Special handling for last two zoombinis at difficulty 1
			// They start visible with exit animation at SCRB 10089+i
			if (_difficultyLevel == 1 && 
			    (runnerIdx == _totalZmbCount - 2 || runnerIdx == _totalZmbCount - 1)) {
				// Load exit animation SCRB
				ZmbFeature *exitFeature = loadScrbFeature(
					ZmbResource(ZmbArchiveKind::kPage, 10000), 10089 + runnerIdx, 4,
					ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
					ZmbFeature::FLAG_04000000_OVERLAY);
				if (exitFeature) {
					exitFeature->activateRender();
				}
			}
		}
		
		runnerIdx++;
	}
}

} // End of namespace Mohawk
