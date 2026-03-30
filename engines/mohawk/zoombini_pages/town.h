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

#ifndef MOHAWK_ZOOMBINI_PAGES_TOWN_H
#define MOHAWK_ZOOMBINI_PAGES_TOWN_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

class ZoombiniInteractiveTown : public ZoombiniInteractive {
public:
	ZoombiniInteractiveTown(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveTown() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	// Callback methods for virtual town Zoombini render feature
	ZmbRenderResult townZoombini_render(ZmbFeature *feature);
	void townZoombini_postRender(ZmbFeature *feature);

	// Pre-render shape callback for overlay features (SCRB 1002, 1003, 1001)
	void overlay_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);

	/**
	 * Transfer active pack Zoombinis into town stored chunk entries.
	 * Each Zoombini is stored at the index matching its snoidId().
	 */
	void transferActivePackToTownStorage();

	enum PageResourceId : uint16 {
		kResBackground1200 = 1200,

		kResBitmapShape1100 = 1100,

		kResScrb1000_Overlay = 1000,
		kResScrb1001_Overlay = 1001,
		kResScrb1002_Overlay = 1002,
		kResScrb1003_Overlay = 1003,

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

		kVirtualFeatureTownZoombini = 9000,
	};

	// -----------------------------------------------------------------------
	// Shape indices within SHPL 1100 (town overlays and buttons)
	// -----------------------------------------------------------------------
	enum ShapeId : uint16 {
		// Exit gate scroll buttons (from picker_renderExitGateScrb / picker_renderHotspot_45876F)
		kShape1100_ExitGateLeftNormal_05 = 5,
		kShape1100_ExitGateLeftPressed_06 = 6,
		kShape1100_ExitGateRightNormal_24 = 24,
		kShape1100_ExitGateRightPressed_25 = 25,
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
	 * Town develop animation timer (frames to wait before playing town develop cutscene).
	 * 0 = no develop animation, 10/20/25 = animate after this many frames.
	 */
	uint16 _developAnimTimer = 0;

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
	 * Original: word_4B7956
	 */
	uint16 _walkingZmbCount = 0;

	/**
	 * Stored chunk entry indices for walking Zoombinis. Up to 20 entries.
	 * Original: word_4B6D4C[20]
	 */
	int16 _walkingZmbStoredIdx[20] = {};

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
};

} // End of namespace Mohawk

#endif
