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

#ifndef MOHAWK_ZOOMBINI_PAGES_SHELTER_TOWN_H
#define MOHAWK_ZOOMBINI_PAGES_SHELTER_TOWN_H

#include "mohawk/zoombini_pages/shelter_base.h"

namespace Mohawk {

class ZoombiniShelterTown : public ZoombiniShelter {
public:
	ZoombiniShelterTown(MohawkEngine_Zoombini *vm);
	~ZoombiniShelterTown() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	ZmbEventHandleResult onKeyDown(const Common::KeyState &kbd, bool kbdRepeat) override;
	void onEveryFrame() override;
	ZmbEventHandleResult onMouseMove(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbSnoid *findSnoidAtPoint(const Common::Point &pos) override;
	void saveStateBeforeMapTransition() override;

	// --- Memorial card overlay system (IDA town_renderMemorialCard @ 0x4595C0) ---

	/**
	 * Shows the memorial card for the given slot. IDA: registers a TOPMOST
	 * SCRB with the 5-row text layout (level title, route name, practice label,
	 * difficulty, date) and disables click on other runners via
	 * `runner_setAttrOnGroupAndList(0)`.
	 */
	void showMemorialCard(int16 slotIdx);

	/** Dismiss the memorial card overlay. IDA: click anywhere → dismiss. */
	void hideMemorialCard();

	/** Resolve a town walker drag release. */
	void endDrag(const Common::Point &dropPos);

	/** Hit-test against memorial statue hotspots (16 card slots). IDA: click_hitTestMemorialHotspots @ 0x458FF4. */
	int16 hitTestMemorialHotspots(const Common::Point &pos) const;
	bool isTownButtonRect(const Common::Point &pos) const;

	// Pre-render shape callback for population-gated overlay features (SCRB 1002, 1003)
	void overlay_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void memorialMarkers_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void memorialCard_onPostRender(ZmbFeature *feature);

	/**
	 * Transfer active pack Zoombinis into town stored chunk entries.
	 * Each Zoombini is stored at the index matching its snoidId().
	 */
	void transferActivePackToTownStorage();

	/**
	 * IDA town_shiftRunnersForScroll(phaseIdx): horizontally shifts town
	 * entity positions by one 320px column with wrap across the 1920px world.
	 * phaseIdx 1 scrolls right (entities move left); phaseIdx 0 scrolls left.
	 */
	void shiftRunnersForScroll(int16 phaseIdx);

	/**
	 * IDA town_advanceLayerFrameState(scrollCol): advances the parallax
	 * background layer frame indices to match the saved scroll column.
	 * Without this, mid-scroll save/load shows the layers at frame 0.
	 */
	void advanceLayerFrameState(uint16 scrollCol);
	void scrollTownLeft();
	void scrollTownRight();

	enum PageResourceId : uint16 {
		kResBackground1200 = 1200,

		kResBitmapShape2000_Cursors = 2000,
		kResBitmapShape1100 = 1100,

		kResScrb1000_Overlay = 1000,
		kResScrb1001_Overlay = 1001,
		kResScrb1002_Overlay = 1002,
		kResScrb1003_Overlay = 1003,
		kResScrb1004_MemorialCard = 1004,
		kResScrb1005_MemorialCard = 1005,
		kResScrb1006_MemorialCard = 1006,
		kResScrb1007_MemorialCard = 1007,

		kResScrb4000_SubFeature = 4000,
		kResScrb4999_Reject = 4999,
		kResScrb5000_Normal = 5000,
		kResScrb6000_Zodiac = 6000,
		kResScrb8000_SubFeatureBase = 8000,

		kResRegs2000 = 2000,

		kResSound997 = 997,
		kResSound996 = 996,
		kResSound3003_BGM = 3003,
		kResSound3000_BGM = 3000,
		kResSound20086_Voice = 20086,
		kResSound20087_Voice = 20087,
		kResSound20088_Voice = 20088,
		kResSoundBGM29999 = 29999,
		kResSoundBGM20000 = 20000,
	};

	// -----------------------------------------------------------------------
	// Shape indices within TOWN cursor/button resources
	// -----------------------------------------------------------------------
	enum ShapeId : uint16 {
		// IDA regs_loadPairWithShapes(2000): hover cursor shapes.
		kShape2000_ArrowLeft_01 = 1,
		kShape2000_ArrowRight_02 = 2,
		kShape2000_Magnifier_03 = 3,

		// Exit gate scroll buttons (from picker_renderExitGateScrb / picker_renderHotspot_45876F)
		kShape1100_ExitGateLeftNormal_05 = 5,
		kShape1100_ExitGateLeftPressed_06 = 6,
		kShape1000_ExitGateRightNormal_24 = 24,
		kShape1000_ExitGateRightPressed_25 = 25,
	};

	/**
	 * Whether all 625 Zoombinis have been stored in town.
	 */
	bool _allZoombinisInTown = false;

	/**
	 * Number of active pack Zoombinis that were moved to town storage on entry.
	 */
	uint16 _activePackCount = 0;

	/**
	 * Calculated town population density, controls how full the town appears.
	 * Range: 25 ~ 80
	 */
	uint16 _townPopDensity = 0;

	/**
	 * Sound resource ID to play on town entry.
	 */
	ZmbResource _entrySoundRes;

	/**
	 * Whether to play the entry sound immediately.
	 */
	bool _playEntrySoundImmediately = false;

	/**
	 * Number of pending celebration walkers ("fireworks") to spawn.
	 * Set from development level thresholds in loadFeatures().
	 * Decremented each time a walker is spawned.
	 * IDA: town_nPendingFireworks (word_4B7966)
	 */
	int16 _developAnimTimer = 0;

	/**
	 * Pointers to the four overlay SCRB features, saved for sub-feature linking.
	 * Index 0: SCRB 1000, 1: SCRB 1002, 2: SCRB 1003, 3: SCRB 1001
	 */
	ZmbFeature *_overlayFeatures[4] = {nullptr, nullptr, nullptr, nullptr};

	/**
	 * Number of town inhabitant Zoombinis rendered as static background decorations.
	 * Determined by (storedTownCount - 20) / 37, clamped to [0, 16].
	 */
	uint16 _inhabitantCount = 0;

	/**
	 * Stored chunk entry indices for town inhabitants. Up to 16 entries.
	 * Original: word_4B7924[16]
	 */
	int16 _inhabitantStoredIdx[16] = {};

	/**
	 * Walking Zoombini snoid count. Up to 20 can be spawned from stored chunk data.
	 * Original: town_nPlacedWalkerZmbs (word_4B7956)
	 */
	uint16 _walkingZmbCount = 0;

	/**
	 * Snoid IDs for walking Zoombinis. Up to 20 entries.
	 * Original: slides_zmbRunnerIdxArr[] (shared global used by Town for walker runner IDs)
	 * These are the snoid feature IDs registered via loadSnoidFromPack(), with bitmask
	 * changed from TYPE_SNOID to TYPE_TOWN_ENTITY after creation.
	 * Snoid IDs use range 20000+ to avoid collision with inhabitant IDs (0-15).
	 */
	uint16 _walkingZmbSnoidIds[20] = {};

	/**
	 * Town inhabitant position data (16 x,y coordinate pairs).
	 * Source: unk_4A72D0 in the original binary.
	 */
	static const Common::Point kInhabitantPositions[16];

	/**
	 * Town inhabitant SCRB IDs (16 IDs, cycling 4000-4007 twice).
	 * These are SCRB resources (not SCRS) loaded via loadSnoidFromScrb().
	 * Source: unk_4A7310 in the original binary.
	 */
	static const uint16 kInhabitantScrbIds[16];

	/**
	 * Memorial statue feature (SCRB 6000).
	 * IDA 0x4581d9: standalone runner with TYPE_TOWN_ENTITY|LOOP_ANIM,
	 * preRenderMemorialStatue callback.
	 */
	ZmbFeature *_memorialStatueFeature = nullptr;

	/**
	 * Memorial card overlay state (IDA `town_renderMemorialCard @ 0x4595C0`).
	 *   _memorialCardActive — card currently displayed, blocks other clicks.
	 *   _memorialCardSlotIdx — which of the 16 memorial slots is shown.
	 *   _memorialHotspots — 16 click rects on the statue for hit-testing.
	 */
	bool _memorialCardActive = false;
	int16 _memorialCardSlotIdx = -1;
	ZmbFeature *_memorialCardFeature = nullptr;
	uint16 _memorialHotspotCount = 0;
	Common::Rect _memorialHotspots[16] = {};
	int16 _memorialSlotMapping[16] = {};
	uint16 _hoverCursorShapeIdx = ZmbHotspot::kShapeNone;

	/**
	 * Pre-render shape callback for the memorial zodiac statue (SCRB 6000).
	 * Filters visibility based on scroll column and sets dial shapes from system time.
	 * IDA: town_preRenderMemorialStatue (0x458597)
	 */
	void memorialStatue_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);

	/**
	 * Updates the zodiac statue dials from the system clock.
	 * Called every 1800 frames (debounced).
	 * IDA: town_updateDateCachePeriodic (0x457C19)
	 */
	void memorialStatue_updateDials();

	/** Memorial statue hour dial position (0-4). IDA: unk_4A72CE → hours/5 */
	uint8 _statueHourDial = 0;
	/** Memorial statue minute dial position (0-11). IDA: unk_4A72CF → minutes%12 */
	uint8 _statueMinuteDial = 0;
	/** Frame counter of last dial update. IDA: town_statueUpdateState */
	uint32 _statueUpdateTimer = 0;

	// --- Celebration walker state (IDA: town_spawnAmbientWalker @ 0x4599F3) ---

	/**
	 * Active celebration walker features. Up to 3 concurrent walkers.
	 * Created from SCRB 8000-8043 with PLAY_ONCE animation.
	 * IDA: town_activeWalkerHandles[3] (word_4B7942)
	 */
	ZmbFeature *_celebWalkerFeatures[3] = {nullptr, nullptr, nullptr};

	/**
	 * Number of celebration walkers pending removal (animation completed).
	 * IDA: town_nPendingWalkerRemovals (word_4B7964)
	 */
	int16 _nPendingWalkerRemovals = 0;

	/**
	 * Spawn one celebration walker if _developAnimTimer > 0 and a slot is free.
	 * IDA: town_spawnAmbientWalker (0x4599F3)
	 */
	void spawnCelebrationWalker();

	/**
	 * Remove completed celebration walkers whose animation has ended.
	 * IDA: cleanup loop in town_onHoverPerFrame (0x45895D)
	 */
	void cleanupFinishedWalkers();

	// --- Ambient sound cycling state (IDA: town_onHoverPerFrame @ 0x458A05) ---

	/**
	 * Current ambient sound resource ID (raw uint16).
	 * Cycles between music (3000-3002) and voice (20089-20093).
	 * IDA: town_currentAmbientSoundId (word_4B6D3C)
	 */
	int16 _ambientSoundId = 0;

	/**
	 * Whether the current ambient sound has finished playing.
	 * When set, triggers the delay timer. Cleared after delay starts.
	 * IDA: town_bSoundPlaybackDone (word_4B6D40)
	 */
	bool _ambientSoundDone = false;

	/**
	 * First-play flag: when set, next sound selection picks from voice pool.
	 * When clear, next selection advances music track.
	 * Toggled each cycle to alternate voice/music.
	 * IDA: town_bSoundFirstPlayFlag (word_4B6D3E)
	 */
	bool _ambientSoundFirstPlay = false;

	/**
	 * Frame counter when the last ambient sound finished.
	 * IDA: town_soundLastPlayTime (dword_4B6D44)
	 */
	uint32 _ambientSoundLastTime = 0;

	/**
	 * Random delay duration before the next ambient sound (150-300 frames).
	 * IDA: town_soundDelayDuration (word_4B6D48)
	 */
	int16 _ambientSoundDelay = 0;

	/**
	 * Non-repeat random pool state for ambient voice selection.
	 * Pool of 5 entries: [20089, 20090, 20091, 20092, 20093].
	 * IDA: dword_4A7288
	 */
	uint32 _ambientVoicePoolState = 0;

	/**
	 * Ambient voice pool: IDA word_4A727E[5].
	 */
	static const int16 kAmbientVoicePool[5];

	/**
	 * Returns the correct archive kind for a town sound ID.
	 * IDs 1000-19999 are page sounds (TOWN.MHK), others are system (ZOOMBINI.MHK).
	 */
	ZmbArchiveKind getAmbientSoundArchiveKind(int16 id) const;

	/**
	 * Compute route-based music sound ID from maze page flag.
	 * IDA: rodmap_getScrbIdFromRoute (0x4588ED)
	 */
	int16 computeRouteMusicId() const;

	// --- Idle animation state (IDA: town_onHoverPerFrame @ 0x458B40) ---

	/** Idle animation budget: number of celebrations remaining. IDA: word_4B7954 */
	int16 _idleAnimBudget = 0;
	/** Frame counter of last idle anim trigger. IDA: dword_4B7958 */
	uint32 _idleAnimLastFrame = 0;
	/** Idle anim interval in frames. IDA: dword_4B795C (init 120) */
	uint32 _idleAnimInterval = 0;
	/** Non-repeat random pool state for idle anim selection. IDA: dword_4B7960 */
	uint32 _idleAnimPoolState = 0;
};

} // End of namespace Mohawk

#endif
