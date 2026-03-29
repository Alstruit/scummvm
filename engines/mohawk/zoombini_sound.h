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

#ifndef MOHAWK_ZOOMBINI_SOUND_H
#define MOHAWK_ZOOMBINI_SOUND_H

#include "common/hashmap.h"
#include "common/queue.h"

#include "mohawk/resource.h"
#include "mohawk/sound.h"

namespace Mohawk {

class MohawkEngine_Zoombini;

class ZoombiniSound : public Sound {
public:
	ZoombiniSound(MohawkEngine_Zoombini *vm);
	~ZoombiniSound() override;

	Audio::SoundHandle *playZmbSound(ZmbResource resource, Audio::Mixer::SoundType soundType = Audio::Mixer::kSFXSoundType, bool loop = false);
	Audio::SoundHandle *playZmbSound(ZmbResource resource, Audio::Mixer::SoundType soundType, byte volume, bool loop);
	void stopZmbSound(ZmbResource resource);

	/**
	 * Opaque handle returned by createSoundQueue().
	 * kInvalidSoundQueueHandle is the null/uninitialised value.
	 */
	typedef uint32 ZmbSoundQueueHandle;
	static constexpr ZmbSoundQueueHandle kInvalidSoundQueueHandle = 0;

	/**
	 * Create a new independent sound queue and return its handle.
	 * Multiple queues run in parallel; each plays its entries sequentially.
	 */
	ZmbSoundQueueHandle createSoundQueue();

	/**
	 * Stop the queue's current sound, discard all pending entries, and
	 * release the handle. The handle must not be used after this call.
	 */
	void deleteSoundQueue(ZmbSoundQueueHandle handle);

	/** Append a sound to the end of the specified queue. */
	void queueZmbSound(ZmbSoundQueueHandle handle, ZmbResource resource, Audio::Mixer::SoundType soundType = Audio::Mixer::kSFXSoundType, bool loop = false);
	void queueZmbSound(ZmbSoundQueueHandle handle, ZmbResource resource, Audio::Mixer::SoundType soundType, byte volume, bool loop = false);

	/** Called every frame to advance all active queues. */
	void updateSoundQueue();

	/** Stop the current sound and discard all pending entries in the queue. */
	void stopSoundQueue(ZmbSoundQueueHandle handle);

	/** Discard pending entries without stopping the currently playing sound. */
	void clearSoundQueue(ZmbSoundQueueHandle handle);

	/** Stop and discard every queue that has been created. */
	void stopAllSoundQueues();

	/** Returns true when the queue is idle (nothing playing, nothing pending). */
	bool isSoundQueueEmpty(ZmbSoundQueueHandle handle) const;

	/** Returns true while the queue has a sound actively playing. */
	bool isSoundQueuePlaying(ZmbSoundQueueHandle handle) const;

private:
	struct SoundQueueEntry {
		ZmbResource resource;
		Audio::Mixer::SoundType soundType;
		byte volume;
		bool loop;
	};

	struct SoundQueueChannel {
		Common::Queue<SoundQueueEntry> queue;
		Audio::SoundHandle currentHandle;
		bool playing = false;
	};

	MohawkEngine_Zoombini *_vm;

	Common::HashMap<uint32, SoundQueueChannel> _soundQueues;
	uint32 _nextQueueHandle = 1; // 0 is kInvalidSoundQueueHandle

	/** Advance a single channel: start next sound if the current one finished. */
	void updateChannel(SoundQueueChannel &ch);

	/** Internal helper: stop and drain a channel without erasing it from the map. */
	void stopChannel(SoundQueueChannel &ch);
};

class ZoombiniMidiPlayer : public MidiPlayer {
public:
	ZoombiniMidiPlayer(MohawkEngine_Zoombini *vm);
	~ZoombiniMidiPlayer() override;

	void playZmbMidi(ZmbResource resource);

private:
	MohawkEngine_Zoombini *_vm;
};

} // End of namespace Mohawk

#endif
