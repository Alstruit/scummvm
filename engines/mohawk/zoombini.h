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

#ifndef MOHAWK_ZOOMBINI_H
#define MOHAWK_ZOOMBINI_H

#include "common/queue.h"
#include "common/stack.h"

#include "mohawk/resource.h"
#include "mohawk/mohawk.h"

#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_debug.h"
#include "mohawk/zoombini_state.h"

namespace Common {

struct Event;

} // End of namespace Common


namespace Mohawk {

class ZoombiniGraphics;
class ZoombiniSound;
class ZoombiniMidiPlayer;
class ZoombiniRandom;
class ZoombiniText;
class ZoombiniGameState;
class ZoombiniPage;
class ZoombiniDialog;
class VideoManager;
class MidiPlayer;
class ZmbRegs;

class MohawkEngine_Zoombini : public MohawkEngine {
public:
	MohawkEngine_Zoombini(OSystem *syst, const MohawkGameDescription *gamedesc);
	~MohawkEngine_Zoombini() override;

	ZoombiniRandom *_rnd = nullptr;
	VideoManager *_video = nullptr;
	ZoombiniSound *_sound = nullptr;
	ZoombiniMidiPlayer *_midi = nullptr;
	ZoombiniGraphics *_gfx = nullptr;
	ZoombiniGameState *_state = nullptr;
	ZoombiniText *_text = nullptr;
	MohawkArchive *_sysMhk = nullptr;

	/**
	 * Registration-point offsets for system snoid shapes (tBMP 3000 in ZOOMBINI.MHK).
	 * Loaded once at startup from REGS 100+101 in ZOOMBINI.MHK.
	 * Used by blitShapes() to correctly anchor each body-part sprite to the
	 * snoid's base position (mirrors IDA's dword_4B731C / dword_4B7320 globals).
	 */
	ZmbRegs *_snoidShapeRegs = nullptr;

	/**
	 * Registration-point offsets for small snoid shapes (tBMP 3200 / 0xC80 in ZOOMBINI.MHK).
	 * Loaded once at startup from REGS 3200+3201 in ZOOMBINI.MHK.
	 * Used for the XFER FromIsle scene where sub_4572C5(0) swaps to small-scale shapes.
	 * Mirrors IDA's dword_4B731C / dword_4B7320 after the swap (IDA: loadREGS 0xC80/0xC81).
	 */
	ZmbRegs *_smallSnoidShapeRegs = nullptr;

	static constexpr uint32 kAnimateFrameRate = 60;
	static constexpr double kAnimateFrameTimeMs = 1000.0 / kAnimateFrameRate;
	/**
	 * Maximum FPS to operate (fade, cursor movement, etc.)
	 */
	static constexpr uint32 kTargetFrameRate = 180;
	static constexpr double kTargetFrameTimeMs = 1000.0 / kTargetFrameRate;
	/**
	 * Double-click time threshold in frame count.
	 * Default double-click time is 500 ms, following Windows default.
	 * - https://learn.microsoft.com/en-us/windows/win32/controls/ttm-setdelaytime
	 * - https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdoubleclicktime
	 * 
	 * In Zoombinis, double-click support is not mandatory.
	 * However, it is used in the LoadDialog to quickly select a save slot.
	 * 
	 * TODO: Make it configurable via ScummVM config?
	 */
	static constexpr uint32 kDoubleClickFrameTimeMs = 500;
	static constexpr uint32 kDoubleClickFrameRate = kAnimateFrameRate / (1000.0 / kDoubleClickFrameTimeMs);
	static constexpr uint32 kTextCursorBlinkFrameTimeMs = 500;
	static constexpr uint32 kTextCursorBlinkFrameRate = kAnimateFrameRate / (1000.0 / kTextCursorBlinkFrameTimeMs);
	/**
	 * Mouse cursor blink frame time for animated cursors.
	 * In original Zoombini engine, this value is 12, alternating valid frame and empty frame.
	 * To avoid flickering, skip empty frame and half this value to 6.
	 */
	static constexpr uint32 kMouseCursorEyeFrameRate = 6;
	static constexpr double kMouseCursorEyeFrameTimeMs = 1000 / kMouseCursorEyeFrameRate;
	
	
	void doFrame();
	void delayRunningFrames(uint32 ms);

	MohawkArchive *loadSystemArchive();
	/**
	 * Load next interactive/transition page from the page queue.
	 */
	void loadNextPage();

	void addPageArchive(Archive *archive);
	void removePageArchive(Archive *archive);
	void clearPageArchives();

	ZoombiniPage *getActivePage() const { return _activePage; }
	void setNextPage(ZoombiniPageType type);

	/**
	 * Source SI page set by each puzzle/area before transitioning to kXfer.
	 * Used by ZoombiniTransitionXfer to determine which route to display.
	 * IDA: wXferSrcSiPage (runtime global, reset after use).
	 */
	ZMB_SI_PAGE _xferSrcSiPage = ZMB_SI_MINUS1;

	/**
	 * IDA: word_4A4764. Global fidget interval threshold.
	 * Snoids trigger fidget only when their per-snoid idle counter exceeds
	 * this value. Default 64. Set to 0 to disable fidgets (e.g. during drag
	 * or XFER). Halved (min 1) when the game has been idle for 3600 ticks.
	 * Reset to 64 on user activity via resetFidgetActivity().
	 */
	uint16 _fidgetThreshold = 64;

	/**
	 * IDA: dLastGameLoopTime_4B825C. Frame counter at the last user activity
	 * event, used by doFrame() to detect idle periods for dynamic fidget
	 * threshold halving.
	 */
	uint32 _lastActivityFrame = 0;

	/**
	 * IDA: word_4B762C. Global counter for fidget sound preloading.
	 * Incremented each time any snoid triggers a fidget voice SFX.
	 * Every 32 triggers (counter wraps to 0 mod 32), SND resources
	 * 100–424 are preloaded into the archive cache.
	 */
	uint16 _fidgetSoundPreloadCounter = 0;

	/**
	 * IDA: word_4B6D4A. Global post-arrival turn-around state.
	 * Set by setArrivalTurnDirection() which maps movement direction
	 * (-1/0/1) to SnoidAnimState (1/0/2). On arrival (state 4) and path
	 * completion (state 112), snoids enter this state instead of idle.
	 * Values kSnoidAnimTurnRight(1)/kSnoidAnimTurnLeft(2) trigger a brief
	 * facing-direction flip before settling to idle.
	 */
	uint8 _arrivalTurnState = 0; // SnoidAnimState, default kSnoidAnimIdle

	/**
	 * IDA: ui_bDragLockActive. Global counter of snoids currently walking in.
	 * Incremented when kSnoidAnimArrivalMotion (state 10) fires, decremented
	 * when a snoid completes its path (state 112 arrival). Used by the picker
	 * to prevent concurrent drag during walk-in animations.
	 */
	int16 _walkersInProgress = 0;

	/**
	 * IDA: setZmbMovementDirection_45621A. Sets _arrivalTurnState from a
	 * movement direction value: -1 → kSnoidAnimTurnRight(1),
	 * 0 → kSnoidAnimIdle(0), 1 → kSnoidAnimTurnLeft(2).
	 */
	void setArrivalTurnDirection(int dir);

	/**
	 * IDA: currentFrameCounter_46084A. Called on user activity (input events,
	 * puzzle init, dialog close, etc.) to reset the fidget threshold to its
	 * default (64) and restart the idle timer.
	 */
	void resetFidgetActivity();
	
	bool hasDialogOpened() const;
	void openOptionsDialog();
	ZoombiniDialogResult openSaveDialog();
	ZoombiniDialogResult openLoadDialog(bool newGameMode = false);
	ZoombiniDialogResult openMsgBoxDialog(ZoombiniMsgBoxType type);
	void openCreditsDialog();
	void openHelpDialog(ZoombiniPageType forPage);
	void openDebugDialog(const ZoombiniDebugCommand &cmd);
	void closeActiveDialog();

	/**
	 * Change acitve resourceKind, and return last active resourceKind.
	 */
	ZmbArchiveKind setActiveResourceKind(ZmbArchiveKind kind);
	Common::SeekableReadStream *getResource(uint32 tag, uint16 id) override;
	Common::SeekableReadStream *getResource(uint32 tag, ZmbResource res);
	bool hasResource(uint32 tag, ZmbResource res);
	Common::Array<uint16> getResourceIDList(ZmbArchiveKind kind, uint32 tag) const;
	uint getArchiveCount(ZmbArchiveKind kind) const;
	Archive *getArchive(ZmbArchiveKind kind, uint archiveIdx) const;

	Common::Language getLanguage() const override;

	enum QuitEventState {
		kQuitEventNone = 0,
		kQuitEventRunning,
		kQuitEventDone,
	};
	bool mustQuit() const;

protected:
	Common::Error run() override;

private:
	void processEvents(ZoombiniPage *page);
	void processEvent(ZoombiniPage *page, const Common::Event &event);
	ZoombiniDialogResult loadModalDialog(ZoombiniDialog *page);

	Common::Language _language = Common::UNK_LANG;
	
	ZoombiniPage *_activePage = nullptr;
	ZmbArchiveKind _activeResourceKind = ZmbArchiveKind::kPage;

	Common::Queue<ZoombiniPageType> _pageQueue;
	Common::Stack<ZoombiniDialog *> _dialogPageStack;
	ZoombiniDialogResult _lastDialogResult = ZoombiniDialogResult::kNone;
	Common::Queue<Common::Event> _deferredEventQueue;
	QuitEventState _quitEventState = kQuitEventNone;
};

} // End of namespace Mohawk

#endif // MOHAWK_ZOOMBINI_H
