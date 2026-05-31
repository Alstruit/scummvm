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

#ifndef MOHAWK_ZOOMBINI_PAGES_INTERACTIVE_BASE_H
#define MOHAWK_ZOOMBINI_PAGES_INTERACTIVE_BASE_H

#include "audio/mixer.h"
#include "mohawk/zoombini_page.h"

namespace Mohawk {

class ZoombiniInteractive : public ZoombiniPage {
public:
	ZoombiniInteractive(MohawkEngine_Zoombini *vm, ZoombiniPageType pageType);
	~ZoombiniInteractive() override;

	void onAnimFrame() override;
	ZmbEventHandleResult onKeyDown(const Common::KeyState &kbd, bool kbdRepeat) override;
	ZmbEventHandleResult onMouseMove(const Common::Point &absPos, const Common::Point &relPos) override;

	// [*] Notification Box
	void showNotiBoxShort(const Common::U32String &ustr);
	void hideNotiBoxShort();
	void showNotiBoxLong(ZoombiniText::Key textKey);

	/**
	 * Debug: force immediate departure, bypassing puzzle completion guards.
	 * Called from the console "finishPuzzle" command.
	 * Calls debugPrepareForDeparture() then onGoButtonActivated().
	 */
	void debugFinishPuzzle();

	/**
	 * Override to set any page-specific state needed for a forced debug departure.
	 * Default implementation does nothing.
	 */
	virtual void debugPrepareForDeparture() {}

	/**
	 * Override to return a human-readable description of the current puzzle rule/answer.
	 * Default implementation returns "(no answer required)".
	 */
	virtual Common::String debugGetAnswer() const { return "(no answer required)"; }

protected:
	void onSnoidDragStarted(ZmbSnoid *snoid) override;
	void onSnoidDragEnded(ZmbSnoid *snoid) override;

	/**
	 * Represent buttons that continously do something while being pressed, e.g. storage scroll buttons
	 */
	class ContinuousButtonState {
	public:
		bool _enabled = false;
		bool _pressed = false;
		uint16 _hsNormalIdx = ZmbHotspot::kIndexNone;
		uint16 _hsPressedIdx = ZmbHotspot::kIndexNone;
		uint16 _shapeNormalId = ZmbHotspot::kShapeNone;
		uint16 _shapePressedId = ZmbHotspot::kShapeNone;
		uint16 _shapeHoverId = ZmbHotspot::kShapeNone;
		bool _isHovered = false;

		ContinuousButtonState() = default;
		virtual ~ContinuousButtonState() = default;
		ContinuousButtonState(uint16 hsNormalIdx, uint16 hsPressedIdx, uint16 normalShapeId, uint16 pressedShapeId)
			: _enabled(true), _hsNormalIdx(hsNormalIdx), _hsPressedIdx(hsPressedIdx), _shapeNormalId(normalShapeId), _shapePressedId(pressedShapeId) {
		}

		void setHoverState(uint16 hoverShapeId);
		bool hasHoverState() const;
		bool setHovered(bool hovered);
		void press();
		void release();
	};
	void continuousButton_selectShapes(ZmbFeature *feature, Common::Array<ZmbHotspot> &hotspots, Common::StableMap<uint32, ContinuousButtonState> &contButtonStateMap, uint16 pressedDeltaX = 0, uint16 pressedDeltaY = 0);

	// [*] Three Buttons
	void setGoButton(const Common::Rect &rect, uint16 shapeDisabledId, uint16 shapeEnabledId, uint16 shapePressedId);
	void setSecondGoButton(const Common::Rect &rect, uint16 shapeDisabledId, uint16 shapeEnabledId, uint16 shapePressedId);
	void setMapButton(const Common::Rect &rect, uint16 shapeNormalId, uint16 shapePressedId);
	void setHelpButton(const Common::Rect &rect);

	uint16 getGoButtonNormalShapeId(bool isPressable) { return isPressable ? _goButtonShapeEnabledId : _goButtonShapeDisabledId; }
	uint16 getSecondGoButtonNormalShapeId(bool isPressable) { return isPressable ? _secondGoButtonShapeEnabledId : _secondGoButtonShapeDisabledId; }

	void loadGoMapButtonsFeature(uint16 bitmapResId);
	void loadHelpButtonFeature();
	void setTlcButtonHoverIfPresent(ButtonState &buttonState, uint16 hoverShapeId, const ZmbResource &bitmapRes);
	void configureTlcGoMapButtonHover(uint16 bitmapResId);
	void updateTlcButtonHover(const Common::Point &absPos);

	void goMapButtons_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void goMapButtons_onPostRender(ZmbFeature *feature);
	void goMapButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs);
	ZmbEventHandleResult goMapButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);

	void helpButton_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void helpButton_onPostRender(ZmbFeature *feature);
	void helpButton_onPostAnimation(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs);
	ZmbEventHandleResult helpButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);

	enum ThreeButtonHotspotIdx {
		kHotspotGoButtonNormal = 0,
		kHotspotSecondGoButtonNormal = 1,
		kHotspotMapButtonNormal = 2,
		kHotspotGoButtonPressed = 3,
		kHotspotSecondGoButtonPressed = 4,
		kHotspotMapButtonPressed = 5,

		kHotspotHelpButtonNormal = 0,
		kHotspotHelpButtonPressed = 1,
	};

	enum ThreeButtonIdx {
		kThreeButtons_Go = 0,
		kThreeButtons_SecondGo,
		kThreeButtons_Map,
		kThreeButtons_Help,
	};

	enum ThreeButtonBitmapResId {
		kBasecamp1ButtonBitmapResId = 2100,
	};

	// Z1-20U/TLC v2.0 release only: hover shapes are yellow-outline variants
	// absent from the 1.x Broderbund/Korean releases.
	enum ThreeButtonShapeIdx {
		kShapeBasecamp1GoRouteUpButtonNormal = 1,
		kShapeBasecamp1GoRouteUpButtonPressed = 2,
		kShapeBasecamp1GoRouteDownButtonNormal = 3,
		kShapeBasecamp1GoRouteDownButtonPressed = 4,
		kShapeBasecamp1GoRouteUpButtonHover = 17,
		kShapeBasecamp1GoRouteDownButtonHover = 18,
		kShapeBasecamp1MapButtonHover = 19,

		kShapeGoButtonNormal = 2,
		kShapeGoButtonPressed = 3,
		kShapeMapButtonNormal = 5,
		kShapeMapButtonPressed = 6,
		kShapeGoButtonHover = 7,
		kShapeMapButtonHover = 8,
		kShapePickerGoButtonNormal = 9,
		kShapePickerGoButtonPressed = 10,
		kShapePickerMapButtonNormal = 11,
		kShapePickerMapButtonPressed = 12,
		kShapePickerGoButtonHover = 17,
		kShapePickerMapButtonHover = 18,
	};

	// [*] Notification Box
	constexpr static uint32 NOTIBOX_LONG_SHOW_FRAME_DURATION = 90; // 1.5 seconds at 60 FPS
	void showNotiBox(const Common::U32String &ustr, bool isNotiBoxLong);

	enum NotiBoxHotspotIdx {
		kHotspotNotiBoxShort = 0,
		kHotspotNotiBoxLong = 1,
	};

	void notiBox_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void notiBox_onPostRender(ZmbFeature *feature);

	// [*] StickyButtonState - Helper for sticky button handling.
	// Pressing toggles _isStuck immediately (no frame animation).
	class StickyButtonState {
	public:
		bool _enabled = false;
		ZmbResource _pressSoundId;   // sound played when sticking (unstuck -> stuck)
		ZmbResource _releaseSoundId; // sound played when unsticking (stuck -> unstuck)
		ZoombiniText::Key _textKey = ZoombiniText::kNone;
		uint16 _hsNormalId = ZmbHotspot::kIndexNone;      // hotspot shown at rest
		uint16 _hsPressedId = ZmbHotspot::kIndexNone;     // hotspot shown while press-animating (unused here, kept for hit-test)
		uint16 _normalShapeIdx = ZmbHotspot::kShapeNone;  // shape shown when not stuck
		uint16 _pressedShapeIdx = ZmbHotspot::kShapeNone; // shape shown when stuck
		bool _isStuck = false;

		StickyButtonState() = default;
		~StickyButtonState() = default;
		StickyButtonState(ZmbResource pressSoundId, uint16 hsNormalIdx, uint16 hsPressedIdx, uint16 normalShapeIdx, uint16 pressedShapeIdx)
			: _enabled(true), _pressSoundId(pressSoundId), _releaseSoundId(pressSoundId), _hsNormalId(hsNormalIdx), _hsPressedId(hsPressedIdx), _normalShapeIdx(normalShapeIdx), _pressedShapeIdx(pressedShapeIdx) {}
		StickyButtonState(ZmbResource pressSoundId, ZmbResource releaseSoundId, uint16 hsNormalIdx, uint16 hsPressedIdx, uint16 normalShapeIdx, uint16 pressedShapeIdx)
			: _enabled(true), _pressSoundId(pressSoundId), _releaseSoundId(releaseSoundId), _hsNormalId(hsNormalIdx), _hsPressedId(hsPressedIdx), _normalShapeIdx(normalShapeIdx), _pressedShapeIdx(pressedShapeIdx) {}
		StickyButtonState(ZoombiniText::Key textKey, ZmbResource pressSoundId, uint16 hsNormalIdx, uint16 hsPressedIdx, uint16 normalShapeIdx, uint16 pressedShapeIdx)
			: _enabled(true), _textKey(textKey), _pressSoundId(pressSoundId), _releaseSoundId(pressSoundId), _hsNormalId(hsNormalIdx), _hsPressedId(hsPressedIdx), _normalShapeIdx(normalShapeIdx), _pressedShapeIdx(pressedShapeIdx) {}
		StickyButtonState(ZoombiniText::Key textKey, ZmbResource pressSoundId, ZmbResource releaseSoundId, uint16 hsNormalIdx, uint16 hsPressedIdx, uint16 normalShapeIdx, uint16 pressedShapeIdx)
			: _enabled(true), _textKey(textKey), _pressSoundId(pressSoundId), _releaseSoundId(releaseSoundId), _hsNormalId(hsNormalIdx), _hsPressedId(hsPressedIdx), _normalShapeIdx(normalShapeIdx), _pressedShapeIdx(pressedShapeIdx) {}

		bool isStuck() const { return _isStuck; }

		void toggle(MohawkEngine_Zoombini *vm);
		void reset() { _isStuck = false; }
	};

	typedef void (ZoombiniInteractive::*OnStickyButtonActionFunc)(ZmbFeature *feature, uint32 bsIdx, StickyButtonState &bs);
	void genericStickyButton_selectShapes(ZmbFeature *feature, Common::Array<ZmbHotspot> &hotspots, Common::StableMap<uint32, StickyButtonState> &buttonStateMap);
	ZmbEventHandleResult genericStickyButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, StickyButtonState> &buttonStateMap, OnStickyButtonActionFunc onActionFunc = nullptr);
	ZmbEventHandleResult genericStickyButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, StickyButtonState> &buttonStateMap, const Common::HashMap<uint32, Common::Rect> &buttonRectMap, OnStickyButtonActionFunc onActionFunc = nullptr);

	/** Enable or disable both the primary and secondary Go buttons. */
	void setGoButtonsEnabled(bool enabled) {
		_goMapButtonStateMap[kThreeButtons_Go]._isPressDisabled = !enabled;
		_goMapButtonStateMap[kThreeButtons_SecondGo]._isPressDisabled = !enabled;
	}

	/**
	 * Override to implement departure-animation click-skip.
	 * IDA: all puzzle click handlers check puzzle_pendingTransitionTarget at the top:
	 * if set, they immediately complete the departure instead of processing clicks.
	 */
	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;

	/**
	 * Called when the Go button is activated.
	 * Default: plays departure SFX (SND 996) and sets _pendingGoDepart = true.
	 * Base class onAnimFrame() polls isDepartSfxDone() and calls executeDeparture().
	 *
	 * Override: set _departXferSrcSiPage and any page-specific pre-departure work
	 * (stop BGM, start walk animation), then call ZoombiniInteractive::onGoButtonActivated().
	 */
	virtual void onGoButtonActivated();
	virtual void onDisabledGoButtonActivated();

	/**
	 * Called when the secondary Go button is activated.
	 * Default: plays departure SFX and sets _pendingGoDepart = true (same as Go button).
	 */
	virtual void onSecondGoButtonActivated();
	virtual void onDisabledSecondGoButtonActivated();

	// [*] Two-phase departure system
	// IDA: puzzle_pendingTransitionTarget → puzzle_nextPuzzleId
	// funcOnHover (called every frame) polls puzzle_pendingTransitionTarget and commits
	// the transition after SFX 996 finishes. Centralized here in onAnimFrame().

	/** Xfer source page for the standard departure. Set before calling base onGoButtonActivated(). */
	ZMB_SI_PAGE _departXferSrcSiPage = ZMB_SI_MINUS1;

	/** True while a go-button departure is pending (SFX playing / walk animating). */
	bool _pendingGoDepart = false;

	/**
	 * Execute the page departure transition.
	 * Called by base class onAnimFrame() when departure SFX finishes.
	 * Default: saves snoid runners back to active pack, routes non-occupied
	 * snoids to the resting pack, sets _vm->_xferSrcPage, navigates to
	 * kXfer, and closes the page.
	 * Override for custom departure logic (e.g. BC1/BC2 save pack state).
	 */
	virtual void executeDeparture();

	/**
	 * Save all loaded snoid runners back to _zmbPackActive.
	 * IDA: save_updateZmbPacksOnPuzzleComplete(0, 1) — two-pass write:
	 *   Pass 1: occupied snoids (on pedestals, passed the puzzle).
	 *   Pass 2: non-occupied snoids (failed, to be routed to resting pack).
	 * Also resets movement direction and re-activates hidden snoids.
	 */
	void saveSnoidsToPack();

	/**
	 * Route non-occupied (failed) snoids from _zmbPackActive to their
	 * resting pack (BC0/BC1/BC2) based on the current route.
	 * IDA: the second half of save_updateZmbPacksOnPuzzleComplete.
	 * Container puzzles (Pizza/Slides/Net/Maze) route to BC0/BC1/BC2.
	 */
	void routeNonOccupiedToRestingPack();

	// [*] Departure walk-off animation infrastructure
	// IDA: zmbMoveAnimation_45479D — shared by Bridge, BC1, BC2, Tunnels, Pizza, Net.

	/**
	 * Start the shared departure walk-off animation.
	 * Iterates idle snoids, sets walk-to-target with staggered timing.
	 *
	 * IDA: zmbMoveAnimation_45479D(staggerDelay, toY, toX)
	 *
	 * @param target      Screen position all snoids walk toward (typically off-screen).
	 * @param stagger     Frames between each successive snoid starting its walk (default 45).
	 */
	void startDepartWalkAnimation(const Common::Point &target, uint32 stagger = 45);

	/**
	 * Poll whether all departure-walking snoids have finished.
	 * Returns true when every snoid is idle or off-screen.
	 */
	bool isDepartWalkComplete() const;

	/**
	 * Play the departure SFX and track its handle for completion polling.
	 * @param systemSoundId  System resource ID of the departure SFX (default 996).
	 */
	void playDepartSfx(uint16 systemSoundId = 996);

	/**
	 * Check whether the departure SFX has finished playing.
	 * Returns true immediately if no SFX handle was captured.
	 */
	bool isDepartSfxDone() const;

	/** Sound handle of the departure SFX, for completion polling. */
	Audio::SoundHandle _departSfxHandle;
	bool _hasDepartSfxHandle = false;

	/** Called when the Map button is activated. */
	virtual void onMapButtonActivated();
	/** Optional per-page confirmation before transitioning to RodMap. */
	virtual bool confirmMapTransition();
	/** Optional per-page save-state cleanup before transitioning to RodMap. */
	virtual void saveStateBeforeMapTransition();

	// [*] Snoid drag-and-drop infrastructure
	// IDA: beginDragFeatureRunner_45360F — universal drag handler.
	// Common state and helpers shared by all interactive pages.

	/** The snoid currently being dragged, or nullptr. */
	ZmbSnoid *_draggedSnoid = nullptr;

	/** Drag offset from snoid origin to mouse click point. */
	Common::Point _dragOffset;

	/** Original snoid position before drag started. */
	Common::Point _dragOrigPos;

	/** Previous mouse X position during drag, for computing movement direction. */
	int16 _dragPrevMouseX = 0;

	static const Common::Rect kDefaultDragConstraint;

	/** Whether a drag is currently active. */
	bool isDragging() const { return _draggedSnoid != nullptr; }

	/**
	 * Find the snoid under the cursor point by checking draw records.
	 * Default implementation iterates _snoidMap, checks FLAG_00000001_TYPE_SNOID
	 * and findDrawRecordAtPoint. Pages can override to add filtering (e.g.
	 * Bridge skips template snoids with ID < 10000).
	 */
	virtual ZmbSnoid *findSnoidAtPoint(const Common::Point &pos);

	/**
	 * Begin a snoid drag operation. Sets _draggedSnoid, _dragOrigPos, _dragOffset
	 * and invokes beginSnoidDrag() to set drag animation and hide cursor.
	 * Call from onLButtonDown after page-specific guards pass.
	 */
	void startSnoidDrag(ZmbSnoid *snoid, const Common::Point &mousePos);

	/**
	 * End a snoid drag operation. Clears _draggedSnoid and invokes endSnoidDrag()
	 * to restore cursor. Returns the formerly dragged snoid for drop evaluation.
	 */
	ZmbSnoid *finishSnoidDrag();

	/**
	 * Return the screen-space constraint rect for drag movement.
	 * Default: full screen (0,0,639,479). Pages override for smaller areas
	 * (e.g. Bridge uses left bank only).
	 */
	virtual const Common::Rect &getDragConstraintRect() const;

	// [*] Draw-on-Region Drop-Target Interaction
	// IDA: beginDragFeatureRunner_45360F — zone-radius highlighting and drop logic.

	/**
	 * Click zone radius for draw-on-reg drop detection.
	 * IDA: zmb_clickZoneRadius (0x4B6D3E) — default 15. Set per page in loadFeatures.
	 * Bridge=55, Picker=60, etc.
	 */
	int16 _clickZoneRadius = 15;

	/**
	 * Source draw-on-reg slot the snoid was picked up from, or -1.
	 * IDA: beginDragFeatureRunner_45360F srcSlotIdx (source occupancy cleared on pickup).
	 */
	int16 _dragSourceSlot = -1;

	/**
	 * Currently highlighted draw-on-reg slot during drag, or -1.
	 * IDA: beginDragFeatureRunner_45360F wFeatureRunnerIdx tracking.
	 */
	int16 _dragHighlightSlot = -1;

	/**
	 * Update draw-on-reg seat highlighting during drag.
	 * Called from onMouseMove. Highlights empty slots within zone radius,
	 * unhighlights when leaving.
	 * IDA: beginDragFeatureRunner_45360F 0x4539BF–0x453B51
	 */
	void updateDrawOnRegHighlight();

	/**
	 * Clear any active draw-on-reg highlight (e.g. on drag end).
	 */
	void clearDrawOnRegHighlight();

	/**
	 * Apply the 75/25 split: first 75% of loaded pack snoids remain idle at their
	 * pedestal positions; last 25% are given a walk-in animation from x=-50.
	 * IDA: zmb_layoutStaticAndWalkInGroups(0)
	 */
	void layoutStaticAndWalkIn();

	/**
	 * Sort walk-in snoids by Y depth and assign staggered dNextRenderFrame
	 * values (45 frames apart) so they enter the scene sequentially.
	 * IDA: zmb_assignStaggeredWalkDelays(0, 45)
	 */
	void assignStaggeredWalkDelays();

private:
	// [*] Ambient Sound Driver
	// IDA: ambient_runPerFrameSoundDriver_435F33 — runs from every puzzle tick handler.
	void runAmbientSoundDriver();
	uint32 _ambientNextPlayFrame = 0;     ///< Frame counter when next ambient sound is due
	uint16 _ambientLastSndId = 0;         ///< Resource ID of the last played ambient SND
	uint32 _ambientPoolBitmask = 0;       ///< Non-repeating bitmask for pool randomization
	uint16 _ambientPreloadCounter = 0;    ///< Counter for periodic preload cycle (mod 16)
	Audio::SoundHandle _ambientSndHandle; ///< Handle of the currently playing ambient sound

	/**
	 * Resource ID for Go/Map buttons bitmap
	 */
	uint16 _goMapBitmapResId = 0;

	// bool _goButtonPressable = false;
	Common::Rect _goButtonRect;
	uint16 _goButtonShapeDisabledId = ZmbHotspot::kShapeNone;
	uint16 _goButtonShapeEnabledId = ZmbHotspot::kShapeNone;
	uint16 _goButtonShapePressedId = ZmbHotspot::kShapeNone;

	// bool _secondGoButtonPressable = false;
	Common::Rect _secondGoButtonRect;
	uint16 _secondGoButtonShapeDisabledId = ZmbHotspot::kShapeNone;
	uint16 _secondGoButtonShapeEnabledId = ZmbHotspot::kShapeNone;
	uint16 _secondGoButtonShapePressedId = ZmbHotspot::kShapeNone;

	Common::Rect _mapButtonRect;
	uint16 _mapButtonShapeNormalId = ZmbHotspot::kShapeNone;
	uint16 _mapButtonShapePressedId = ZmbHotspot::kShapeNone;

	Common::Rect _helpButtonRect;

	Common::StableMap<uint32, ButtonState> _goMapButtonStateMap;
	Common::StableMap<uint32, ButtonState> _helpButtonStateMap;
	Common::HashMap<uint32, Common::Rect> _threeButtonRectMap;
	ZmbFeature *_goMapButtonsFeature = nullptr;
	ZmbFeature *_helpButtonFeature = nullptr;

	const Common::Rect _notiBoxShortRect = Common::Rect(0x0115, 0x01CA, 0x016C, 0x01DD);
	const Common::Rect _notiBoxLongRect = Common::Rect(0x0101, 0x01CA, 0x0183, 0x01DD);
	uint32 _notiBoxShowUntilFrame = 0;
	Common::U32String _notiBoxText;
	bool _isNotiBoxLong = false;
	ZmbFeature *_notiBoxFeature = nullptr;

protected:
	// [*] Active Help Sound (F1 key replay)
	// IDA: sound_activeHandle (0x4B94F4) — stores the help voice sound ID,
	// replayed by playOrEnqueueActiveSound_4626DB() when F1 is pressed.
	ZmbResource _activeHelpSoundId;

private:
	void playActiveHelpSound();
};

} // End of namespace Mohawk

#endif
