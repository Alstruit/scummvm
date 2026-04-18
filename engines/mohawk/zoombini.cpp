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

#include "mohawk/zoombini_resource.h"

#include "common/archive.h"
#include "common/config-manager.h"
#include "common/debug.h"
#include "common/error.h"
#include "common/events.h"
#include "common/fs.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "graphics/cursorman.h"

#include "mohawk/console.h"
#include "mohawk/cursors.h"
#include "mohawk/resource.h"
#include "mohawk/sound.h"
#include "mohawk/video.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_debug.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_page.h"
#include "mohawk/zoombini_pages/shelter_basecamp1.h"
#include "mohawk/zoombini_pages/shelter_basecamp2.h"
#include "mohawk/zoombini_pages/dialog_credits.h"
#include "mohawk/zoombini_pages/dialog_debug.h"
#include "mohawk/zoombini_pages/dialog_help.h"
#include "mohawk/zoombini_pages/dialog_msgbox.h"
#include "mohawk/zoombini_pages/dialog_options.h"
#include "mohawk/zoombini_pages/dialog_saveload.h"
#include "mohawk/zoombini_pages/interactive_rodmap.h"
#include "mohawk/zoombini_pages/shelter_picker.h"
#include "mohawk/zoombini_pages/puzzle_bridge.h"
#include "mohawk/zoombini_pages/puzzle_caves.h"
#include "mohawk/zoombini_pages/puzzle_pizza.h"
#include "mohawk/zoombini_pages/puzzle_ferry.h"
#include "mohawk/zoombini_pages/puzzle_lilly.h"
#include "mohawk/zoombini_pages/puzzle_net.h"
#include "mohawk/zoombini_pages/puzzle_fleens.h"
#include "mohawk/zoombini_pages/puzzle_hotel.h"
#include "mohawk/zoombini_pages/puzzle_slides.h"
#include "mohawk/zoombini_pages/puzzle_tunnels.h"
#include "mohawk/zoombini_pages/puzzle_smoke.h"
#include "mohawk/zoombini_pages/puzzle_maze.h"
#include "mohawk/zoombini_pages/shelter_town.h"
#include "mohawk/zoombini_pages/transition_logo.h"
#include "mohawk/zoombini_pages/transition_xfer.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_text.h"

namespace Mohawk {

namespace {

bool parsePracticeBootParam(int32 bootParam, ZoombiniPageType &pageType, uint16 &level) {
	if (bootParam <= 0)
		return false;

	int32 levelVal = bootParam % 100;
	int32 pageVal = bootParam / 100;
	if (levelVal < 1 || 4 < levelVal)
		return false;
	if (pageVal < static_cast<int32>(ZoombiniPageType::kBridge) ||
		static_cast<int32>(ZoombiniPageType::kMaze) < pageVal)
		return false;

	pageType = static_cast<ZoombiniPageType>(pageVal);
	level = static_cast<uint16>(levelVal);
	return true;
}

} // End of anonymous namespace.

MohawkEngine_Zoombini::MohawkEngine_Zoombini(OSystem *syst, const MohawkGameDescription *gamedesc) : MohawkEngine(syst, gamedesc) {
	DebugMan.addDebugChannel(kZmbDebugSaveLoad, "SaveLoad", "Track Save/Load Function");
	DebugMan.addDebugChannel(kZmbDebugPage, "Page", "Track Page Execution");
	DebugMan.addDebugChannel(kZmbDebugResource, "Resource", "Track Resource Parsing");
	DebugMan.addDebugChannel(kZmbDebugAnimation, "Animation", "Track Animation State");
	DebugMan.addDebugChannel(kZmbDebugRender, "Render", "Track Rendering");
}

MohawkEngine_Zoombini::~MohawkEngine_Zoombini() {
	delete _activePage;
	delete _sysMhk;
	delete _snoidShapeRegs;
	delete _smallSnoidShapeRegs;
	delete _snoidScriptShapeRegs;

	delete _midi;
	delete _sound;
	delete _video;
	delete _gfx;
	delete _rnd;
	delete _state;
	delete _text;
}

Common::Error MohawkEngine_Zoombini::run() {
	MohawkEngine::run();

	if (!_mixer->isReady()) {
		return Common::kAudioDeviceInitFailed;
	}

	setDebugger(new ZoombiniConsole(this));

	_language = getLanguage();

	_gfx = new ZoombiniGraphics(this);
	_video = new VideoManager(this);
	_sound = new ZoombiniSound(this);
	_midi = new ZoombiniMidiPlayer(this);
	_rnd = new ZoombiniRandom("zoombini");
	_state = new ZoombiniGameState(this, _saveFileMan);
	_text = new ZoombiniText(this, _language);

	_cursor = new ZoombiniCursorManager(this);
	_cursor->setDefaultCursor();
	_cursor->showCursor();

	// Load ZOOMBINI.MHK
	_sysMhk = loadSystemArchive();

	// Load global registration-point offsets for snoid body-part shapes.
	// IDA: dword_4B731C (X) and dword_4B7320 (Y) loaded from REGS 100+101 in ZOOMBINI.MHK.
	_snoidShapeRegs = new ZmbRegs();
	_snoidShapeRegs->parseStreams(this, ZmbArchiveKind::kSystem, 100, 101);

	// Load REGS offsets for small snoid shapes (resource 0xC80=3200/0xC81=3201).
	// IDA: sub_4572C5(0) loads these after swapping body-part tables.
	_smallSnoidShapeRegs = new ZmbRegs();
	_smallSnoidShapeRegs->parseStreams(this, ZmbArchiveKind::kSystem, 3200, 3201);

	// Load REGS offsets for SCRS-script-rendered snoids (resource 102/103 paired
	// with shape archive tBMP 3100). IDA `midiMpcLoad_452237` @ 0x4522BA-0x4522CB
	// loads dword_4B7324 (= REGS 0x66 = 102) and dword_4B7328 (= REGS 0x67 = 103).
	// Selected by `snoidScript_renderFrame_4562B2` for state 9 NORMAL playback,
	// which is what Ferry's reject-flight (case 1's SCRS 1900-1906) needs.
	_snoidScriptShapeRegs = new ZmbRegs();
	_snoidScriptShapeRegs->parseStreams(this, ZmbArchiveKind::kSystem, 102, 103);

	// Load a roster of game saves
	_state->loadRoster();

	// Load default page or a direct practice-mode target encoded as page*100+level.
	setActiveResourceKind(ZmbArchiveKind::kPage);
	ZoombiniPageType bootPracticePage = ZoombiniPageType::kNone;
	uint16 bootPracticeLevel = 0;
	int32 bootParam = ConfMan.getInt("boot_param");
	if (parsePracticeBootParam(bootParam, bootPracticePage, bootPracticeLevel)) {
		_state->_practiceLevel = bootPracticeLevel;
		_state->generateRandomPack();
		setNextPage(bootPracticePage);
		debug("Zoombini: boot_param=%d -> practice page=%d level=%u",
		      bootParam, static_cast<int32>(bootPracticePage), bootPracticeLevel);
		debugC(kZmbDebugPage, "Zoombini: boot_param=%d -> practice page=%d level=%u",
		       bootParam, static_cast<int32>(bootPracticePage), bootPracticeLevel);
	} else {
		if (bootParam != 0) {
			warning("Zoombini: ignoring unsupported boot_param %d (expected puzzlePage*100 + level, e.g. 1204 for Slides level 4)",
			        bootParam);
		}
		setNextPage(ZoombiniPageType::kLogo);
	}
	loadNextPage();

	// Main game loop
	while (!mustQuit()) {
		doFrame();
	}

	return Common::kNoError;
}

void MohawkEngine_Zoombini::resetFidgetActivity() {
	// IDA: currentFrameCounter_46084A — reset threshold to default and
	// restart the idle timer so the halving logic in doFrame() begins fresh.
	_lastActivityFrame = _system->getMillis() / static_cast<uint32>(kAnimateFrameTimeMs);
	if (_fidgetThreshold)
		_fidgetThreshold = 64;
}

void MohawkEngine_Zoombini::setArrivalTurnDirection(int dir) {
	// IDA: setZmbMovementDirection_45621A — maps movement direction to
	// post-arrival turn-around state: -1→1 (TurnRight), 0→0 (Idle), 1→2 (TurnLeft).
	if (dir == -1)
		_arrivalTurnState = 1; // kSnoidAnimTurnRight
	else if (dir == 1)
		_arrivalTurnState = 2; // kSnoidAnimTurnLeft
	else
		_arrivalTurnState = 0; // kSnoidAnimIdle
}

void MohawkEngine_Zoombini::processEvents(ZoombiniPage *page) {
	Common::Event event;

	// If fading, defer event processing until fade is done
	if (_gfx->isFading()) {
		// pollEvent() must be called to keep the mouse cursor moving
		while (_system->getEventManager()->pollEvent(event))
			_deferredEventQueue.push(event);

		return;
	}

	// Process deferred events first
	while (!_deferredEventQueue.empty()) {
		event = _deferredEventQueue.front();
		_deferredEventQueue.pop();
		processEvent(page, event);
	}

	// Process new events
	while (_system->getEventManager()->pollEvent(event))
		processEvent(page, event);
}

void MohawkEngine_Zoombini::processEvent(ZoombiniPage *page, const Common::Event &event) {
	// IDA: currentFrameCounter_46084A is called on user input to reset fidget
	// threshold and idle timer.  We call it on any mouse/keyboard event.
	switch (event.type) {
	case Common::EVENT_LBUTTONDOWN:
	case Common::EVENT_LBUTTONUP:
	case Common::EVENT_MOUSEMOVE:
	case Common::EVENT_KEYDOWN:
	case Common::EVENT_KEYUP:
		resetFidgetActivity();
		break;
	default:
		break;
	}

	switch (event.type) {
	case Common::EVENT_LBUTTONDOWN:
		page->onLButtonDown(event.mouse, event.relMouse);
		break;
	case Common::EVENT_LBUTTONUP:
		page->onLButtonUp(event.mouse, event.relMouse);
		break;
	case Common::EVENT_WHEELUP:
		page->onWheelUp(event.mouse);
		break;
	case Common::EVENT_WHEELDOWN:
		page->onWheelDown(event.mouse);
		break;
	case Common::EVENT_MOUSEMOVE:
		page->onMouseMove(event.mouse, event.relMouse);
		break;
	case Common::EVENT_KEYDOWN:
		page->onKeyDown(event.kbd, event.kbdRepeat);
		break;
	case Common::EVENT_KEYUP:
		page->onKeyUp(event.kbd, event.kbdRepeat);
		break;
	case Common::EVENT_QUIT:
	case Common::EVENT_RETURN_TO_LAUNCHER:
		page->onQuit();
		page->close();

		if (page != _activePage) {
			_activePage->onQuit();
			_activePage->close();
		}

		if (_quitEventState == kQuitEventNone) {
			_quitEventState = kQuitEventRunning;
			_gfx->setMouseCursor(ZoombiniGraphics::kResCursor01_Watch);
		}
		break;
	default:
		break;
	}
}

void MohawkEngine_Zoombini::doFrame() {
	// Update background running things
	uint32 frameStartTime = _system->getMillis();

	_sound->updateSoundQueue();

	// IDA: gameMainLoop_45DDD4 — when idle > 3600 ticks (~60s at 60fps),
	// halve the fidget threshold (minimum 1) to increase fidget frequency.
	if (_fidgetThreshold) {
		uint32 curFrame = frameStartTime / static_cast<uint32>(kAnimateFrameTimeMs);
		if (curFrame - _lastActivityFrame > 3600) {
			_lastActivityFrame = curFrame;
			_fidgetThreshold /= 2;
			if (!_fidgetThreshold)
				_fidgetThreshold = 1;
		}
	}

	bool isDialogOpened = !_dialogPageStack.empty();
	ZoombiniPage *page = nullptr;
	if (isDialogOpened)
		page = _dialogPageStack.top();
	else
		page = _activePage;

	// Debugger console is spawned in pollEvent() if requested.
	processEvents(page);

	// Page frame update
	page->onFrame();

	// Update the screen once per frame
	_gfx->flushScreens();
	bool inFade = _gfx->applyFadeEffect(frameStartTime);
	_system->updateScreen();

	// Process cursor animation
	if (_gfx->isMouseCursorEyeAnimationActive()) {
		if (inFade)
			_gfx->runMouseCursorEyeAnimationFrame(frameStartTime);
		else
			_gfx->stopMouseCursorEyeAnimation();
	}

	// Check if page is finished
	if (!inFade && page->isClosed()) {	
		if (isDialogOpened) {
			closeActiveDialog();
		} else {
			loadNextPage();
		}
		page = nullptr;
	}

	// Cut down on CPU usage
	uint32 loopElapsed = _system->getMillis() - frameStartTime;
	if (loopElapsed < kTargetFrameTimeMs)
		_system->delayMillis(kTargetFrameTimeMs - loopElapsed);
}

void MohawkEngine_Zoombini::delayRunningFrames(uint32 ms) {
	uint32 startTime = _system->getMillis();

	while (_system->getMillis() < startTime + ms && !mustQuit()) {
		doFrame();
	}
}

MohawkArchive *MohawkEngine_Zoombini::loadSystemArchive() {
	MohawkArchive *mhkArchive = new MohawkArchive();
	if (!mhkArchive->openFile(Common::Path(ZMB_MHK_ZOOMBINI))) {
		error("Cannot open resource file '%s'", ZMB_MHK_ZOOMBINI);
	}

	return mhkArchive;
}

void MohawkEngine_Zoombini::loadNextPage() {
	if (_quitEventState == kQuitEventRunning) {
		_quitEventState = kQuitEventDone;
		return;
	}

	if (_activePage) {
		delete _activePage;
		_activePage = nullptr;
	}
	_gfx->clearScreens();
	_gfx->clearCache();

	assert(!_pageQueue.empty());
	ZoombiniPageType nextPageType = _pageQueue.pop();

	ZoombiniPage *page;
	switch (nextPageType) {
	case ZoombiniPageType::kLogo:
		page = new ZoombiniTransitionLogo(this);
		break;
	case ZoombiniPageType::kRodMap:
		page = new ZoombiniInteractiveRodMap(this);
		break;
	case ZoombiniPageType::kXfer:
		page = new ZoombiniTransitionXfer(this);
		break;
	case ZoombiniPageType::kPicker:
		page = new ZoombiniShelterPicker(this);
		break;
	case ZoombiniPageType::kBasecamp1:
		page = new ZoombiniShelterBasecampOne(this);
		break;
	case ZoombiniPageType::kTown:
		page = new ZoombiniShelterTown(this);
		break;
	case ZoombiniPageType::kBasecamp2:
		page = new ZoombiniShelterBasecampTwo(this);
		break;
	case ZoombiniPageType::kBridge:
		page = new ZoombiniPuzzleBridge(this);
		break;
	case ZoombiniPageType::kCaves:
		page = new ZoombiniPuzzleCaves(this);
		break;
	case ZoombiniPageType::kPizza:
		page = new ZoombiniPuzzlePizza(this);
		break;
	case ZoombiniPageType::kFerry:
		page = new ZoombiniPuzzleFerry(this);
		break;
	case ZoombiniPageType::kLilly:
		page = new ZoombiniPuzzleLilly(this);
		break;
	case ZoombiniPageType::kSlides:
		page = new ZoombiniPuzzleSlides(this);
		break;
	case ZoombiniPageType::kFleens:
		page = new ZoombiniPuzzleFleens(this);
		break;
	case ZoombiniPageType::kHotel:
		page = new ZoombiniPuzzleHotel(this);
		break;
	case ZoombiniPageType::kNet:
		page = new ZoombiniPuzzleNet(this);
		break;
	case ZoombiniPageType::kTunnels:
		page = new ZoombiniPuzzleTunnels(this);
		break;
	case ZoombiniPageType::kSmoke:
		page = new ZoombiniPuzzleSmoke(this);
		break;
	case ZoombiniPageType::kMaze:
		page = new ZoombiniPuzzleMaze(this);
		break;
	default:
		error("Not implemented page: %d", static_cast<int32>(nextPageType));
		break;
	}

	_activePage = page;
	if (page->getPageCategory() == ZoombiniPageCategory::kInteractive)
		_state->_f.setCurrentPageType(nextPageType);

	// IDA: execActivePuzzle_435BE8 (0x435E27-60) — perfect streak flag.
	// Set to true when entering the first puzzle of a route.
	// In practice mode, always clear.
	if (_state->inPracticeMode()) {
		_state->_perfectStreakFlag = false;
	} else if (nextPageType == ZoombiniPageType::kBridge ||
	           nextPageType == ZoombiniPageType::kFerry ||
	           nextPageType == ZoombiniPageType::kFleens ||
	           nextPageType == ZoombiniPageType::kCaves) {
		_state->_perfectStreakFlag = true;
	}

	page->open();
	page->setBackgroundMusic();
	page->setBackgroundBitmap();
	page->loadFeatures();
	page->onFadeIn();
}

void MohawkEngine_Zoombini::addPageArchive(Archive *archive) {
	_mhk.push_back(archive);
	debugC(kZmbDebugPage, "addArchive: added page archive %p, now size: %u", reinterpret_cast<void *>(archive), _mhk.size());
}

void MohawkEngine_Zoombini::removePageArchive(Archive *archive) {
	debugC(kZmbDebugPage,"trying to remove page archive %p, now size: %u", reinterpret_cast<void *>(archive), _mhk.size());
	for (uint i = 0; i < _mhk.size(); i++) {
		if (archive != _mhk[i])
			continue;
		_mhk.remove_at(i);
		delete archive;
		debugC(kZmbDebugPage,"removeArchive removed and deleted archive %p, now size: %u", reinterpret_cast<void *>(archive), _mhk.size());
		return;
	}

	error("removeArchive didn't find archive %p, now size: %u", reinterpret_cast<void *>(archive), _mhk.size());
}

void MohawkEngine_Zoombini::clearPageArchives() {
	MohawkEngine::closeAllArchives();
}

Common::Language MohawkEngine_Zoombini::getLanguage() const {
	Common::Language language = MohawkEngine::getLanguage();
	if (language == Common::UNK_LANG)
		language = Common::EN_ANY;
	return language;
}

void MohawkEngine_Zoombini::setNextPage(ZoombiniPageType type) {
	_pageQueue.clear();
	_pageQueue.push(type);
}

bool MohawkEngine_Zoombini::hasDialogOpened() const {
	return !_dialogPageStack.empty();
}

void MohawkEngine_Zoombini::openOptionsDialog() {
	ZoombiniDialog *dialogPage = new ZoombiniDialogOptions(this);
	loadModalDialog(dialogPage);
}

ZoombiniDialogResult MohawkEngine_Zoombini::openSaveDialog() {
	ZoombiniDialog *dialogPage = new ZoombiniDialogSaveLoad(this, ZoombiniDialogSaveLoad::kSaveMode);
	return loadModalDialog(dialogPage);
}

ZoombiniDialogResult MohawkEngine_Zoombini::openLoadDialog(bool newGameMode) {
	if (_state->_r._saveCount1 == 0) {
		openMsgBoxDialog(ZoombiniMsgBoxType::kAlertNoSavedGame);
		return ZoombiniDialogResult::kNo;
	}

	ZoombiniDialogSaveLoad::SaveLoadMode mode = newGameMode ? ZoombiniDialogSaveLoad::kLoadOrNewMode : ZoombiniDialogSaveLoad::kLoadMode;
	ZoombiniDialog *dialogPage = new ZoombiniDialogSaveLoad(this, mode);
	return loadModalDialog(dialogPage);
}

ZoombiniDialogResult MohawkEngine_Zoombini::openMsgBoxDialog(ZoombiniMsgBoxType type) {
	ZoombiniDialog *dialogPage = new ZoombiniDialogMsgBox(this, type);
	return loadModalDialog(dialogPage);
}

void MohawkEngine_Zoombini::openCreditsDialog() {
	ZoombiniDialog *dialogPage = new ZoombiniDialogCredits(this);
	loadModalDialog(dialogPage);
}

void MohawkEngine_Zoombini::openHelpDialog(ZoombiniPageType forPage) {
	ZoombiniDialog *dialogPage = new ZoombiniDialogHelp(this, forPage);
	loadModalDialog(dialogPage);
}

void MohawkEngine_Zoombini::openDebugDialog(const ZoombiniDebugCommand &cmd) {
	ZoombiniDialog *dialogPage = new ZoombiniDialogDebug(this, cmd);
	loadModalDialog(dialogPage);
}

ZoombiniDialogResult MohawkEngine_Zoombini::loadModalDialog(ZoombiniDialog *dialogPage) {
	if (_dialogPageStack.empty())
		_mixer->pauseAll(true);

	dialogPage->open();
	dialogPage->setBackgroundBitmap();
	dialogPage->loadFeatures();
	_dialogPageStack.push(dialogPage);

	// Loop on a dialog page until the dialog is closed.
	// Do not loop on DebugDialog, as looping here make a debugger console blocking the screen.
	if (dialogPage->getPageType() != ZoombiniPageType::kDialogDebug) {
		uint32 dialogStackSize = _dialogPageStack.size();
		while (!mustQuit() && _dialogPageStack.size() == dialogStackSize)
			doFrame();
	}

	ZoombiniDialogResult result = _lastDialogResult;
	_lastDialogResult = ZoombiniDialogResult::kNone;
	return result;
}

void MohawkEngine_Zoombini::closeActiveDialog() {
	if (_dialogPageStack.empty())
		error("There is no modal dialog opened");

	ZoombiniDialog *dialogPage = _dialogPageStack.pop();
	_lastDialogResult = dialogPage->getResult();
	assert(dialogPage != nullptr);
	delete dialogPage;

	if (_dialogPageStack.empty()) {
		_mixer->pauseAll(false);
		// IDA gfx_renderFrame (0x45F070): the original resets dirty state
		// when dialogs close (dlg_wPendingCloseKind).  Force a full redraw
		// of the underlying page so that any dialog-painted pixels on the
		// persistent shapeScreen are overwritten.
		if (_activePage)
			_activePage->scheduleForceRedraw();
	}
}

ZmbArchiveKind MohawkEngine_Zoombini::setActiveResourceKind(ZmbArchiveKind kind) {
	ZmbArchiveKind lastKind = _activeResourceKind;
	_activeResourceKind = kind;
	return lastKind;
}

Common::SeekableReadStream *MohawkEngine_Zoombini::getResource(uint32 tag, uint16 id) {
	return getResource(tag, ZmbResource(_activeResourceKind, id));
}

Common::SeekableReadStream *MohawkEngine_Zoombini::getResource(uint32 tag, ZmbResource res) {
	// Pre-check existence so the failure message can tell the user WHICH
	// archive was searched (kPage = current .mhk page archive, kSystem =
	// ZOOMBINI.MHK). The Mohawk base class `error` on miss only says
	// "Archive does not contain 'XXXX' NNNN!" without naming the archive,
	// which makes it impossible to tell whether the caller targeted the
	// wrong archive or the resource is genuinely missing.
	char tagBuf[5];
	tagBuf[0] = static_cast<char>((tag >> 24) & 0xFF);
	tagBuf[1] = static_cast<char>((tag >> 16) & 0xFF);
	tagBuf[2] = static_cast<char>((tag >> 8) & 0xFF);
	tagBuf[3] = static_cast<char>(tag & 0xFF);
	tagBuf[4] = 0;
	switch (res._archiveKind) {
	case ZmbArchiveKind::kSystem:
		if (!_sysMhk->hasResource(tag, res._id))
			error("Zoombini: resource '%s' id %u (0x%04x) not found in kSystem (ZOOMBINI.MHK)",
			      tagBuf, res._id, res._id);
		return _sysMhk->getResource(tag, res._id);
	case ZmbArchiveKind::kPage:
		if (!MohawkEngine::hasResource(tag, res._id))
			error("Zoombini: resource '%s' id %u (0x%04x) not found in kPage (current page archive)",
			      tagBuf, res._id, res._id);
		return MohawkEngine::getResource(tag, res._id);
	default:
		error("Invalid ZmbArchiveKind: %u", static_cast<uint32>(res._archiveKind));
		break;
	}
	return nullptr;
}

bool MohawkEngine_Zoombini::hasResource(uint32 tag, ZmbResource res) {
	switch (res._archiveKind) {
	case ZmbArchiveKind::kSystem:
		return _sysMhk->hasResource(tag, res._id);
	case ZmbArchiveKind::kPage:
		return MohawkEngine::hasResource(tag, res._id);
	default:
		error("Invalid ZmbResourceKind: %u", static_cast<uint32>(res._archiveKind));
		break;
	}
	return false;
}

Common::Array<uint16> MohawkEngine_Zoombini::getResourceIDList(ZmbArchiveKind kind, uint32 tag) const {
	Common::Array<uint16> ids;

	switch (kind) {
	case ZmbArchiveKind::kSystem:
		ids.push_back(_sysMhk->getResourceIDList(tag));
		break;
	case ZmbArchiveKind::kPage:
		for (Mohawk::Archive *mhk : _mhk)
			ids.push_back(mhk->getResourceIDList(tag));
		break;
	default:
		error("Invalid ZmbResourceKind: %u", static_cast<uint32>(kind));
		break;
	}

	return ids;
}

uint MohawkEngine_Zoombini::getArchiveCount(ZmbArchiveKind kind) const {
	switch (kind) {
	case ZmbArchiveKind::kSystem:
		return 1;
	case ZmbArchiveKind::kPage:
		return _mhk.size();
	default:
		error("Invalid ZmbArchiveKind: %u", static_cast<uint32>(kind));
	}
	return 0;
}

Archive *MohawkEngine_Zoombini::getArchive(ZmbArchiveKind kind, uint archiveIdx) const {
	switch (kind) {
	case ZmbArchiveKind::kSystem:
		assert(archiveIdx == 0);
		return _sysMhk;
	case ZmbArchiveKind::kPage:
		assert(archiveIdx < _mhk.size());
		return _mhk[archiveIdx];
	default:
		error("Invalid ZmbArchiveKind: %u", static_cast<uint32>(kind));
	}
	return nullptr;
}

bool MohawkEngine_Zoombini::mustQuit() const {
	// When there is a scheduled quit event, wait until the fadeOut is done.
	return shouldQuit() && _quitEventState == kQuitEventDone;
}

} // End of namespace Mohawk
