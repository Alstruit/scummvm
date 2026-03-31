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
#include "mohawk/zoombini_pages/picker.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

const int16 ZoombiniInteractivePicker::kEmbarkOrder[4] = {11, 12, 6, 7};

ZoombiniInteractivePicker::ZoombiniInteractivePicker(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kPicker), _previewSnoid(vm, 0, ZmbFeature::FLAG_00000001_TYPE_SNOID) {
	_mode = kPickerMode_SelectZoombinis;
	_isFirstVisit = _vm->_state->isFirstLaunch();
	if (_isFirstVisit && 0 < _vm->_state->_r._saveCount1 && _vm->_state->_currentSaveSlot == ZoombiniGameState::kUnsavedNewGame) {
		_mode = kPickerMode_LoadGame;
	}

	ZmbResource matrixPressSoundResId(ZmbArchiveKind::kPage, kResSound1000_PressMatrixButton);
	ZmbResource matrixReleaseSoundResId(ZmbArchiveKind::kPage, kResSound1004_ReleaseMatrixButton);
	for (uint32 i = 0; i < ARRAYSIZE(_pickerMatrixRects); i++) {
		_matrixButtonRectMap[i] = _pickerMatrixRects[i];

		uint16 normalShapeId = 2 * i + 1;
		uint16 pressedShapeId = 2 * i + 2;
		_matrixButtonStateMap[i] = StickyButtonState(matrixPressSoundResId, matrixReleaseSoundResId, i, i + 20, normalShapeId, pressedShapeId);
	}

	ZmbResource generateSoundResId(ZmbArchiveKind::kPage, kResSound1005_PressGenerateButton);
	ZmbResource diceSoundResId(ZmbArchiveKind::kPage, kResSound1006_PressDiceButton);
	_pickerButtonStateMap[kPickerButtons_Generate] = ButtonState(generateSoundResId, kHotspotGenerateButtonNormal, kHotspotGenerateButtonPressed, kShape4200_02_GenerateButtonNormal, kShape4200_03_GenerateButtonPressed);
	_pickerButtonStateMap[kPickerButtons_Generate].setDisabledState(kShape4200_01_GenerateButtonDisabled);
	_pickerButtonStateMap[kPickerButtons_Generate]._isPressDisabled = true; // Disabled until all matrix rows have a selection
	_pickerButtonStateMap[kPickerButtons_Dice] = ButtonState(diceSoundResId, kHotspotDiceButtonNormal, kHotspotDiceButtonPressed, kShape4200_04_DiceButtonNormal, kShape4200_05_DiceButtonPressed);
	_pickerButtonRectMap[kPickerButtons_Generate] = _generateButtonRect;
	_pickerButtonRectMap[kPickerButtons_Dice] = _diceButtonRect;
}

ZoombiniInteractivePicker::~ZoombiniInteractivePicker() {
}

void ZoombiniInteractivePicker::open() {
	if (_vm->isGameVariant(GF_ZMB_TLC))
		openArchive(ZMB_MHK_MUSIC);
	else
		openArchive(ZMB_MHK_MIDIMPC);
	openArchive(ZMB_MHK_PICKER);
}

void ZoombiniInteractivePicker::setBackgroundMusic() {
	// Play background music (1.x: MIDI, 2.0: WAV).
	if (_vm->isGameVariant(GF_ZMB_TLC)) // 1.x: MIDI from MIDIMPC.MHK
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, kResSound30001_Isle), Audio::Mixer::kMusicSoundType);
	else // 2.0: WAV from MUSIC.MHK
		_vm->_midi->playZmbMidi(ZmbResource(ZmbArchiveKind::kPage, kResMidi30001_Isle));
}

void ZoombiniInteractivePicker::setBackgroundBitmap() {
	_vm->_gfx->setPalette(kResBackground4000);
	_vm->_gfx->drawBackground(kResBackground4000);
}

void ZoombiniInteractivePicker::loadFeatures() {
	_vm->_gfx->preloadImage(kResBitmapShapes4400_PickerMatrix);    // to shape slot 0
	_vm->_gfx->preloadImage(kResBitmapShapes4200_Buttons);         // to shape slot 2
	_vm->_gfx->preloadImage(kResBitmapShapes4300_ZoombiniPreview); // to shape slot 1
	_vm->_gfx->preloadImage(kResBitmapShapes4100_BackObjects);     // main shape

	// Load NODE 1000: the path snoids walk when entering the corral.
	// waypoints[0] = (148, 215) is the IDA-confirmed entry point (wSrcX=148, wSrcY=215).
	loadNODE(ZmbArchiveKind::kPage, 1000);

	// Update Game State
	ZmbStateFile &f = _vm->_state->_f;
	// Copy Isle pack -> Active pack, then clear the Isle pack count
	// (mirrors IDA: qmemcpy(zmbPackActive, zmbPackBC0); p_zmbPackActive[1]=p_zmbPackBC0[1]; zmbPackBC0.wCount=0)
	f._zmbPackIsle.copyTo(f._zmbPackActive);
	f._wZmbPackActiveVal = f._wZmbPackIsleVal;
	f._zmbPackIsle._wPackZmbCount = 0;

	// Load Zoombini snoids from the active pack (Isle zoombinis sitting on seats)
	// IDA: handleZoombiniAnimation_maybe_4528A6(0); (only occupied entries)
	loadZoombinisFromPack(f._zmbPackActive);

	if (!_vm->_state->isLessActionEnabled())
		f._wPickerCaveBlinkState = 1;

	if (f._currentRoute == 1) {
		uint16 leavedZmbCount = f._zmbStoredBC1Count + f._zmbStoredBC2Count + f._zmbStoredTownCount;
		uint16 remaingZmbCount = 625 - leavedZmbCount - _snoidMap.size();
		if (0 < remaingZmbCount && _snoidMap.size() < 625) {
			uint16 soundRand = _vm->_rnd->getRandomNumber(19);
			if (soundRand == 0) {
				_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound20043_PickerAfterVideoVoice), Audio::Mixer::kSFXSoundType);
			} else if (soundRand == 9) {
				_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound20044_PickerAfterVideoVoice), Audio::Mixer::kSFXSoundType);
			}
		}
	} else {
		_vm->_state->getDifficultyIdFromPageFlag(f._pageFlagIsle);
		// IDA: only on first visit this session (chIsFirstVisit_4A71B8 == 1)
		if (f._zmbGeneratedCount < 625 && _isFirstVisit) {
			_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound20042_PickerAfterVideoVoice), Audio::Mixer::kSFXSoundType);
		}
	}

	// Background Animation: Stars
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4100_BackObjects), kResScrb4101_Star, 10,
					ZmbFeature::FLAG_00008000_LOOP_ANIM);
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4100_BackObjects), kResScrb4102_Star, 11,
					ZmbFeature::FLAG_00008000_LOOP_ANIM);
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4100_BackObjects), kResScrb4103_Star, 12,
					ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// Background Animation: Waves and Boat (Disabled in less action mode)
	if (!_vm->_state->isLessActionEnabled()) {
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4100_BackObjects), kResScrb4104_Waves, 7,
						ZmbFeature::FLAG_08000000_REGION_TRACK | ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE);
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4100_BackObjects), kResScrb4105_Boat, 9,
						ZmbFeature::FLAG_08000000_REGION_TRACK | ZmbFeature::FLAG_00008000_LOOP_ANIM);
	}

	// Background Objects
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4100_BackObjects), kResScrb4100_BackObjects, 0,
					ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_20000000_ZSORT_BOTTOM | ZmbFeature::FLAG_40000000_ZSORT_LEFT);

	// Cave Mark on Hover with Zoombinis
	// - Appears only when a user is holding a zoombini
	// IDA: pCaveMarkFeatureCore_4A2D90 point (172, 226)
	_caveMarkFeature = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4100_BackObjects), kResScrb4110_CaveMark, 6,
					ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00002000_DRAW_ON_REG);

	// [*] Rocks near Cave Entrance
	// Part of Rocks - Bottom from Entrance
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4100_BackObjects), kResScrb4106_RockShape, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES);
	// Part of Rocks - Bottom-Left from Entrance
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4100_BackObjects), kResScrb4107_RockShape, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES);
	// Part of Rocks - Left from Entrance
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4100_BackObjects), kResScrb4108_RockShape, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES);
	// Part of Rocks - Bottom-Right from Entrance
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4100_BackObjects), kResScrb4109_RockShape, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES);

	setGoButton(_goButtonRect, kShape4200_08_GoButtonDisabled, kShape4200_09_GoButtonNormal, kShape4200_10_GoButtonPressed);
	setMapButton(_mapButtonRect, kShape4200_11_MapButtonNormal, kShape4200_12_MapButtonPressed);
	setHelpButton(_helpButtonRect);

	// [*] Virtual Feature (tBMP 4200) - Go, Map Buttons
	ZoombiniInteractive::loadGoMapButtonsFeature(kResBitmapShapes4200_Buttons);

	// [*] Virtual Features (tBMP c:0001) - Help Button
	ZoombiniInteractive::loadHelpButtonFeature();

	{ // [*] Virtual Feature (tBMP 4400) - Picker Matrix
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractivePicker::pickerMatrix_onPreRenderShape));
		hooks.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractivePicker::pickerMatrix_onPostRender));
		hooks.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractivePicker::pickerMatrix_onLButtonDown));

		Common::Array<ZmbHotspot> hotspots;
		// Normal hotspots (indices 0..19)
		for (uint32 i = 0; i < ARRAYSIZE(_pickerMatrixRects); i++) {
			uint16 normalShapeId = 2 * i + 1;
			hotspots.push_back(ZmbHotspot(i, normalShapeId, 0, _pickerMatrixRects[i]));
		}
		// Pressed hotspots (indices 20..39)
		for (uint32 i = 0; i < ARRAYSIZE(_pickerMatrixRects); i++) {
			uint16 pressedShapeId = 2 * i + 2;
			hotspots.push_back(ZmbHotspot(i + 20, pressedShapeId, 0, _pickerMatrixRects[i]));
		}

		loadVirtualFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4400_PickerMatrix), kVirtualFeaturePickerMatrix,
						   hotspots, 0,
						   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooks);
	}

	{ // [*] Virtual Feature (tBMP 4200) - Picker Buttons
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractivePicker::pickerButtons_onPreRenderShape));
		hooks.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractivePicker::pickerButtons_onPostRender));
		hooks.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractivePicker::pickerButtons_onLButtonDown));
		hooks.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniInteractivePicker::pickerButtons_onKeyDown));
		hooks.setKeyUpFunc(reinterpret_cast<ZmbFeature::OnKeyUpFunc>(&ZoombiniInteractivePicker::pickerButtons_onKeyUp));

		Common::Array<ZmbHotspot> hotspots;
		hotspots.push_back(ZmbHotspot(kHotspotGenerateButtonNormal, kShape4200_02_GenerateButtonNormal, 0, _generateButtonRect));
		hotspots.push_back(ZmbHotspot(kHotspotDiceButtonNormal, kShape4200_04_DiceButtonNormal, 0, _diceButtonRect));
		hotspots.push_back(ZmbHotspot(kHotspotGenerateButtonPressed, kShape4200_03_GenerateButtonPressed, 0, _generateButtonRect));
		hotspots.push_back(ZmbHotspot(kHotspotDiceButtonPressed, kShape4200_05_DiceButtonPressed, 0, _diceButtonRect));
		hotspots.push_back(ZmbHotspot(kHotspotNameBox, kShape4200_13_NameBox, 0, _nameBoxRect));

		loadVirtualFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4200_Buttons), kVirtualFeaturePickerButtons,
						   hotspots, 0,
						   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooks);
	}

	{ // [*] Virtual Feature (tBMP 4300) - Zoombini Preview
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractivePicker::zoombiniPreview_onPreRenderShape));

		// TODO: Use proper ZmbFeature for Zoombinis later
		const int16 previewTraitOffsetX = _previewZoombiniRect.left + 39;
		const int16 previewTraitOffsetY = _previewZoombiniRect.top + 31;
		Common::Array<ZmbHotspot> hotspots;
		hotspots.push_back(ZmbHotspot(kHotspotPreviewBody, kShape4300_01_PreviewBody, 0, previewTraitOffsetX, previewTraitOffsetY));
		hotspots.push_back(ZmbHotspot(kHotspotPreviewHair, ZmbHotspot::kShapeNone, 0, previewTraitOffsetX, previewTraitOffsetY));
		hotspots.push_back(ZmbHotspot(kHotspotPreviewEye, ZmbHotspot::kShapeNone, 0, previewTraitOffsetX, previewTraitOffsetY));
		hotspots.push_back(ZmbHotspot(kHotspotPreviewNose, ZmbHotspot::kShapeNone, 0, previewTraitOffsetX, previewTraitOffsetY));
		hotspots.push_back(ZmbHotspot(kHotspotPreviewFoot, ZmbHotspot::kShapeNone, 0, previewTraitOffsetX, previewTraitOffsetY));

		loadVirtualFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShapes4300_ZoombiniPreview), kVirtualFeatureZoombiniPreview,
						   hotspots, 0,
						   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooks);
	}

	// Generate initial preview name.
	// IDA: init_pickerPuzzleRunner_439674 calls getNextZoombiniNameStr_453E12.
	generateZoombiniName();

	// [*] Virtual Feature - open loadDialog if required
	if (_mode == kPickerMode_LoadGame) {
		ZmbFeature::EventHooks hooks;
		hooks.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniInteractivePicker::oneTimeLoadDialog_onRenderShape));

		loadVirtualFeature(kVirtualFeatureLoadDialog, 0,
						   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooks);
	}
}

void ZoombiniInteractivePicker::pickerMatrix_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	genericStickyButton_selectShapes(feature, hotspots, _matrixButtonStateMap);
}

void ZoombiniInteractivePicker::pickerMatrix_onPostRender(ZmbFeature *feature) {
}

void ZoombiniInteractivePicker::pickerMatrix_onButtonAction(ZmbFeature *feature, uint32 bsIdx, StickyButtonState &bs) {
	uint32 row = bsIdx / kMatrixColumns;
	uint32 column = bsIdx % kMatrixColumns;

	if (bs.isStuck()) {
		// Enforce mutual exclusion: in each row only one button may be stuck at a time
		switch (row) {
		case 0: // Head
			_previewSnoid._trait._head = column + 1;
			break;
		case 1: // Eye
			_previewSnoid._trait._eye = column + 1;
			break;
		case 2: // Nose
			_previewSnoid._trait._nose = column + 1;
			break;
		case 3: // Foot
			_previewSnoid._trait._foot = column + 1;
			break;
		default:
			error("Invalid ButtonState index %u", bsIdx);
			break;
		}

		for (uint32 colIdx = 0; colIdx < kMatrixColumns; colIdx++) {
			uint32 sibIdx = row * kMatrixColumns + colIdx;
			if (sibIdx != bsIdx)
				_matrixButtonStateMap[sibIdx].reset();
		}
	} else {
		// Clear the trait if the button is unstuck
		switch (row) {
		case 0: // Head
			_previewSnoid._trait._head = ZmbTrait::TRAIT_NONE;
			break;
		case 1: // Eye
			_previewSnoid._trait._eye = ZmbTrait::TRAIT_NONE;
			break;
		case 2: // Nose
			_previewSnoid._trait._nose = ZmbTrait::TRAIT_NONE;
			break;
		case 3: // Foot
			_previewSnoid._trait._foot = ZmbTrait::TRAIT_NONE;
			break;
		default:
			error("Invalid ButtonState index %u", bsIdx);
			break;
		}
	}

	// Enable Generate button only when every row has a selection.
	// IDA: trait matrix clicks never call getNextZoombiniNameStr_453E12.
	// The name stays unchanged when the user manually selects traits.
	_pickerButtonStateMap[kPickerButtons_Generate]._isPressDisabled = !isZoombiniTraitGeneratable(_previewSnoid._trait);
}

ZmbEventHandleResult ZoombiniInteractivePicker::pickerMatrix_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericStickyButton_onLButtonDown(feature, absPos, _matrixButtonStateMap, _matrixButtonRectMap, reinterpret_cast<OnStickyButtonActionFunc>(&ZoombiniInteractivePicker::pickerMatrix_onButtonAction));
}

void ZoombiniInteractivePicker::pickerButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// IDA: puzzlePicker_checkCanMoreZmbGenerated_43B359 logic:
	// wBoolCanMoreZmbGenerated = (loadedCount >= 16 || totalGenerated >= 625)
	const bool isFull = (static_cast<int16>(_snoidMap.size()) >= 16) || (_vm->_state->_f._zmbGeneratedCount >= 625);
	_pickerButtonStateMap[kPickerButtons_Generate]._isPressDisabled = isFull || !isZoombiniTraitGeneratable(_previewSnoid._trait);
	setGoButtonsEnabled(isFull);

	genericButton_selectShapes(feature, hotspots, _pickerButtonStateMap);
}

void ZoombiniInteractivePicker::pickerButtons_onPostRender(ZmbFeature *feature) {
	updatePendingGoTransition();

	// IDA 0x43A816: Name text is only drawn when wBoolUpdateDrawPreviewName is
	// true, i.e. all 4 traits are selected and the combination is generatable.
	if (isZoombiniTraitGeneratable(_previewSnoid._trait)) {
		ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

		Common::Rect nameTextRect = _nameBoxRect;
		nameTextRect.left += 4;
		nameTextRect.top += 4;

		ZoombiniGraphics::TextConf tc;
		tc._hAlign = Graphics::TextAlign::kTextAlignCenter;
		tc._vAlign = Graphics::TextAlign::kTextAlignCenter;
		_vm->_gfx->drawText(screenKind, _previewSnoid._name, nameTextRect, tc);
	}

	genericButton_action(feature, _pickerButtonStateMap, reinterpret_cast<OnButtonActionFunc>(&ZoombiniInteractivePicker::pickerButtons_onButtonAction));
}

void ZoombiniInteractivePicker::updatePendingGoTransition() {
	if (!_pendingGoTransition)
		return;

	// Wait for departure sound to finish
	if (_pendingGoTransitionHasSoundHandle && _vm->_system->getMixer()->isSoundHandleActive(_pendingGoTransitionSoundHandle))
		return;

	// Wait for all embarking snoids to finish their walk animation
	// IDA: puzzle_pendingTransitionTarget=7 polling
	if (!areAllEmbarkersDone())
		return;

	_pendingGoTransition = false;
	_pendingGoTransitionHasSoundHandle = false;
	_embarkingSnoids.clear();
	_vm->setNextPage(ZoombiniPageType::kXfer);
	close();
}

ZmbSnoid *ZoombiniInteractivePicker::findSnoidAtSeat(int16 seatIdx) {
	if (seatIdx < 0 || seatIdx >= 16)
		return nullptr;
	return _seatToSnoid[seatIdx];
}

bool ZoombiniInteractivePicker::areAllEmbarkersDone() const {
	// Check if all embarking snoids have finished (either idle or walked off-screen right edge)
	// IDA: departure polling similar to picker_tryTransition at 0x439E8F
	for (ZmbSnoid *snoid : _embarkingSnoids) {
		if (!snoid)
			continue;

		// Check if snoid has reached the right edge (off-screen)
		const Common::Point pos = snoid->getPointLoc();
		if (pos.x >= 640)
			continue; // Snoid walked off screen - done

		// Check if snoid has returned to idle state (reached destination)
		if (snoid->getAnimState() == kSnoidAnimIdle)
			continue; // Done animating

		// Snoid is still walking
		return false;
	}
	return true;
}

void ZoombiniInteractivePicker::pickerButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs) {
	if (bs._isPressDisabled)
		return;

	switch (bsIdx) {
	case kPickerButtons_Generate: {
		// IDA: puzzlePicker_buttonClickHandler_439EC2 case 1
		ZmbStateFile &f = _vm->_state->_f;
		const int16 zmbOnScreen = static_cast<int16>(_snoidMap.size());
		if (zmbOnScreen >= 16 || f._zmbGeneratedCount >= 625)
			break;
		if (!isZoombiniTraitGeneratable(_previewSnoid._trait))
			break;

		// Increment twinGenStatus for the current preview trait combination
		// IDA: puzzlePicker_doSomething_43AE9F(1)
		const int16 snoidTraitId = _previewSnoid._trait.snoidId();
		if (f._twinGenStatus[snoidTraitId] < 2)
			++f._twinGenStatus[snoidTraitId];

		// Add new entry to the active pack for later transfer to basecamp
		const int16 packIdx = f._zmbPackActive._wPackZmbCount;
		if (packIdx < 16) {
			ZmbStateActiveEntry &entry = f._zmbPackActive._entries[packIdx];
			entry._traits = _previewSnoid._trait;
			entry._bIsOccupied = 1;
			Common::String nameStr = _vm->_text->fromU32String(_previewSnoid._name);
			memset(entry._name, 0, sizeof(entry._name));
			memcpy(entry._name, nameStr.c_str(), MIN<uint32>(nameStr.size(), sizeof(entry._name)));
			++f._zmbPackActive._wPackZmbCount;
		}

		// Create a new snoid walking from the NODE 1000 entry point to the next seat.
		// IDA: setZoombiniNextAnimation_4527D7(seatPos.y, seatPos.x, wSrcY=215, wSrcX=148, 0, preview)
		// The source (148, 215) is waypoints[0] of NODE 1000; use kSnoidAnimDepart so the
		// snoid walks in rather than teleporting directly to the seat.
		Common::Point spawnPos(148, 215); // IDA-confirmed NODE 1000 entry (wSrcX=148, wSrcY=215)
		{
			auto nodeIt = _nodeMap.find(static_cast<uint16>(1000));
			if (nodeIt != _nodeMap.end() && !nodeIt->_value->_waypoints.empty())
				spawnPos = nodeIt->_value->_waypoints[0];
		}
		const Common::Point seatPos = _zoombiniSeatPoints[zmbOnScreen];
		const uint16 newSnoidId = kSnoidPackBase + _nextPackSnoidId++;
		ZmbSnoid *snoid = loadSnoidFromPack(newSnoidId, spawnPos,
											ZmbFeature::FLAG_00000001_TYPE_SNOID);
		if (snoid) {
			snoid->_trait = _previewSnoid._trait;
			snoid->_name = _previewSnoid._name;
			snoid->setupIdleHotspots();
			snoid->setFacingLeft(spawnPos.x > seatPos.x);
			snoid->setAnimTargetPos(seatPos);
			snoid->setAnimState(kSnoidAnimDepart, nullptr);
			// Track seat-to-snoid mapping for embark animation
			_seatToSnoid[zmbOnScreen] = snoid;
		}

		// Increment total generated count
		// IDA: ++pGameState_4A476C->wGeneratedZmbCount
		++f._zmbGeneratedCount;

		// Immediately enable/disable the Go button based on current state.
		// This runs before goMapButtons_preRenderShape (GoMapButtons id=90 > PickerButtons id=3),
		// so the button visual updates in the same frame the 16th snoid is added.
		{
			const bool isFull = (static_cast<int16>(_snoidMap.size()) >= 16) || (f._zmbGeneratedCount >= 625);
			setGoButtonsEnabled(isFull);
		}

		// Generate a new name for the next preview snoid
		// IDA: getNextZoombiniNameStr_453E12(10, pickerPreviewZmbCore259_4B0B54.pcZmbName_noTerm)
		generateZoombiniName();

		break;
	}
	case kPickerButtons_Dice: {
		// SHIFT held changes the dice button to an "arrow" variant that force-generates all 16 zoombinis
		randomizeTraitSelection();
		{
			const ZmbStateFile &f = _vm->_state->_f;
			const bool isFull = (static_cast<int16>(_snoidMap.size()) >= 16) || (f._zmbGeneratedCount >= 625);
			setGoButtonsEnabled(isFull);
		}
		break;
	}
	default:
		error("ZoombiniInteractivePicker::pickerButtons_onPostAnimation: Invalid ButtonState index %u", bsIdx);
		break;
	}
}

ZmbEventHandleResult ZoombiniInteractivePicker::pickerButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericButton_onLButtonDown(feature, absPos, _pickerButtonStateMap, _pickerButtonRectMap);
}

ZmbEventHandleResult ZoombiniInteractivePicker::pickerButtons_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	// SHIFT held changes the dice button to an "arrow" variant that force-generates all 16 zoombinis
	if (kbd.hasFlags(Common::KBD_SHIFT)) {
		_randomizeAll = true;

		uint32 diceButtonNormalShapeIdx = kShape4200_06_DiceArrowButtonNormal;
		uint32 diceButtonPressedShapeIdx = kShape4200_07_DiceArrowButtonPressed;

		feature->getHotspotGroup(0)->getHotspot(kHotspotDiceButtonNormal)._shapeIdx = diceButtonNormalShapeIdx;
		feature->getHotspotGroup(0)->getHotspot(kHotspotDiceButtonPressed)._shapeIdx = diceButtonPressedShapeIdx;
		_pickerButtonStateMap[kPickerButtons_Dice]._shapeNormalIdx = diceButtonNormalShapeIdx;
		_pickerButtonStateMap[kPickerButtons_Dice]._shapePressedIdx = diceButtonPressedShapeIdx;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniInteractivePicker::pickerButtons_onKeyUp(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	if (!kbd.hasFlags(Common::KBD_SHIFT)) {
		_randomizeAll = false;

		uint32 diceButtonNormalShapeIdx = kShape4200_04_DiceButtonNormal;
		uint32 diceButtonPressedShapeIdx = kShape4200_05_DiceButtonPressed;

		feature->getHotspotGroup(0)->getHotspot(kHotspotDiceButtonNormal)._shapeIdx = diceButtonNormalShapeIdx;
		feature->getHotspotGroup(0)->getHotspot(kHotspotDiceButtonPressed)._shapeIdx = diceButtonPressedShapeIdx;
		_pickerButtonStateMap[kPickerButtons_Dice]._shapeNormalIdx = diceButtonNormalShapeIdx;
		_pickerButtonStateMap[kPickerButtons_Dice]._shapePressedIdx = diceButtonPressedShapeIdx;
	}

	return ZmbEventHandleResult::kPassthrough;
}

void ZoombiniInteractivePicker::zoombiniPreview_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// Body is always visible
	ZmbHotspot &hsBody = hotspots[kHotspotPreviewBody];
	hsBody._shapeIdx = kShape4300_01_PreviewBody;
	hsBody._x -= _previewTraitOffsets[0].x;
	hsBody._y -= _previewTraitOffsets[0].y;

	// For each trait row, find the selected column and map it to the corresponding preview shape.
	// Rows are laid out regularly: rowStart = traitIdx * kMatrixColumns,
	const uint32 numTraitRows = ARRAYSIZE(_pickerMatrixRects) / kMatrixColumns;
	for (uint32 traitIdx = 0; traitIdx < numTraitRows; traitIdx++) {
		uint32 rowStart = traitIdx * kMatrixColumns;
		uint32 hsId = kHotspotPreviewHair + traitIdx;
		uint16 rowShapeIdxBase = kShape4300_02_PreviewHair1 + traitIdx * kMatrixColumns;

		ZmbHotspot &hs = hotspots[hsId];
		uint16 shapeIdx = ZmbHotspot::kShapeNone;
		for (uint32 col = 0; col < kMatrixColumns; col++) {
			auto it = _matrixButtonStateMap.find(rowStart + col);
			if (it != _matrixButtonStateMap.end() && it->second.isStuck()) {
				shapeIdx = rowShapeIdxBase + col;
				break;
			}
		}
		if (0 < shapeIdx) {
			hs._shapeIdx = shapeIdx;
			hs._x -= _previewTraitOffsets[shapeIdx - 1].x;
			hs._y -= _previewTraitOffsets[shapeIdx - 1].y;
		}
	}
}

ZmbRenderResult ZoombiniInteractivePicker::oneTimeLoadDialog_onRenderShape(ZmbFeature *feature) {
	_vm->_gfx->flushScreens(); // Ensure screen is up-to-date before opening a first-screen loadDialog
	ZoombiniDialogResult dialogResult = _vm->openLoadDialog(true);
	if (dialogResult == ZoombiniDialogResult::kYes)
		close();

	// This function must be called only once
	feature->scheduleClose();

	return ZmbRenderResult::kRendered;
}

// ---------------------------------------------------------------------------
// Page-level mouse handlers — Snoid drag/drop
// IDA: puzzlePicker_buttonClickHandler_439EC2 case 8 (DragZoombini)
// ---------------------------------------------------------------------------

ZmbEventHandleResult ZoombiniInteractivePicker::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// IDA: puzzlePicker_buttonClickHandler_439EC2 calls picker_tryTransition() (0x439E8F) first.
	// picker_tryTransition immediately completes the pending departure if puzzle_pendingTransitionTarget is set.
	if (_pendingGoTransition) {
		_pendingGoTransition = false;
		_pendingGoTransitionHasSoundHandle = false;
		_embarkingSnoids.clear();
		_vm->setNextPage(ZoombiniPageType::kXfer);
		close();
		return ZmbEventHandleResult::kConsumed;
	}

	// In sticky mouse mode, a second click ends the drag
	if (isDragging() && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	// Let the base class handle button/feature clicks first.
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Don't start a new drag while already dragging
	if (isDragging())
		return ZmbEventHandleResult::kPassthrough;

	// Guard: don't drag while any snoid is walking in (depart or path state)
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		SnoidAnimState st = it->second->getAnimState();
		if (st == kSnoidAnimDepart || st == kSnoidAnimPath)
			return ZmbEventHandleResult::kPassthrough;
	}

	// Find snoid under cursor
	ZmbSnoid *snoid = findSnoidAtPoint(absPos);
	if (!snoid)
		return ZmbEventHandleResult::kPassthrough;

	// IDA case 8: v14 = animState; v13 = (!v14 || v14==6 || v14==4)
	// Only idle (0), arrive (4), or fidget (6) are draggable.
	SnoidAnimState state = snoid->getAnimState();
	if (state != kSnoidAnimIdle && state != kSnoidAnimArrive && state != kSnoidAnimFidget)
		return ZmbEventHandleResult::kPassthrough;

	startSnoidDrag(snoid, absPos);
	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniInteractivePicker::updateCaveMarkHighlight() {
	// IDA: beginDragFeatureRunner_45360F — DRAW_ON_REG highlight logic.
	// Check if the cave mark reg point is within clickZoneRadius of the dragged snoid.
	if (!isDragging() || !_caveMarkFeature) {
		if (_caveMarkHighlighted) {
			_caveMarkFeature->deactivateRender();
			_caveMarkFeature->deactivateAnimate();
			_caveMarkHighlighted = false;
		}
		return;
	}

	Common::Point snoidPos = _draggedSnoid->getPointLoc();
	Common::Rect dropZone(snoidPos.x - kPickerClickZoneRadius, snoidPos.y - kPickerClickZoneRadius,
						  snoidPos.x + kPickerClickZoneRadius, snoidPos.y + kPickerClickZoneRadius);

	if (dropZone.contains(_caveMarkRegPoint)) {
		if (!_caveMarkHighlighted) {
			_caveMarkFeature->activateRender();
			_caveMarkFeature->activateAnimate();
			_caveMarkHighlighted = true;
		}
	} else {
		if (_caveMarkHighlighted) {
			_caveMarkFeature->deactivateRender();
			_caveMarkFeature->deactivateAnimate();
			_caveMarkHighlighted = false;
		}
	}
}

ZmbEventHandleResult ZoombiniInteractivePicker::onMouseMove(const Common::Point &absPos, const Common::Point &relPos) {
	ZmbEventHandleResult result = ZoombiniInteractive::onMouseMove(absPos, relPos);
	updateCaveMarkHighlight();
	return result;
}

void ZoombiniInteractivePicker::endDrag(const Common::Point &dropPos) {
	// Deactivate cave mark highlight when drag ends
	if (_caveMarkHighlighted && _caveMarkFeature) {
		_caveMarkFeature->deactivateRender();
		_caveMarkFeature->deactivateAnimate();
		_caveMarkHighlighted = false;
	}

	ZmbSnoid *snoid = finishSnoidDrag();
	Common::Point snoidPos = snoid->getPointLoc();

	if (_caveDropRect.contains(snoidPos)) {
		// --- Dropped on cave entrance: remove the snoid ---
		// IDA: getDropTargetResult_453571 → true → picker_removeDraggedZmb

		// Copy traits back to preview panel
		_previewSnoid._trait = snoid->_trait;
		_previewSnoid._name = snoid->_name;

		// Update matrix selection buttons to match the restored preview traits
		for (uint32 row = 0; row < 4; row++) {
			uint8 traitVal = 0;
			switch (row) {
			case 0: traitVal = _previewSnoid._trait._head; break;
			case 1: traitVal = _previewSnoid._trait._eye; break;
			case 2: traitVal = _previewSnoid._trait._nose; break;
			case 3: traitVal = _previewSnoid._trait._foot; break;
			}
			for (uint32 col = 0; col < kMatrixColumns; col++) {
				_matrixButtonStateMap[row * kMatrixColumns + col]._isStuck = (traitVal == col + 1);
			}
		}

		// Decrement twin status
		// IDA: pGameState->dTwinGenStatusArr_A7F0[snoidTraitId] -= 1
		ZmbStateFile &f = _vm->_state->_f;
		int16 snoidTraitId = snoid->_trait.snoidId();
		if (f._twinGenStatus[snoidTraitId] > 0)
			f._twinGenStatus[snoidTraitId]--;

		// Decrement total generated count
		if (f._zmbGeneratedCount > 0)
			f._zmbGeneratedCount--;

		// Remove snoid from screen
		uint16 removedId = snoid->getId();
		unloadSnoid(removedId);

		// Repack remaining snoids to sequential seat positions
		repackSeatPositions();

		// Play remove SFX
		// IDA: scrb_enqueueSoundResource(0, SND_01007_PICKER_GENERATE_BUTTON_SFX)
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, kResSound1007_RemoveZoombini),
								  Audio::Mixer::kSFXSoundType);

		// Update Go button: enabled only when board is full
		const bool isFull = (static_cast<int16>(_snoidMap.size()) >= 16) || (f._zmbGeneratedCount >= 625);
		setGoButtonsEnabled(isFull);

		// Update Generate button: re-enable since we have an open seat
		_pickerButtonStateMap[kPickerButtons_Generate]._isPressDisabled =
			!isZoombiniTraitGeneratable(_previewSnoid._trait);
	} else {
		// --- Not on cave: return snoid to original seat position ---
		snoid->setPointLoc(_dragOrigPos);
		snoid->setAnimState(kSnoidAnimIdle);
		snoid->setupIdleHotspots();
	}
}

ZmbEventHandleResult ZoombiniInteractivePicker::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!isDragging())
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);

	// In sticky mouse mode, button-up does NOT end drag (click again to drop)
	if (_vm->_state->getEnableStickyMouse())
		return ZmbEventHandleResult::kConsumed;

	endDrag(absPos);

	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniInteractivePicker::repackSeatPositions() {
	// IDA: picker_getNextSeatOrRepack(0, 0)
	// Reassign all remaining snoids to sequential seat positions.
	// Also rebuild the active pack entries and seat-to-snoid mapping.
	ZmbStateFile &f = _vm->_state->_f;

	// Clear old pack entries and seat mapping
	f._zmbPackActive._wPackZmbCount = 0;
	memset(_seatToSnoid, 0, sizeof(_seatToSnoid));

	uint16 seatIdx = 0;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *s = it->second;
		if (!s->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			continue;
		if (seatIdx >= 16)
			break;

		// Move snoid to the next sequential seat
		s->setPointLoc(_zoombiniSeatPoints[seatIdx]);
		s->setAnimState(kSnoidAnimIdle);
		s->setupIdleHotspots();
		_seatToSnoid[seatIdx] = s;

		// Rebuild pack entry
		ZmbStateActiveEntry &entry = f._zmbPackActive._entries[seatIdx];
		entry._traits = s->_trait;
		entry._bIsOccupied = 1;
		Common::String nameStr = _vm->_text->fromU32String(s->_name);
		memset(entry._name, 0, sizeof(entry._name));
		memcpy(entry._name, nameStr.c_str(), MIN<uint32>(nameStr.size(), sizeof(entry._name)));

		seatIdx++;
	}
	f._zmbPackActive._wPackZmbCount = seatIdx;
}

void ZoombiniInteractivePicker::onGoButtonActivated() {
	if (_pendingGoTransition)
		return;

	// IDA: puzzlePicker_buttonClickHandler_439EC2 case 6
	// Play departure sound 996
	Audio::SoundHandle *departSfxHandle = _vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound0996_DepartSFX),
																Audio::Mixer::kSFXSoundType);

	ZmbStateFile &f = _vm->_state->_f;
	// IDA: picker_cleanup ELSE branch (normal departure, puzzle_nextPuzzleId != 1):
	// Only zeroes BC0.wPackZmbCount. Active pack stays intact for xfer and bridge.
	f._zmbPackIsle._wPackZmbCount = 0;

	_vm->_xferSrcSiPage = ZMB_SI_PICKER_01;
	setGoButtonsEnabled(false);

	// Clear embarking snoid list for new departure
	_embarkingSnoids.clear();

	// IDA: Only ONE snoid embarks! The loop condition is `ui_bDragLockActive < 1`
	// which means only the first idle snoid from priority seats [11, 12, 6, 7] animates.
	// This snoid walks to the embark destination (544, 264) on the boat.
	// IDA: dword_4A2D9A = [11, 12], dword_4A2D9E = [6, 7]
	for (uint16 i = 0; i < 4; i++) {
		int16 seatIdx = kEmbarkOrder[i];
		if (seatIdx < 0 || seatIdx >= 16)
			continue;

		ZmbSnoid *snoid = _seatToSnoid[seatIdx];
		if (!snoid)
			continue;

		// Only take snoids that are idle (not still walking in, etc.)
		if (snoid->getAnimState() != kSnoidAnimIdle)
			continue;

		// Set destination and begin walking animation
		// IDA: pZmb->animDestPos = (544, 264); animateZoombini(0, 7, pZmb);
		snoid->setAnimTargetPos(_embarkDestination);
		snoid->setAnimState(kSnoidAnimDepart, nullptr);

		_embarkingSnoids.push_back(snoid);
		// IDA: ui_bDragLockActive < 1 means only ONE snoid - break after first
		break;
	}

	// Keep Picker visible until embark animation and SFX 996 finish, then switch to XFER.
	_pendingGoTransition = true;
	_pendingGoTransitionHasSoundHandle = departSfxHandle != nullptr;
	if (departSfxHandle)
		_pendingGoTransitionSoundHandle = *departSfxHandle;

	updatePendingGoTransition();
}

void ZoombiniInteractivePicker::randomizeTraitSelection() {
	// IDA: puzzlePicker_diceRandomZoombini_43ABE2(wBool)
	// wBool = pickerRunner_4B0C5C.wBoolUpdateDrawPreviewName (whether current
	// combo is already generatable). When wBool=false, only EMPTY trait slots
	// are randomized — user-selected traits are preserved. Name is only
	// generated when wBool becomes true.
	bool randomizeAll = isZoombiniTraitGeneratable(_previewSnoid._trait);
	bool isGeneratable = false;
	int attempt = 0;
	while (!isGeneratable && attempt < 64) {
		attempt++;
		if (attempt < 64) {
			uint8 *traits[4] = {
				&_previewSnoid._trait._head,
				&_previewSnoid._trait._eye,
				&_previewSnoid._trait._nose,
				&_previewSnoid._trait._foot,
			};
			for (int i = 0; i < 4; i++) {
				// IDA 0x43ACCE: only randomize if empty OR wBool is true
				if (!(*traits[i]) || randomizeAll)
					*traits[i] = static_cast<uint8>(_vm->_rnd->getRandomNumber(1, 5));
			}
		} else { // Fallback: exhaustive scan — find the last valid combination
			randomizeAll = true; // IDA 0x43AC12: force all traits randomized
			for (int hi = 0; hi < 5; hi++) {
				for (int ei = 0; ei < 5; ei++) {
					for (int ni = 0; ni < 5; ni++) {
						for (int fi = 0; fi < 5; fi++) {
							int16 snoidId = static_cast<int16>(125 * hi + 25 * ei + 5 * ni + fi);
							if (_vm->_state->_f._twinGenStatus[snoidId] < 2) {
								_previewSnoid._trait._head = static_cast<uint8>(hi + 1);
								_previewSnoid._trait._eye = static_cast<uint8>(ei + 1);
								_previewSnoid._trait._nose = static_cast<uint8>(ni + 1);
								_previewSnoid._trait._foot = static_cast<uint8>(fi + 1);
							}
						}
					}
				}
			}
		}

		isGeneratable = isZoombiniTraitGeneratable(_previewSnoid._trait);
		// IDA 0x43AD11: if not yet generatable, force wBool=true on next retry
		if (!isGeneratable)
			randomizeAll = true;
	}

	// IDA 0x43AD31: only generate a new name when wBool is true.
	// When the user had a partial selection (wBool started false) and the
	// first fill-in succeeded, the name stays unchanged.
	if (randomizeAll)
		generateZoombiniName();

	// Sync matrix sticky-button states to reflect the rolled traits
	const uint8 rolledTraits[4] = {
		_previewSnoid._trait._head,
		_previewSnoid._trait._eye,
		_previewSnoid._trait._nose,
		_previewSnoid._trait._foot,
	};
	for (uint32 row = 0; row < 4; row++) {
		for (uint32 col = 0; col < kMatrixColumns; col++) {
			uint32 idx = row * kMatrixColumns + col;
			_matrixButtonStateMap[idx]._isStuck = (rolledTraits[row] == col + 1);
		}
	}

	if (_randomizeAll) {
		ZmbStateFile &f = _vm->_state->_f;
		Common::Point spawnPos(148, 215);
		{
			auto nodeIt = _nodeMap.find(static_cast<uint16>(1000));
			if (nodeIt != _nodeMap.end() && !nodeIt->_value->_waypoints.empty())
				spawnPos = nodeIt->_value->_waypoints[0];
		}

		// IDA: FrameCtr = doFramePer60FPS_46EBB7(); each snoid gets a staggered dNextRenderFrame.
		// Normal: interval rand(60..120) frames (~1-2s); more-action: rand(120..180) frames (~2-3s).
		uint32 nextStartFrame = getCurrentFrameCounter();
		const bool moreAction = !_vm->_state->isLessActionEnabled();

		while (isGeneratable && static_cast<int16>(_snoidMap.size()) < 16 && f._zmbGeneratedCount < 625) {
			const int16 zmbOnScreen = static_cast<int16>(_snoidMap.size());

			// Increment twinGenStatus for the generated trait
			const int16 snoidTraitId = _previewSnoid._trait.snoidId();
			if (f._twinGenStatus[snoidTraitId] < 2)
				++f._twinGenStatus[snoidTraitId];

			// Add to active pack
			const int16 packIdx = f._zmbPackActive._wPackZmbCount;
			if (packIdx < 16) {
				ZmbStateActiveEntry &entry = f._zmbPackActive._entries[packIdx];
				entry._traits = _previewSnoid._trait;
				entry._bIsOccupied = 1;
				Common::String nameStr = _vm->_text->fromU32String(_previewSnoid._name);
				memset(entry._name, 0, sizeof(entry._name));
				memcpy(entry._name, nameStr.c_str(), MIN<uint32>(nameStr.size(), sizeof(entry._name)));
				++f._zmbPackActive._wPackZmbCount;
			}

			// Spawn snoid walking to its seat, with deferred start for staggering.
			// IDA: CFeatureRunner::dNextRenderFrame = nextStartFrame.
			const Common::Point seatPos = _zoombiniSeatPoints[zmbOnScreen];
			const uint16 newSnoidId = kSnoidPackBase + _nextPackSnoidId++;
			ZmbSnoid *snoid = loadSnoidFromPack(newSnoidId, spawnPos,
			                                    ZmbFeature::FLAG_00000001_TYPE_SNOID);
			if (snoid) {
				snoid->_trait = _previewSnoid._trait;
				snoid->_name = _previewSnoid._name;
				snoid->setupIdleHotspots();
				snoid->setFacingLeft(spawnPos.x > seatPos.x);
				snoid->setAnimTargetPos(seatPos);
				snoid->setAnimState(kSnoidAnimDepart, nullptr);
				if (nextStartFrame > getCurrentFrameCounter()) {
					snoid->setDelayUntilFrame(nextStartFrame);
					snoid->deactivateRender();
				}
				// Track seat-to-snoid mapping for embark animation
				_seatToSnoid[zmbOnScreen] = snoid;
			}

			++f._zmbGeneratedCount;

			// IDA: FrameCtr += nextRand(120,60) [normal] or nextRand(180,120) [more action]
			if (moreAction)
				nextStartFrame += _vm->_rnd->getRandomNumber(120, 180);
			else
				nextStartFrame += _vm->_rnd->getRandomNumber(60, 120);

			// Roll next traits for the following iteration
			isGeneratable = false;
			for (int nextAttempt = 0; !isGeneratable && nextAttempt < 64; nextAttempt++) {
				_previewSnoid._trait._head = static_cast<uint8>(_vm->_rnd->getRandomNumber(1, 5));
				_previewSnoid._trait._eye  = static_cast<uint8>(_vm->_rnd->getRandomNumber(1, 5));
				_previewSnoid._trait._nose = static_cast<uint8>(_vm->_rnd->getRandomNumber(1, 5));
				_previewSnoid._trait._foot = static_cast<uint8>(_vm->_rnd->getRandomNumber(1, 5));
				isGeneratable = isZoombiniTraitGeneratable(_previewSnoid._trait);
			}
			if (isGeneratable)
				generateZoombiniName();
		}

		// Re-sync matrix buttons to reflect the current (next) preview trait
		const uint8 newTraits[4] = {
			_previewSnoid._trait._head,
			_previewSnoid._trait._eye,
			_previewSnoid._trait._nose,
			_previewSnoid._trait._foot,
		};
		for (uint32 row = 0; row < 4; row++) {
			for (uint32 col = 0; col < kMatrixColumns; col++) {
				_matrixButtonStateMap[row * kMatrixColumns + col]._isStuck = (newTraits[row] == col + 1);
			}
		}
	}

	// Enable Generate button only when result is generatable
	_pickerButtonStateMap[kPickerButtons_Generate]._isPressDisabled = !isGeneratable;
}

bool ZoombiniInteractivePicker::isZoombiniTraitGeneratable(ZmbTrait trait) const {
	if (!trait.isComplete())
		return false;
	int16 snoidId = trait.snoidId();
	return _vm->_state->_f._twinGenStatus[snoidId] < 2;
}

void ZoombiniInteractivePicker::generateZoombiniName() {
	_previewSnoid._name = _vm->_text->pickNextZoombiniName();
}

int16 ZoombiniInteractivePicker::loadZoombinisFromPack(ZmbStateActivePack &pack) {
	int16 count = 0;
	uint16 occupiedPosIdx = 0;

	// Clear seat-to-snoid mapping
	memset(_seatToSnoid, 0, sizeof(_seatToSnoid));

	for (int16 i = 0; i < pack._wPackZmbCount; i++) {
		ZmbStateActiveEntry &entry = pack._entries[i];

		// Skip entries without complete traits
		if (!entry._traits.isComplete())
			continue;

		// IDA: handleZoombiniAnimation_maybe_4528A6(0) — only occupied entries
		if (!(entry._bIsOccupied && !pack._bSkipOccupiedAnim))
			continue;

		// Occupied zoombinis get sequential seat positions
		// IDA: sub_4535B5(++v9, &posLoc) → _zoombiniSeatPoints[occupiedPosIdx]
		if (16 <= occupiedPosIdx)
			continue;
		Common::Point pos = _zoombiniSeatPoints[occupiedPosIdx];

		// Create ZmbSnoid from pack entry
		uint16 snoidId = kSnoidPackBase + _nextPackSnoidId++;
		ZmbSnoid *snoid = loadSnoidFromPack(snoidId, pos,
											ZmbFeature::FLAG_00000001_TYPE_SNOID);
		if (snoid) {
			snoid->_trait = entry._traits;
			snoid->_name = entry.getU32Name(_vm);
			snoid->_packIsOccupied = true;
			snoid->setupIdleHotspots();
			// Track seat-to-snoid mapping for embark animation
			_seatToSnoid[occupiedPosIdx] = snoid;
		}

		occupiedPosIdx++;
		count++;
	}

	// IDA: pGameState->zmbPackActive.wPackZmbCount = 0;
	pack._wPackZmbCount = 0;

	return count;
}

} // End of namespace Mohawk
