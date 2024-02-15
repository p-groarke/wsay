#include "wsay/engine.hpp"

#define _ATL_APARTMENT_THREADED
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <sapi.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#include <sphelper.h>
#pragma warning(pop)

#include <string_view>
#include <wil/resource.h>
#include <wil/result.h>


namespace wsy {
namespace {
struct coinit {
	coinit() {
		THROW_IF_FAILED_MSG(::CoInitializeEx(nullptr, COINIT_MULTITHREADED),
				"Couldn't initialize COM.\n");
	}
	~coinit() {
		::CoUninitialize();
	}

	// using couninit_t
	//		= wil::unique_call<decltype(&::CoUninitialize), ::CoUninitialize>;
	// couninit_t cleanup;
};
coinit _coinit;


std::vector<CComPtr<ISpObjectToken>> make_voice_ptrs() {
	constexpr std::wstring_view win10_regkey
			= L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_"
			  L"OneCore\\Voices";

	auto add_voices = [&](const wchar_t* regkey,
							  std::vector<CComPtr<ISpObjectToken>>& ret) {
		CComPtr<IEnumSpObjectTokens> cpEnum;
		if (!SUCCEEDED(SpEnumTokens(regkey, nullptr, nullptr, &cpEnum))) {
			return;
		}

		unsigned long num_voices = 0;
		if (!SUCCEEDED(cpEnum->GetCount(&num_voices))) {
			return;
		}

		for (size_t i = 0; i < num_voices; ++i) {
			CComPtr<ISpObjectToken> cpVoiceToken;
			if (!SUCCEEDED(cpEnum->Next(1, &cpVoiceToken, nullptr))) {
				return;
			}
			ret.push_back(std::move(cpVoiceToken));
		}
	};

	std::vector<CComPtr<ISpObjectToken>> ret;
	add_voices(SPCAT_VOICES, ret);
	add_voices(win10_regkey.data(), ret);
	return ret;
}

std::vector<CComPtr<ISpObjectToken>> make_device_ptrs() {
	std::vector<CComPtr<ISpObjectToken>> ret;
	CComPtr<IEnumSpObjectTokens> cpEnum;

	if (!SUCCEEDED(SpEnumTokens(SPCAT_AUDIOOUT, nullptr, nullptr, &cpEnum))) {
		return ret;
	}

	unsigned long device_count = 0;
	if (!SUCCEEDED(cpEnum->GetCount(&device_count))) {
		return ret;
	}

	for (size_t i = 0; i < device_count; ++i) {
		CComPtr<ISpObjectToken> cpAudioOutToken;
		if (!SUCCEEDED(cpEnum->Next(1, &cpAudioOutToken, nullptr))) {
			return ret;
		}
		ret.push_back(std::move(cpAudioOutToken));
	}
	return ret;
}

std::vector<std::wstring> make_names(
		const std::vector<CComPtr<ISpObjectToken>>& ptrs) {
	std::vector<std::wstring> ret;
	for (const CComPtr<ISpObjectToken>& ptr : ptrs) {
		LPWSTR str = nullptr;
		ptr->GetStringValue(nullptr, &str);
		ret.push_back(std::wstring{ str });
	}
	return ret;
}


} // namespace


struct engine_imp {
	engine_imp()
			: voice_ptrs(make_voice_ptrs())
			, voice_names(make_names(voice_ptrs))
			, device_ptrs(make_device_ptrs())
			, device_names(make_names(device_ptrs)) {
	}

	std::vector<CComPtr<ISpObjectToken>> voice_ptrs;
	std::vector<std::wstring> voice_names;

	std::vector<CComPtr<ISpObjectToken>> device_ptrs;
	std::vector<std::wstring> device_names;
};


engine::engine() = default;
engine::~engine() = default;
engine& wsy::engine::operator=(engine&&) = default;
engine& wsy::engine::operator=(const engine&) = default;
engine::engine(engine&&) = default;
engine::engine(const engine&) = default;


const std::vector<std::wstring>& engine::voices() const {
	return imp().voice_names;
}

const std::vector<std::wstring>& engine::devices() const {
	return imp().device_names;
}

// void engine::speak(const std::wstring& sentence) {
// }

const engine_imp& engine::imp() const {
	return *_impl;
}
engine_imp& engine::imp() {
	return *_impl;
}

} // namespace wsy
