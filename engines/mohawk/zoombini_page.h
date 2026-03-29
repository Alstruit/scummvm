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

#ifndef MOHAWK_ZOOMBINI_PAGE_H
#define MOHAWK_ZOOMBINI_PAGE_H

#include "mohawk/zoombini_resource.h"

#include "common/events.h"
#include "common/stablemap.h"
#include "common/stack.h"

#include "mohawk/resource.h"
#include "mohawk/zoombini_scripts.h"
#include "mohawk/zoombini_text.h"

namespace Mohawk {

constexpr const char *ZMB_MHK_ZOOMBINI = "ZOOMBINI.MHK";
constexpr const char *ZMB_MHK_MIDIMPC = "MIDIMPC.MHK"; // Broderbund 1.x releases only
constexpr const char *ZMB_MHK_MUSIC = "MUSIC.MHK";     // The Learning Company 2.0 release only
constexpr const char *ZMB_MHK_XFER = "XFER.MHK";
constexpr const char *ZMB_MHK_RODMAP = "RODMAP.MHK";
constexpr const char *ZMB_MHK_PICKER = "PICKER.MHK";
constexpr const char *ZMB_MHK_TOWN = "TOWN.MHK";
constexpr const char *ZMB_MHK_BASECAMP = "BASECAMP.MHK";
constexpr const char *ZMB_MHK_BCTWO = "BCTWO.MHK";
constexpr const char *ZMB_MHK_BRIDGE = "BRIDGE.MHK";
constexpr const char *ZMB_MHK_TUNNELS = "TUNNELS.MHK";
constexpr const char *ZMB_MHK_PIZZA = "PIZZA.MHK";
constexpr const char *ZMB_MHK_FERRY = "FERRY.MHK";
constexpr const char *ZMB_MHK_LILLY = "LILLY.MHK";
constexpr const char *ZMB_MHK_SLIDES = "SLIDES.MHK";
constexpr const char *ZMB_MHK_FLEENS = "FLEENS.MHK";
constexpr const char *ZMB_MHK_HOTEL = "HOTEL.MHK";
constexpr const char *ZMB_MHK_NET = "NET.MHK";
constexpr const char *ZMB_MHK_CAVES = "CAVES.MHK";
constexpr const char *ZMB_MHK_SMOKE = "SMOKE.MHK";
constexpr const char *ZMB_MHK_MAZE2 = "MAZE2.MHK";

class MohawkEngine_Zoombini;
class MohawkSurface;

/**
 * One frame of a snoid walk animation: raw sprite shape and offsets per body slot.
 * Slot ordering follows zmbRunner_setAnimShape_456785 variant convention:
 *   variant=0: slot0=foot, slot1=body(base 0), slot2=nose, slot3=eye, slot4=head
 *   variant=1: slot0=foot, slot1=nose,          slot2=body(base 0), slot3=eye, slot4=head
 */
struct ZmbWalkFrame {
	int16 shape[5];      ///< Raw sprite shape index per slot (added to traitBase at render time)
	int16 x[5];          ///< X position offset per slot
	int16 y[5];          ///< Y position offset per slot
	uint8 entryCount = 0; ///< Number of valid slot entries (0 = empty frame, keep previous pose)
};

/** Parsed walk animation for one foot-type × direction combination (SCRS 105–129). */
struct ZmbWalkAnim {
	uint8  variant    = 0; ///< SCRS variant: 0=body at slot1, 1=body at slot2
	uint16 frameCount = 0; ///< Total animation frames in loop
	Common::Array<ZmbWalkFrame> frames;
};

class ZoombiniPage {
public:
	ZoombiniPage(MohawkEngine_Zoombini *vm, ZoombiniPageCategory pageCategory, ZoombiniPageType pageType);
	virtual ~ZoombiniPage();

	virtual void open() {}
	virtual void setBackgroundMusic() {}
	virtual void setBackgroundBitmap() {}
	virtual void loadFeatures() = 0;
	virtual void close();
	void onFrame();
	/**
	 * Called in every ScummVM render frames. Can reach over 60FPS when the game is running fast, so only use for non-animated logic that needs to be updated as fast as possible, e.g. mouse cursor movement.
	 */
	virtual void onEveryFrame() {};
	/**
	 * Called in every Zoombini animation frames (60FPS), where game logic tied to animation frames (e.g. ambient sound driver) should be updated and executed.
	 */
	virtual void onAnimFrame();

	void onFadeIn();
	void onFadeOut();

	virtual bool isClosed();

	void openArchive(const Common::String &name);
	ZoombiniPageType getPageType() const { return _pageType; }
	ZoombiniPageCategory getPageCategory() const { return _pageCategory; }
	void scheduleForceRedraw() { _doForceRedraw = true; }

	/**
	 * Called when a SCRB/SCRS feature fires a non-voice event code during animation.
	 * IDA: step callback / hotspotShapeOrFrameFunc dispatch.
	 * @param feature The feature that fired the event.
	 * @param eventCode The adjusted event code (IDA convention: raw - 1).
	 *                  -1 signals end-of-animation (PLAY_ONCE completion).
	 */
	virtual void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {}

	// [*] Page-level Event Handlers
	virtual ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos);
	virtual ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos);
	virtual ZmbEventHandleResult onWheelUp(const Common::Point &absPos);
	virtual ZmbEventHandleResult onWheelDown(const Common::Point &absPos);
	virtual ZmbEventHandleResult onMouseMove(const Common::Point &absPos, const Common::Point &relPos);
	virtual ZmbEventHandleResult onKeyDown(const Common::KeyState &kbd, bool kbdRepeat);
	virtual ZmbEventHandleResult onKeyUp(const Common::KeyState &kbd, bool kbdRepeat);
	virtual ZmbEventHandleResult onQuit();

	// [*] Feature Script (SCRB: Map Object)
	ZmbFeature *loadScrbFeature(ZmbResource imgResource, uint16 scrbId, uint32 frameInterval, uint32 flags, const ZmbFeature::EventHooks &eventHooks = ZmbFeature::EventHooks());
	ZmbFeature *loadScrbFeature(ZmbResource imgResource, uint16 scrbId, uint32 frameInterval, const Common::Point &point, uint32 flags, const ZmbFeature::EventHooks &eventHooks = ZmbFeature::EventHooks());
	ZmbFeature *loadVirtualFeature(ZmbResource imgResource, uint16 virtFeatureId, const Common::Array<ZmbHotspot> &hotspots, uint32 frameInterval, uint32 flags, const ZmbFeature::EventHooks &eventHooks);
	ZmbFeature *loadVirtualFeature(uint16 virtFeatureId, uint32 frameInterval, uint32 flags, const ZmbFeature::EventHooks &eventHooks);
	ZmbFeature *loadSubFeature(ZmbFeature *parentFeature, ZmbResource imgResource, uint16 scrbId);
	void unloadScrbFeature(uint16 scrbId);
	void unloadVirtualFeature(uint16 virtFeatureId);
	/**
	 * Register a sub-feature (already owned by a parent feature) into the page's active scrb feature map
	 * so that it is rendered independently. The page does NOT take ownership; the parent feature retains it.
	 * When the sub-feature's animation cycle ends, it is automatically detached via checkCloseFeatures().
	 * @param subFeature The sub-feature to register
	 */
	void attachSubFeature(ZmbFeature *subFeature);

	uint32 getCurrentFrameCounter() const { return _currentFrameCounter; }

	void renderFeatures();
	void checkCloseFeatures();

	/**
	 * Pre-render pass for a single feature: animation logic.
	 * IDA: runner_preRenderStandard (0x4619A1) — called for ALL features
	 * BEFORE Z-sorting. Handles frame selection, end-of-cycle events
	 * (CHAIN_SCRIPT, PLAY_ONCE), per-frame flag checks (SKIP_RENDER,
	 * SKIP_ONCE), and sound dispatch.
	 */
	void preRenderFeature(ZmbFeature *feature);

	/**
	 * Post-render pass for a single feature: shape blitting.
	 * IDA: runner_postRenderStandard (0x46182F) — called in Z-sorted order
	 * AFTER pre-render pass. Only blits shapes and computes sort rects.
	 */
	ZmbRenderResult blitShapes(ZmbFeature *feature);

	int32 selectRenderFrame(ZmbFeature *feature);

	/**
	 * Select-render-frame callback for SCRS script playback (states 8/9).
	 * Returns the snoid's internally-managed frame index, which is advanced
	 * one-per-tick by onSnoidAnimTick rather than time-cycled.
	 */
	int32 selectScrsRenderFrame(ZmbFeature *feature);

	/**
	 * Render a stored Zoombini (idle pose, right-facing) at the given screen position
	 * using the given traits, and return the bounding rect of the rendered shapes.
	 *
	 * Mirrors IDA: zmbRunner_setAnimShape_456785 + snoidScript_renderFrame_4562B2
	 * as called by onPostRenderVirtualSCRB_storage_tBMP2000_41265F.
	 *
	 * @param screenKind   Target screen buffer.
	 * @param trait        Trait data (head/eye/nose/foot) to use for the idle pose.
	 * @param pos          Screen position of the snoid anchor point.
	 * @return             Bounding rect of all drawn shapes, or an invalid Rect if nothing was drawn.
	 */
	Common::Rect renderStoredSnoid(ZoombiniGraphics::ScreenKind screenKind, const ZmbTrait &trait, const Common::Point &pos);

	void clear();
	void clearScrbFeatures();
	void clearVirtualFeatures();
	void clearSubFeatures();
	void clearSnoids();
	void clearRegs();
	void clearNode();

	// [*] SCRS: Snoid Script (Zoombini)
	ZmbSnoid *loadSnoid(ZmbResource imgResource, uint16 scrsId, uint32 flags, const ZmbFeature::EventHooks &eventHooks = ZmbFeature::EventHooks());
	ZmbSnoid *loadSnoid(ZmbResource imgResource, uint16 scrsId, const Common::Point &point, uint32 flags, const ZmbFeature::EventHooks &eventHooks = ZmbFeature::EventHooks());
	ZmbSnoid *loadSnoidFromPack(uint16 snoidId, const Common::Point &point, uint32 flags, const ZmbFeature::EventHooks &eventHooks = ZmbFeature::EventHooks());
	/**
	 * Load a SCRB-based snoid. Identical to loadSnoid() but reads an SCRB resource
	 * instead of SCRS. Use this for town inhabitants (SCRB 4000-4007), which share
	 * the same animation scripts as the SCRB sub-features but are registered as
	 * independent snoids. @p snoidId is used as the map key (must be unique);
	 * @p scrbId is the SCRB resource to parse.
	 * IDA: puzzleTown_457C7E — inhabitants use registerSCRB_45F60C with SNOID flag.
	 */
	ZmbSnoid *loadSnoidFromScrb(ZmbResource imgResource, uint16 snoidId, uint16 scrbId, const Common::Point &point, uint32 flags, const ZmbFeature::EventHooks &eventHooks = ZmbFeature::EventHooks());
	void unloadSnoid(uint16 scrsId);
	ZmbSnoid *getSnoid(uint16 scrsId) const;

	/**
	 * Return true if any stationary snoid (idle/flip/fidget) other than @p self
	 * is within @p distSquared squared-pixel distance of @p pt.
	 * IDA: snoid_collectIdlePositions (0x456ACA) — threshold is the
	 * raw squared distance; comparison is strict less-than, and only
	 * states 0/3/6 (idle/flip/fidget) are checked.
	 */
	bool isPointOccupiedByOtherSnoid(const ZmbSnoid *self, const Common::Point &pt, int32 distSquared) const;

	// [*] Snoid Drag Helpers
	/**
	 * Begin dragging a snoid: set drag animation and hide mouse cursor.
	 * IDA: beginDragFeatureRunner_45360F entry — animateZoombini(0, 5, pZmb)
	 * sets kSnoidAnimDrag; cursor is hidden so the snoid sprite acts as cursor.
	 * Called by all interactive pages when starting a drag operation.
	 */
	void beginSnoidDrag(ZmbSnoid *snoid);

	/**
	 * End dragging a snoid: restore flags and mouse cursor visibility.
	 * IDA: beginDragFeatureRunner_45360F 0x453CCF — bitmask restored from saved
	 * value; cursor restored after drag loop.
	 * The caller is responsible for setting the final animation state
	 * (idle, arrive, etc.) and position.
	 */
	void endSnoidDrag(ZmbSnoid *snoid);

	// [*] Terrain Barrier Bitmap
	/**
	 * Load a terrain barrier bitmap (tBMP resource) that defines walkable areas.
	 * The bitmap is 160x120 (screen / 4), 8bpp. Pixel value 1 = walkable.
	 * IDA: rmap_loadTerrainArchive (0x46001A).
	 * @param resId  tBMP resource ID from the page archive (e.g. 100, 500, 1600).
	 */
	void loadTerrainBitmap(uint16 resId);

	/**
	 * Check if a screen position is on walkable terrain.
	 * Maps screen coords to bitmap coords by dividing by 4, then checks pixel == 1.
	 * If no terrain bitmap is loaded, returns false (no terrain = invalid drop).
	 * IDA: terrain_validateAndPlaceSnoid (0x453D28) terrain check logic.
	 */
	bool isTerrainWalkable(int16 x, int16 y) const;

	/**
	 * Validate a snoid drop against the terrain barrier bitmap and adjust
	 * position to avoid collisions with idle snoids.
	 * IDA: terrain_validateAndPlaceSnoid (0x453D28).
	 *
	 * Flow:
	 *  1. If no terrain loaded, returns false (drop invalid).
	 *  2. Checks pixel at (snoidX/4, snoidY/4); pixel != 1 → returns false.
	 *  3. Checks collision with idle snoids (threshold 36 sq dist ≈ 6px).
	 *  4. If colliding, finds a non-colliding position via random offset scan.
	 *  5. Updates snoid position and returns true.
	 *
	 * Pages with terrain: BC2(100), Bridge(1600), Caves(100), Ferry(100),
	 * Fleens(500), Hotel(100), Maze2(100), Pizza(100), Slides(100),
	 * Smoke(100), Tunnels(100).
	 * Pages WITHOUT terrain: BC1, Picker, Town — always returns false.
	 */
	bool validateTerrainDrop(ZmbSnoid *snoid);

	/**
	 * Find a non-colliding position for a snoid by scanning random x-offsets.
	 * IDA: snoid_findNonCollidingPos (0x456C95) — when called from
	 * terrain_validateAndPlaceSnoid with (threshold=36, randSeed=0,
	 * gridParam=NULL): tries up to 20 random x-offset positions
	 * (4 * random(-5,5) pixels) at the same y. Keeps first non-colliding.
	 */
	Common::Point findNonCollidingPosition(const ZmbSnoid *self, const Common::Point &origin, int32 distSquared) const;

	/**
	 * Free the terrain barrier bitmap. Called by clear().
	 * IDA: clearTerrainSprite_4600B9.
	 */
	void clearTerrainBitmap();

	// [*] Constant
	enum CommonResourceId : uint16 {
		kResSound0996_DepartSFX = 996,
		kResSound0997_ArriveSFX = 997,
		kResSound0999_ButtonSFX = 999,

		kResSound20104_TownBGM = 20104,

		kResShapeBitmap0001_Dialog = 1,
		kResShapeBitmap0020_Credits = 20,
		kResShapeBitmap3001_NotiBox = 3001,

		kResScrb0001_DialogOptionsFrame = 1,
		kResScrb0002_DialogOptionsSmallButtons = 2,
		kResScrb0003_DialogOptionsBigButtons = 3,
		kResScrb0004_DialogLoad = 4,
		kResScrb0005_DialogLoad = 5,
		kResScrb0006_DialogLoad = 6,
		kResScrb0007_DialogSave = 7,
		kResScrb0008_DialogSave = 8,
		kResScrb0009_DialogSave = 9,
		kResScrb0010_DialogMsgBox = 10,
		kResScrb0011_DialogMsgBox = 11,
		kResScrb0012_Dialog = 12,
		kResScrb0013_Dialog = 13,
		kResScrb0014_Dialog = 14,
		kResScrb0015_Dialog = 15,
		kResScrb0016_Dialog = 16,
		kResScrb0017_DialogHelp = 17,
		kResScrb0018_Dialog = 18,
		kResScrb0020_DialogCredits = 20,

		kResStrl30000_ZoombiniNames = 30000,
		kResStrl30001_ZoombiniNames = 30001,
		kResStrl30002_ZoombiniNames = 30002,
		kResStrl30003_ZoombiniNames = 30003,
		kResStrl30004_ZoombiniNames = 30004,
		kResStrl30005_ZoombiniNames = 30005,
		kResStrl30006_ZoombiniNames = 30006,
	};

	enum VirtualFeatureId : uint16 {
		kVirtualFeatureMinus02_NotiBox = 0xFFFE,
		kVirtualFeatureMinus01_ButtonLast = 0xFFFF,
		kVirtualFeaturePlus01_ButtonRoot = 0x0001,
	};

	enum ShapeId0001 {
		kShape0001_01_OptionsDialog = 1,
		kShape0001_02_OptionsFrame = 2,
		kShape0001_03_OptionsRedButtonNormal = 3,
		kShape0001_04_OptionsRedButtonPressed = 4,
		kShape0001_05_OptionsOnButtonNormal = 5,
		kShape0001_06_OptionsOffButtonNormal = 6,
		kShape0001_07_OptionsOnButtonPressed = 7,
		kShape0001_08_OptionsOffButtonPressed = 8,
		kShape0001_09_ShortGreenButtonNormal = 9,
		kShape0001_10_ShortGreenButtonPressed = 10,
		kShape0001_11_SaveLoadListFrame = 11,
		kShape0001_12_LongGreenButtonNormal = 12,
		kShape0001_13_LongGreenButtonPressed = 13,
		kShape0001_14_LongRedButtonNormal = 14,
		kShape0001_15_LongRedButtonPressed = 15,
		kShape0001_16_SaveLoadScrollUpButtonNormal = 16,
		kShape0001_17_SaveLoadScrollUpButtonPressed = 17,
		kShape0001_18_SaveLoadScrollDownButtonNormal = 18,
		kShape0001_19_SaveLoadScrollDownButtonPressed = 19,
		kShape0001_20_SaveLoadInputFrame = 20,
		kShape0001_21_ModalDialog = 21,
		kShape0001_22_OptionsToggleLegendOn = 22,
		kShape0001_23_OptionsToggleLegendOff = 23,
		kShape0001_24_HelpButtonNormal = 24,
		kShape0001_25_HelpButtonPressed = 25,
		kShape0001_26_HelpDialogPrevButtonNormal = 26,
		kShape0001_27_HelpDialogPrevButtonPressed = 27,
		kShape0001_28_HelpDialogNextButtonNormal = 28,
		kShape0001_29_HelpDialogNextButtonPressed = 29,

		kShape3001_01_NotiBoxShort = 1,
		kShape3001_02_NotiBoxLong = 2,
	};

	// [*] NODE: Waypoint Paths for Snoid Animation
	/**
	 * Return the ZmbNode for the given resource id, or nullptr if not loaded.
	 * Snoid animation uses this during kSnoidAnimDepart to build a waypoint path.
	 */
	ZmbNode *getNode(uint16 nodeId) const {
		auto it = _nodeMap.find(nodeId);
		return (it != _nodeMap.end()) ? it->_value : nullptr;
	}

	/**
	 * Return the first loaded ZmbNode (pages typically have at most one),
	 * or nullptr if no nodes are loaded.
	 */
	ZmbNode *getFirstNode() const {
		if (_nodeMap.empty())
			return nullptr;
		return _nodeMap.begin()->_value;
	}

	/**
	 * Return walk animation data for the given foot type (1–5) and direction bucket (0–4).
	 * Loads all 25 walk SCRS resources (SCRS 105–129) from ZOOMBINI.MHK lazily on first call.
	 * @param footType  Snoid foot trait, 1–5.
	 * @param dirBucket Direction bucket, 0–4 (computed from movement slope).
	 */
	const ZmbWalkAnim &getWalkAnim(uint8 footType, int dirBucket);

	/**
	 * Return fidget animation data for the given set and variant.
	 * Set 0 = SCRS 130–136 (chZmbAnimShapeCommonImageIdx=1), set 1 = SCRS 138–144 (=2).
	 * Loads all 14 fidget SCRS resources lazily on first call.
	 * @param fidgetSet 0 (normal idle) or 1 (flipped/variant idle).
	 * @param variant   Random variant 0–6 (wAnimBaseFlag00F5 from nextRand(7)).
	 */
	const ZmbWalkAnim &getFidgetAnim(int fidgetSet, int variant);

	/**
	 * Return holding (drag) animation data for the given foot type.
	 * Loads SCRS 146–150 lazily on first call.
	 * @param footType Foot trait 1–5.
	 */
	const ZmbWalkAnim &getHoldingAnim(uint8 footType);

protected:
	MohawkEngine_Zoombini *_vm;

	ZoombiniPageCategory _pageCategory;
	ZoombiniPageType _pageType;
	bool _useFadeEffect = true;

	Common::StableMap<uint16, ZmbFeature *> _scrbFeatureMap;
	Common::StableMap<uint16, ZmbFeature *> _virtualFeatureMap;
	/**
	 * Sub-features temporarily running independently (e.g. FLAG_00040000_CHAIN_SCRIPT).
	 */
	Common::StableMap<uint16, ZmbFeature *> _subFeatureMap;
	Common::StableMap<uint16, ZmbSnoid *> _snoidMap;
	Common::HashMap<uint16, ZmbRegs *> _regsMap;
	Common::HashMap<uint16, ZmbNode *> _nodeMap;

	/** Terrain barrier bitmap for walkability checks. Null if not loaded. Not owned; cached by GraphicsManager. */
	MohawkSurface *_terrainBitmap = nullptr;

	/** Walk animation cache: _walkAnims[footType-1][dirBucket], lazily populated. */
	ZmbWalkAnim _walkAnims[5][5];
	bool _walkAnimsLoaded = false;
	void loadWalkAnims();

	/**
	 * Fidget animation cache: _fidgetAnims[set][variant].
	 * set 0 = SCRS 130–136 (chZmbAnimShapeCommonImageIdx=1).
	 * set 1 = SCRS 138–144 (chZmbAnimShapeCommonImageIdx=2, played with SFX).
	 */
	ZmbWalkAnim _fidgetAnims[2][7];
	bool _fidgetAnimsLoaded = false;
	void loadFidgetAnims();

	/**
	 * Holding (drag) animation cache: _holdingAnims[footType-1].
	 * Holds SCRS 146–150 (one per foot type 1–5).
	 * IDA: index = footTrait + 45 → SCRS 146–150.
	 */
	ZmbWalkAnim _holdingAnims[5];
	bool _holdingAnimsLoaded = false;
	void loadHoldingAnims();

	/**
	 * Saved bitmask of the dragged snoid, restored on endSnoidDrag.
	 * IDA: beginDragFeatureRunner_45360F 0x4536BD saves bitmask before
	 * ORing 0x4001000 (TOPMOST + OVERLAY) at 0x4536C3.
	 */
	uint32 _dragSavedSnoidFlags = 0;

	uint32 _pageStartFrameTime = 0;
	uint32 _pageStartFrameCounter = 0;
	uint32 _currentFrameTime = 0;
	uint32 _currentFrameCounter = 0;
	uint32 _lastFrameTime = 0;
	uint32 _lastFrameCounter = 0;
	bool _doForceRedraw = false;

	static ZmbFeature *registerFeature(ZoombiniPage *page, Common::StableMap<uint16, ZmbFeature *> &featureMap, ZmbResource imgResource, uint16 scrbId, uint32 frameInterval, const Common::Point &point, uint32 flags, bool isPhysicalScrb, const Common::Array<ZmbHotspot> *virtualHotspots, const ZmbFeature::EventHooks &eventHooks = ZmbFeature::EventHooks());
	static void deregisterFeature(Common::StableMap<uint16, ZmbFeature *> &featureMap, uint16 featureId);

	void loadNODE(ZmbArchiveKind archiveKind, uint16 imgResource);
	void loadREGS(ZmbArchiveKind archiveKind, uint16 imgResource);

	static constexpr uint32 BUTTON_PRESS_ANIMATION_FRAMES = 4;

	/**
	 * AnimateState - Helper for press-animation and toggle-animation handling
	 */
	class AnimateState {
	public:
		bool _firePostAnimationEvent = false;
		uint32 _animationStartFrame = 0;
		uint32 _animationFrameCount = BUTTON_PRESS_ANIMATION_FRAMES;
		ZmbResource _pressSoundId;

		AnimateState() = default;
		AnimateState(ZmbResource pressSoundId) : _pressSoundId(pressSoundId) {}
		virtual ~AnimateState() = default;

		void animate(uint32 frameCounter) {
			_animationStartFrame = frameCounter;
		}
		bool isAnimating() const { return _animationStartFrame != 0; }
		bool isAnimationDone(uint32 frameCounter) const {
			return isAnimating() && _animationFrameCount <= frameCounter - _animationStartFrame;
		}
		void setAnimateFrameCount(uint32 animFrameCount) { _animationFrameCount = animFrameCount; }
		virtual void reset() {
			_firePostAnimationEvent = false;
			_animationStartFrame = 0;
		}

	protected:
		// Subclasses should expose animation triggering through their own typed methods (e.g. press()).
		// Re-declare animate() as private in each subclass to enforce this.
	};

	// [*] ButtonState - Helper for press-animation-fire Button Handling
	class ButtonState : public AnimateState {
	public:
		bool _drawEnabled = false;
		ZoombiniText::Key _textKey = ZoombiniText::kNone;
		uint16 _hsNormalId = ZmbHotspot::kIndexNone;
		uint16 _hsPressedId = ZmbHotspot::kIndexNone;
		uint16 _shapeNormalIdx = ZmbHotspot::kShapeNone;
		uint16 _shapePressedIdx = ZmbHotspot::kShapeNone;

		bool _isPressDisabled = false;
		uint16 _shapeDisabledIdx = ZmbHotspot::kShapeNone;

		ButtonState() = default;
		~ButtonState() override = default;
		ButtonState(ZoombiniText::Key textKey, ZmbResource pressSoundId, uint16 hsNormalId, uint16 hsPressedId, uint16 normalShapeId, uint16 pressedShapeId)
			: AnimateState(pressSoundId), _drawEnabled(true), _textKey(textKey), _hsNormalId(hsNormalId), _hsPressedId(hsPressedId), _shapeNormalIdx(normalShapeId), _shapePressedIdx(pressedShapeId) {}
		ButtonState(ZmbResource pressSoundId, uint16 hsNormalId, uint16 hsPressedId, uint16 normalShapeId, uint16 pressedShapeId)
			: AnimateState(pressSoundId), _drawEnabled(true), _hsNormalId(hsNormalId), _hsPressedId(hsPressedId), _shapeNormalIdx(normalShapeId), _shapePressedIdx(pressedShapeId) {}

		void setDisabledState(uint16 disabledShapeId) {
			_shapeDisabledIdx = disabledShapeId;
		}
		bool hasDisabledState() const {
			return _shapeDisabledIdx != ZmbHotspot::kShapeNone;
		}
		void press(MohawkEngine_Zoombini *vm, uint32 frameCounter);

	private:
		using AnimateState::animate;
	};
	typedef void (ZoombiniPage::*OnButtonActionFunc)(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs);
	typedef Common::Rect (ZoombiniPage::*ButtonGetRectFunc)(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs, const Common::Rect &drawnRect);
	void genericButton_selectShapes(ZmbFeature *feature, Common::Array<ZmbHotspot> &hotspots, Common::StableMap<uint32, ButtonState> &buttonStateMap, uint16 pressedDeltaX = 0, uint16 pressedDeltaY = 0);
	void genericButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ButtonState> &buttonStateMap, Graphics::TextAlign textAlign = Graphics::kTextAlignLeft, int16 normalDeltaY = 0, int16 pressedDeltaY = 0);
	void genericButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ButtonState> &buttonStateMap, const ZoombiniGraphics::TextConf &tc, int16 normalDeltaY = 0, int16 pressedDeltaY = 0);
	void genericButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ButtonState> &buttonStateMap, ButtonGetRectFunc getRectFunc, const ZoombiniGraphics::TextConf &tc);
	void genericButton_action(ZmbFeature *feature, Common::StableMap<uint32, ButtonState> &buttonStateMap, OnButtonActionFunc onButtonActionFunc);
	ZmbEventHandleResult genericButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ButtonState> &buttonStateMap, OnButtonActionFunc onButtonActionFunc = nullptr);
	ZmbEventHandleResult genericButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ButtonState> &buttonStateMap, const Common::HashMap<uint32, Common::Rect> &buttonRectMap, OnButtonActionFunc onButtonActionFunc = nullptr);
	ZmbEventHandleResult genericButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ButtonState> &buttonStateMap, ButtonGetRectFunc getRectFunc, OnButtonActionFunc onButtonActionFunc = nullptr);

	// [*] ToggleButtonState - Helper for Toggle Button Handling
	class ToggleButtonState : public AnimateState {
	public:
		bool _enabled = false;
		ZoombiniText::Key _textKey = ZoombiniText::kNone;
		uint16 _hsNormalId = ZmbHotspot::kIndexNone;
		uint16 _hsPressedId = ZmbHotspot::kIndexNone;
		uint16 _onNormalShapeIdx = ZmbHotspot::kShapeNone;
		uint16 _onPressedShapeIdx = ZmbHotspot::kShapeNone;
		uint16 _offNormalShapeIdx = ZmbHotspot::kShapeNone;
		uint16 _offPressedShapeIdx = ZmbHotspot::kShapeNone;
		bool _toggleState = true; // On by default

		ToggleButtonState() = default;
		~ToggleButtonState() override = default;
		ToggleButtonState(ZoombiniText::Key textKey, ZmbResource pressSoundId, uint16 hsNormalIdx, uint16 hsPressedIdx, uint16 onNormalShapeIdx, uint16 onPressedShapeIdx, uint16 offNormalShapeIdx, uint16 offPressedShapeIdx)
			: AnimateState(pressSoundId), _enabled(true), _textKey(textKey), _hsNormalId(hsNormalIdx), _hsPressedId(hsPressedIdx), _onNormalShapeIdx(onNormalShapeIdx), _onPressedShapeIdx(onPressedShapeIdx), _offNormalShapeIdx(offNormalShapeIdx), _offPressedShapeIdx(offPressedShapeIdx) {}
		ToggleButtonState(ZmbResource pressSoundId, uint16 hsNormalIdx, uint16 hsPressedIdx, uint16 onNormalShapeIdx, uint16 onPressedShapeIdx, uint16 offNormalShapeIdx, uint16 offPressedShapeIdx)
			: AnimateState(pressSoundId), _enabled(true), _hsNormalId(hsNormalIdx), _hsPressedId(hsPressedIdx), _onNormalShapeIdx(onNormalShapeIdx), _onPressedShapeIdx(onPressedShapeIdx), _offNormalShapeIdx(offNormalShapeIdx), _offPressedShapeIdx(offPressedShapeIdx) {}

		void press(MohawkEngine_Zoombini *vm, uint32 frameCounter);
		void reset() override {
			_toggleState = true;
			AnimateState::reset();
		}

	private:
		using AnimateState::animate;
	};

	typedef void (ZoombiniPage::*OnToggleButtonPostAnimationFunc)(ZmbFeature *feature, uint32 bsIdx, ToggleButtonState &bs);
	typedef Common::Rect (ZoombiniPage::*ToggleButtonGetRectFunc)(ZmbFeature *feature, uint32 bsIdx, ToggleButtonState &bs, const Common::Rect &drawnRect);
	void genericToggleButton_selectShapes(ZmbFeature *feature, Common::Array<ZmbHotspot> &hotspots, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, uint16 pressedDeltaX = 0, uint16 pressedDeltaY = 0);
	void genericToggleButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, Graphics::TextAlign textAlign = Graphics::kTextAlignLeft);
	void genericToggleButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, const ZoombiniGraphics::TextConf &tc);
	void genericToggleButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, ToggleButtonGetRectFunc getRectFunc, const ZoombiniGraphics::TextConf &tc);
	void genericToggleButton_postAnimation(ZmbFeature *feature, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, OnToggleButtonPostAnimationFunc onButtonActionFunc);
	ZmbEventHandleResult genericToggleButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap);
	ZmbEventHandleResult genericToggleButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, const Common::HashMap<uint32, Common::Rect> &buttonRectMap);
	ZmbEventHandleResult genericToggleButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, ToggleButtonGetRectFunc getRectFunc);

private:
	static void categorizeFeature(ZmbFeature *feature, Common::Array<ZmbFeature *> &loopAnimList, Common::Array<ZmbFeature *> &normalList, Common::Array<ZmbFeature *> &entityList, Common::Array<ZmbFeature *> &overlayList);
	static void insertionSortFeatures(Common::Array<ZmbFeature *> &list);
	static void mergeSortedListInto(Common::Array<ZmbFeature *> &existingList, const Common::Array<ZmbFeature *> &incomingList);
	void buildSortedRenderList(Common::Array<ZmbFeature *> &outList);
	void buildSortedEventList(Common::Array<ZmbFeature *> &outList);

	bool _isClosed = false;
};

} // End of namespace Mohawk

#endif
