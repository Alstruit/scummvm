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

#include "mohawk/resource.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_sound.h"

namespace Mohawk {

ZoombiniSound::ZoombiniSound(MohawkEngine_Zoombini *vm) : _vm(vm), Sound(vm) {
}

ZoombiniSound::~ZoombiniSound() {
}

Audio::SoundHandle *ZoombiniSound::playZmbSound(ZmbResource resource, Audio::Mixer::SoundType soundType, bool loop) {
	// Check SFX mute flag (used by Smoke puzzle during question phase)
	if (_sfxMuted && soundType == Audio::Mixer::kSFXSoundType)
		return nullptr;
	if (_stopMidiOnSfx && soundType == Audio::Mixer::kSFXSoundType)
		_vm->_midi->stopMidi();
	ZmbArchiveKind lastKind = _vm->setActiveResourceKind(resource._archiveKind);
	Audio::SoundHandle *sndHandle = playSound(resource._id, soundType, 255, loop, nullptr);
	_vm->setActiveResourceKind(lastKind);
	return sndHandle;
}

Audio::SoundHandle *ZoombiniSound::playZmbSound(ZmbResource resource, Audio::Mixer::SoundType soundType, byte volume, bool loop) {
	// Check SFX mute flag (used by Smoke puzzle during question phase)
	if (_sfxMuted && soundType == Audio::Mixer::kSFXSoundType)
		return nullptr;
	if (_stopMidiOnSfx && soundType == Audio::Mixer::kSFXSoundType)
		_vm->_midi->stopMidi();
	ZmbArchiveKind lastKind = _vm->setActiveResourceKind(resource._archiveKind);
	Audio::SoundHandle *sndHandle = playSound(resource._id, soundType, volume, loop, nullptr);
	_vm->setActiveResourceKind(lastKind);
	return sndHandle;
}

void ZoombiniSound::stopZmbSound(ZmbResource resource) {
	ZmbArchiveKind lastKind = _vm->setActiveResourceKind(resource._archiveKind);
	stopSound(resource._id);
	_vm->setActiveResourceKind(lastKind);
}

ZoombiniSound::ZmbSoundQueueHandle ZoombiniSound::createSoundQueue() {
	ZmbSoundQueueHandle handle = _nextQueueHandle++;
	_soundQueues[handle] = SoundQueueChannel();
	return handle;
}

void ZoombiniSound::deleteSoundQueue(ZmbSoundQueueHandle handle) {
	auto it = _soundQueues.find(handle);
	if (it == _soundQueues.end())
		return;
	stopChannel(it->_value);
	_soundQueues.erase(it);
}

void ZoombiniSound::queueZmbSound(ZmbSoundQueueHandle handle, ZmbResource resource, Audio::Mixer::SoundType soundType, bool loop) {
	queueZmbSound(handle, resource, soundType, Audio::Mixer::kMaxChannelVolume, loop);
}

void ZoombiniSound::queueZmbSound(ZmbSoundQueueHandle handle, ZmbResource resource, Audio::Mixer::SoundType soundType, byte volume, bool loop) {
	auto it = _soundQueues.find(handle);
	if (it == _soundQueues.end())
		return;
	SoundQueueChannel &ch = it->_value;
	ch.queue.push({resource, soundType, volume, loop});
	// Kick off immediately if the channel is idle.
	if (!ch.playing)
		updateChannel(ch);
}

void ZoombiniSound::updateChannel(SoundQueueChannel &ch) {
	if (ch.playing) {
		// Still playing — nothing to do.
		if (_vm->_system->getMixer()->isSoundHandleActive(ch.currentHandle))
			return;
		ch.playing = false;
	}

	// Start the next pending sound, if any.
	if (!ch.queue.empty()) {
		SoundQueueEntry entry = ch.queue.pop();
		ZmbArchiveKind lastKind = _vm->setActiveResourceKind(entry.resource._archiveKind);
		Audio::SoundHandle *handle = playSound(entry.resource._id, entry.soundType, entry.volume, entry.loop, nullptr);
		if (handle)
			ch.currentHandle = *handle;
		_vm->setActiveResourceKind(lastKind);
		ch.playing = true;
	}
}

void ZoombiniSound::stopChannel(SoundQueueChannel &ch) {
	while (!ch.queue.empty())
		ch.queue.pop();
	if (ch.playing) {
		_vm->_system->getMixer()->stopHandle(ch.currentHandle);
		ch.playing = false;
	}
}

void ZoombiniSound::updateSoundQueue() {
	for (auto &kv : _soundQueues)
		updateChannel(kv._value);
}

void ZoombiniSound::stopSoundQueue(ZmbSoundQueueHandle handle) {
	auto it = _soundQueues.find(handle);
	if (it == _soundQueues.end())
		return;
	stopChannel(it->_value);
}

void ZoombiniSound::clearSoundQueue(ZmbSoundQueueHandle handle) {
	auto it = _soundQueues.find(handle);
	if (it == _soundQueues.end())
		return;
	SoundQueueChannel &ch = it->_value;
	while (!ch.queue.empty())
		ch.queue.pop();
}

void ZoombiniSound::stopAllSoundQueues() {
	for (auto &kv : _soundQueues)
		stopChannel(kv._value);
}

bool ZoombiniSound::isSoundQueueEmpty(ZmbSoundQueueHandle handle) const {
	auto it = _soundQueues.find(handle);
	if (it == _soundQueues.end())
		return true;
	const SoundQueueChannel &ch = it->_value;
	return !ch.playing && ch.queue.empty();
}

bool ZoombiniSound::isSoundQueuePlaying(ZmbSoundQueueHandle handle) const {
	auto it = _soundQueues.find(handle);
	if (it == _soundQueues.end())
		return false;
	return it->_value.playing;
}

ZoombiniMidiPlayer::ZoombiniMidiPlayer(MohawkEngine_Zoombini *vm) : MidiPlayer(vm), _vm(vm) {
}

ZoombiniMidiPlayer::~ZoombiniMidiPlayer() {
}

void ZoombiniMidiPlayer::playZmbMidi(ZmbResource resource) {
	ZmbArchiveKind lastKind = _vm->setActiveResourceKind(resource._archiveKind);
	playMidi(resource._id);
	_vm->setActiveResourceKind(lastKind);
}

} // End of namespace Mohawk

