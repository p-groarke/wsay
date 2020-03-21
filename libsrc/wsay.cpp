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
#include <fea_utils/fea_utils.hpp>
#include <wil/resource.h>
#include <wil/result.h>

namespace wsy {
namespace {
using unique_couninitialize_call
		= wil::unique_call<decltype(&::CoUninitialize), ::CoUninitialize>;

std::wstring new_win10_registrykey
		= L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_"
		  L"OneCore\\Voices";


std::vector<std::pair<std::wstring, CComPtr<ISpObjectToken>>> get_voices(
		const wchar_t* registry_key) {

	std::vector<std::pair<std::wstring, CComPtr<ISpObjectToken>>> ret;
	CComPtr<IEnumSpObjectTokens> cpEnum;

	if (!SUCCEEDED(SpEnumTokens(registry_key, nullptr, nullptr, &cpEnum))) {
		return ret;
	}

	unsigned long num_voices = 0;
	if (!SUCCEEDED(cpEnum->GetCount(&num_voices))) {
		return ret;
	}

	for (size_t i = 0; i < num_voices; ++i) {
		CComPtr<ISpObjectToken> cpVoiceToken;

		if (!SUCCEEDED(cpEnum->Next(1, &cpVoiceToken, nullptr))) {
			return ret;
		}

		LPWSTR str = nullptr;
		cpVoiceToken->GetStringValue(nullptr, &str);

		ret.push_back({ std::wstring{ str }, std::move(cpVoiceToken) });
	}

	return ret;
}

std::vector<std::pair<std::wstring, CComPtr<ISpObjectToken>>>
get_playback_devices() {

	std::vector<std::pair<std::wstring, CComPtr<ISpObjectToken>>> ret;
	CComPtr<IEnumSpObjectTokens> cpEnum;

	// Enumerate the available audio output devices.
	if (!SUCCEEDED(SpEnumTokens(SPCAT_AUDIOOUT, nullptr, nullptr, &cpEnum))) {
		return ret;
	}

	unsigned long device_count = 0;
	// Get the number of audio output devices.
	if (!SUCCEEDED(cpEnum->GetCount(&device_count))) {
		return ret;
	}

	// Obtain a list of available audio output tokens,
	// set the output to the token, and call Speak.
	for (size_t i = 0; i < device_count; ++i) {
		CComPtr<ISpObjectToken> cpAudioOutToken;

		if (!SUCCEEDED(cpEnum->Next(1, &cpAudioOutToken, nullptr))) {
			return ret;
		}

		LPWSTR str = nullptr;
		cpAudioOutToken->GetStringValue(nullptr, &str);

		ret.push_back({ std::wstring{ str }, std::move(cpAudioOutToken) });
	}

	return ret;
}

CComPtr<ISpVoice> create_voice() {
	CComPtr<ISpVoice> ret;
	if (!SUCCEEDED(ret.CoCreateInstance(CLSID_SpVoice))) {
		fprintf(stderr, "Couldn't initialize voice.\n");
		std::exit(-1);
	}
	return ret;
}

void setup_fileout(CComPtr<ISpVoice>& voice,
		const std::filesystem::path& output_filepath) {
	CComPtr<ISpStream> cpStream;
	CSpStreamFormat cAudioFmt;
	if (!SUCCEEDED(cAudioFmt.AssignFormat(SPSF_44kHz16BitMono))) {
		fprintf(stderr, "Couldn't set audio format (44kHz, 16bit, mono).\n");
		std::exit(-1);
	}

	std::wstring fileout;
	if (!output_filepath.empty()) {
		fileout = output_filepath.wstring();
	} else {
		fileout = (std::filesystem::current_path() / L"out.wav").wstring();
	}

	if (!SUCCEEDED(SPBindToFile(fileout.c_str(), SPFM_CREATE_ALWAYS, &cpStream,
				&cAudioFmt.FormatId(), cAudioFmt.WaveFormatExPtr()))) {
		fprintf(stderr, "Couldn't bind stream to file.\n");
		std::exit(-1);
	}

	if (!SUCCEEDED(voice->SetOutput(cpStream, TRUE))) {
		fprintf(stderr, "Couldn't set voice output.\n");
		std::exit(-1);
	}
}

} // namespace


bool parse_text_file(
		const std::filesystem::path& path, std::wstring& out_text) {

	if (!std::filesystem::exists(path)) {
		fprintf(stderr, "Input text file doesn't exist : '%s'\n",
				path.string().c_str());
		return false;
	}

	if (path.extension() != ".txt") {
		fprintf(stderr,
				"Text option only supports '.txt' "
				"files.\n");
		return false;
	}

	std::vector<std::wstring> sentences;
	if (!fea::wopen_text_file(path, sentences)) {
		return false;
	}

	for (const std::wstring& str : sentences) {
		if (str.empty()) {
			continue;
		}

		out_text += str;
		out_text += '\n';
	}

	if (out_text.empty()) {
		fprintf(stderr,
				"Couldn't parse text file or there is no "
				"text in file.\n");
		return false;
	}

	return true;
}


struct voice_impl {
	voice_impl() {
		available_voices = get_voices(SPCAT_VOICES);
		{
			std::vector<std::pair<std::wstring, CComPtr<ISpObjectToken>>>
					extra_voices = get_voices(new_win10_registrykey.c_str());
			available_voices.insert(available_voices.end(),
					extra_voices.begin(), extra_voices.end());
		}

		available_devices = get_playback_devices();

		voices.push_back(create_voice());
	}

	unique_couninitialize_call cleanup;
	std::vector<std::pair<std::wstring, CComPtr<ISpObjectToken>>>
			available_voices;
	std::vector<std::pair<std::wstring, CComPtr<ISpObjectToken>>>
			available_devices;

	std::vector<CComPtr<ISpVoice>> voices;
	bool contains_default_voice = true;
};


voice::voice()
		: _impl(std::make_unique<voice_impl>()) {

	THROW_IF_FAILED_MSG(::CoInitializeEx(nullptr, COINIT_MULTITHREADED),
			"Couldn't initialize COM.\n");
}
voice::~voice() = default;

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

void voice::add_file_output(const std::filesystem::path& path) {
	if (_impl->contains_default_voice) {
		_impl->voices.clear();
		_impl->contains_default_voice = false;
	}

	_impl->voices.push_back(create_voice());
	setup_fileout(_impl->voices.back(), path);
}

void voice::add_device_playback(size_t device_idx) {
	if (device_idx >= _impl->available_devices.size()) {
		throw std::runtime_error{
			"voice : Selected device index out-of-range."
		};
	}

	if (_impl->contains_default_voice) {
		_impl->voices.clear();
		_impl->contains_default_voice = false;
	}

	_impl->voices.push_back(create_voice());
	_impl->voices.back()->SetOutput(
			_impl->available_devices[device_idx].second, TRUE);
}

void voice::select_voice(size_t voice_idx) {
	if (voice_idx >= _impl->available_voices.size()) {
		throw std::runtime_error{
			"voice : Selected voice index out-of-range."
		};
	}

	for (CComPtr<ISpVoice>& voice : _impl->voices) {
		voice->SetVoice(_impl->available_voices[voice_idx].second);
	}
}

void voice::speak(const std::wstring& sentence) {
	for (CComPtr<ISpVoice>& voice : _impl->voices) {
		voice->Speak(sentence.c_str(), SPF_DEFAULT | SPF_ASYNC, nullptr);
	}

	for (CComPtr<ISpVoice>& voice : _impl->voices) {
		voice->WaitUntilDone(INFINITE);
	}
}

void voice::speak_async(const std::wstring& sentence) {
	for (CComPtr<ISpVoice>& voice : _impl->voices) {
		voice->Speak(sentence.c_str(), SPF_DEFAULT | SPF_ASYNC, nullptr);
	}
}

void voice::stop_speaking() {
	for (CComPtr<ISpVoice>& voice : _impl->voices) {
		voice->Speak(
				L"", SPF_DEFAULT | SPF_PURGEBEFORESPEAK | SPF_ASYNC, nullptr);
	}
}

} // namespace wsy
