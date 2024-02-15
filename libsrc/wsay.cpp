#include "wsay/wsay.hpp"

#define _ATL_APARTMENT_THREADED
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <sapi.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#include <sphelper.h>
#pragma warning(pop)

#include <algorithm>
#include <fea/string/conversions.hpp>
#include <functional>

namespace wsy {
struct voice_data {
	// Creates default voice.
	voice_data(ISpObjectToken* selected_voice, uint16_t vol, int speed) {
		if (!SUCCEEDED(voice_ptr.CoCreateInstance(CLSID_SpVoice))) {
			throw std::runtime_error{ "Couldn't initialize voice." };
		}

		voice_ptr->SetVoice(selected_voice);
		voice_ptr->SetVolume(vol);
		voice_ptr->SetRate(speed);
	}

	// Creates file out voice.
	voice_data(ISpObjectToken* selected_voice,
			const std::filesystem::path& filepath, uint16_t vol, int speed)
			: voice_data(selected_voice, vol, speed) {
		path = filepath;

		CComPtr<ISpStream> cpStream;
		CSpStreamFormat cAudioFmt;
		if (!SUCCEEDED(cAudioFmt.AssignFormat(SPSF_44kHz16BitMono))) {
			throw std::runtime_error{
				"Couldn't set audio format (44kHz, 16bit, mono)."
			};
		}

		std::wstring fileout = path.wstring();

		if (!SUCCEEDED(SPBindToFile(fileout.c_str(), SPFM_CREATE_ALWAYS,
					&cpStream, &cAudioFmt.FormatId(),
					cAudioFmt.WaveFormatExPtr()))) {
			throw std::runtime_error{ "Couldn't bind stream to file." };
		}

		if (!SUCCEEDED(voice_ptr->SetOutput(cpStream, TRUE))) {
			throw std::runtime_error{ "Couldn't set voice output." };
		}
	}

	// voice_data(ISpObjectToken* selected_voice, size_t device_idx,
	//		ISpObjectToken* device, uint16_t vol, int speed)
	//		: voice_data(selected_voice, vol, speed) {

	//	device_playback_idx = device_idx;
	//	voice_ptr->SetOutput(device, TRUE);
	//}

	bool is_default() const {
		return device_playback_idx == std::numeric_limits<size_t>::max()
				&& path.empty();
	}

	CComPtr<ISpVoice> voice_ptr;
	size_t device_playback_idx = std::numeric_limits<size_t>::max();
	std::filesystem::path path;
};

struct voice_impl {
	voice_impl() {
		// THROW_IF_FAILED_MSG(::CoInitializeEx(nullptr, COINIT_MULTITHREADED),
		//		"Couldn't initialize COM.\n");

		// available_voices = get_voices(SPCAT_VOICES);
		//{
		//	std::vector<std::pair<std::wstring, CComPtr<ISpObjectToken>>>
		//			extra_voices = get_voices(win10_extravoices_regkey.c_str());
		//	available_voices.insert(available_voices.end(),
		//			extra_voices.begin(), extra_voices.end());
		// }

		// available_devices = get_playback_devices();

		// voices.push_back(voice_data{ nullptr, 100u, 0 });
	}

	template <class Func>
	void execute(Func func) {
		if (voices.size() == 1) {
			std::invoke(func, voices.back().voice_ptr);
			return;
		}

		for (voice_data& v : voices) {
			if (v.is_default()) {
				continue;
			}
			std::invoke(func, v.voice_ptr);
		}
	}

	std::vector<std::pair<std::wstring, CComPtr<ISpObjectToken>>>
			available_voices;
	std::vector<std::pair<std::wstring, CComPtr<ISpObjectToken>>>
			available_devices;

	std::vector<voice_data> voices;
	size_t selected_voice = 0;
	uint16_t volume = 100;
	int speed = 0;
};


voice::voice()
		: _impl(std::make_unique<voice_impl>()) {
}
voice::~voice() = default;

voice::voice(const voice& other)
		: _impl(std::make_unique<voice_impl>(*other._impl)) {
}
voice::voice(voice&& other) noexcept
		: _impl(std::make_unique<voice_impl>(std::move(*other._impl))) {
}

voice& voice::operator=(const voice& other) {
	if (this == &other) {
		return *this;
	}

	*_impl = *other._impl;
	return *this;
}
voice& voice::operator=(voice&& other) noexcept {
	if (this == &other) {
		return *this;
	}

	*_impl = std::move(*other._impl);
	return *this;
}

size_t voice::available_voices_size() const {
	return _impl->available_voices.size();
}

std::vector<std::wstring> voice::available_voices() const {
	std::vector<std::wstring> ret;
	ret.reserve(_impl->available_voices.size());

	for (const std::pair<std::wstring, CComPtr<ISpObjectToken>>& v :
			_impl->available_voices) {
		ret.push_back(v.first);
	}
	return ret;
}

void voice::select_voice(size_t voice_idx) {
	if (voice_idx >= _impl->available_voices.size()) {
		throw std::out_of_range{ "voice : Selected voice index out-of-range." };
	}

	_impl->selected_voice = voice_idx;

	stop_speaking();

	for (voice_data& v : _impl->voices) {
		v.voice_ptr->SetVoice(
				_impl->available_voices[_impl->selected_voice].second);
	}
}


size_t voice::available_devices_size() const {
	return _impl->available_devices.size();
}

std::vector<std::wstring> voice::available_devices() const {
	std::vector<std::wstring> ret;
	ret.reserve(_impl->available_devices.size());

	for (const std::pair<std::wstring, CComPtr<ISpObjectToken>>& d :
			_impl->available_devices) {
		ret.push_back(d.first);
	}
	return ret;
}

void voice::enable_device_playback(size_t device_idx) {
	if (device_idx >= _impl->available_devices.size()) {
		throw std::out_of_range{
			"voice : Selected device index out-of-range."
		};
	}

	stop_speaking();

	_impl->voices.push_back(voice_data{
			_impl->available_voices[_impl->selected_voice].second,
			device_idx,
			_impl->available_devices[device_idx].second,
			_impl->volume,
			_impl->speed,
	});
}

void voice::disable_device_playback(size_t device_idx) {
	if (device_idx >= _impl->available_devices.size()) {
		throw std::out_of_range{
			"voice : Selected device index out-of-range."
		};
	}

	auto it = std::find_if(_impl->voices.begin(), _impl->voices.end(),
			[&](const voice_data& v) {
				return v.device_playback_idx == device_idx;
			});

	if (it == _impl->voices.end()) {
		return;
	}

	it->voice_ptr->Speak(
			L"", SPF_DEFAULT | SPF_PURGEBEFORESPEAK | SPF_ASYNC, nullptr);
	_impl->voices.erase(it);
}

void voice::start_file_output(const std::filesystem::path& path) {
	if (path.empty()) {
		throw std::invalid_argument{ "voice : Ouput filepath is empty." };
	}

	_impl->voices.push_back(voice_data{
			_impl->available_voices[_impl->selected_voice].second,
			path,
			_impl->volume,
			_impl->speed,
	});
}

void voice::stop_file_output() {
	auto it = std::find_if(_impl->voices.begin(), _impl->voices.end(),
			[&](const voice_data& v) { return !v.path.empty(); });

	if (it == _impl->voices.end()) {
		return;
	}

	it->voice_ptr->Speak(
			L"", SPF_DEFAULT | SPF_PURGEBEFORESPEAK | SPF_ASYNC, nullptr);

	_impl->voices.erase(it);
}

void voice::set_volume(uint16_t volume) {
	volume = std::clamp(volume, uint16_t(0), uint16_t(100));

	_impl->volume = volume;

	for (voice_data& v : _impl->voices) {
		v.voice_ptr->SetVolume(_impl->volume);
	}
}

void voice::set_speed(uint16_t speed) {
	speed = std::clamp(speed, uint16_t(0), uint16_t(100));
	double temp = (speed / 100.0) * 20.0;

	_impl->speed = int(temp) - 10;

	for (voice_data& v : _impl->voices) {
		v.voice_ptr->SetRate(_impl->speed);
	}
}

void voice::speak(const std::wstring& sentence) {
	_impl->execute([&](CComPtr<ISpVoice>& voice) {
		voice->Speak(sentence.c_str(), SPF_DEFAULT | SPF_ASYNC, nullptr);
	});

	_impl->execute(
			[&](CComPtr<ISpVoice>& voice) { voice->WaitUntilDone(INFINITE); });
}

void voice::speak_async(const std::wstring& sentence) {
	_impl->execute([&](CComPtr<ISpVoice>& voice) {
		voice->Speak(sentence.c_str(), SPF_DEFAULT | SPF_ASYNC, nullptr);
	});
}

void voice::stop_speaking() {
	for (voice_data& v : _impl->voices) {
		v.voice_ptr->Speak(
				L"", SPF_DEFAULT | SPF_PURGEBEFORESPEAK | SPF_ASYNC, nullptr);
	}
}

} // namespace wsy
