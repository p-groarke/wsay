#include "wsay/engine.hpp"
#include "wsay/voice.hpp"

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
#include <cassert>
#include <chrono>
#include <fea/enum/enum_array.hpp>
#include <fea/numerics/literals.hpp>
#include <fea/utils/error.hpp>
#include <fea/utils/throw.hpp>
#include <string_view>
#include <thread>
#include <wil/resource.h>
#include <wil/result.h>


namespace wsy {
using namespace fea::literals;

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


std::vector<CComPtr<ISpObjectToken>> make_voice_tokens() {
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

std::vector<CComPtr<ISpObjectToken>> make_device_tokens() {
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

SPSTREAMFORMAT to_spstreamformat(const voice& vopts) {
	// All mono.
	using sampling_arr_t = fea::enum_array<SPSTREAMFORMAT, sampling_rate_e>;
	using bit_arr_t = fea::enum_array<sampling_arr_t, bit_depth_e>;
	using compression_arr_t = fea::enum_array<bit_arr_t, compression_e>;
	constexpr compression_arr_t map{
		// none
		bit_arr_t{
				// 8bit
				sampling_arr_t{
						SPSF_8kHz8BitMono,
						SPSF_11kHz8BitMono,
						SPSF_22kHz8BitMono,
						SPSF_44kHz8BitMono,
				},
				// 16bit
				sampling_arr_t{
						SPSF_8kHz16BitMono,
						SPSF_11kHz16BitMono,
						SPSF_22kHz16BitMono,
						SPSF_44kHz16BitMono,
				},
		},
		// alaw
		bit_arr_t{
				// 8bit (ignored)
				sampling_arr_t{
						SPSF_CCITT_ALaw_8kHzMono,
						SPSF_CCITT_ALaw_11kHzMono,
						SPSF_CCITT_ALaw_22kHzMono,
						SPSF_CCITT_ALaw_44kHzMono,
				},
				// 16bit (ignored)
				sampling_arr_t{
						SPSF_CCITT_ALaw_8kHzMono,
						SPSF_CCITT_ALaw_11kHzMono,
						SPSF_CCITT_ALaw_22kHzMono,
						SPSF_CCITT_ALaw_44kHzMono,
				},
		},
		// ulaw
		bit_arr_t{
				// 8bit (ignored)
				sampling_arr_t{
						SPSF_CCITT_uLaw_8kHzMono,
						SPSF_CCITT_uLaw_11kHzMono,
						SPSF_CCITT_uLaw_22kHzMono,
						SPSF_CCITT_uLaw_44kHzMono,
				},
				// 16bit (ignored)
				sampling_arr_t{
						SPSF_CCITT_uLaw_8kHzMono,
						SPSF_CCITT_uLaw_11kHzMono,
						SPSF_CCITT_uLaw_22kHzMono,
						SPSF_CCITT_uLaw_44kHzMono,
				},
		},
		// adpcm
		bit_arr_t{
				// 8bit (ignored)
				sampling_arr_t{
						SPSF_ADPCM_8kHzMono,
						SPSF_ADPCM_11kHzMono,
						SPSF_ADPCM_22kHzMono,
						SPSF_ADPCM_44kHzMono,
				},
				// 16bit (ignored)
				sampling_arr_t{
						SPSF_ADPCM_8kHzMono,
						SPSF_ADPCM_11kHzMono,
						SPSF_ADPCM_22kHzMono,
						SPSF_ADPCM_44kHzMono,
				},
		},
		// gsm610
		bit_arr_t{
				// 8bit (ignored)
				sampling_arr_t{
						SPSF_GSM610_8kHzMono,
						SPSF_GSM610_11kHzMono,
						SPSF_GSM610_22kHzMono,
						SPSF_GSM610_44kHzMono,
				},
				// 16bit (ignored)
				sampling_arr_t{
						SPSF_GSM610_8kHzMono,
						SPSF_GSM610_11kHzMono,
						SPSF_GSM610_22kHzMono,
						SPSF_GSM610_44kHzMono,
				},
		},
	};

	return map.at(vopts.compression)
			.at(vopts.bit_depth)
			.at(vopts.sampling_rate);
}

struct tts_voice {
	auto* operator->() {
		return voice.operator->();
	}

	unsigned long flags = 0;
	CComPtr<IStream> data_stream{};
	CComPtr<ISpStream> sp_stream{};
	CComPtr<ISpVoice> voice{};
};

tts_voice make_tts_voice(const voice& vopts, ISpObjectToken* voice_token) {
	tts_voice ret{};

	// Create underlying data stream.
	{
		if (!SUCCEEDED(
					CreateStreamOnHGlobal(nullptr, true, &ret.data_stream))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't create data stream.");
		}
	}

	// Create sp stream which uses backing istream.
	{
		SPSTREAMFORMAT fmt_e = to_spstreamformat(vopts);
		CSpStreamFormat audio_fmt;
		if (!SUCCEEDED(audio_fmt.AssignFormat(fmt_e))) {
			fea::maybe_throw<std::runtime_error>(__FUNCTION__, __LINE__,
					"Couldn't set audio format on tts stream.");
		}
		if (!SUCCEEDED(ret.sp_stream.CoCreateInstance(CLSID_SpStream))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't create tts sp stream.");
		}
		if (!SUCCEEDED(ret.sp_stream->SetBaseStream(ret.data_stream,
					audio_fmt.FormatId(), audio_fmt.WaveFormatExPtr()))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't set tts base stream.");
		}
	}

	// Create the tts voice.
	{
		// if (!SUCCEEDED(SpCreateObjectFromToken(vtoken, &ret.voice))) {
		//	fea::maybe_throw<std::runtime_error>(
		//			__FUNCTION__, __LINE__, "Couldn't create tts voice.");
		// }
		if (!SUCCEEDED(ret.voice.CoCreateInstance(CLSID_SpVoice))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't create tts voice.");
		}
		if (!SUCCEEDED(ret.voice->SetVoice(voice_token))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't set voice token.");
		}

		uint16_t vol = std::clamp(vopts.volume, 0_u8, 100_u8);
		if (!SUCCEEDED(ret.voice->SetVolume(vol))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't set tts voice volume.");
		}

		long speed = std::clamp(int(vopts.speed), 0, 100);
		double speed_temp = (speed / 100.0) * 20.0;
		speed = long(speed_temp) - 10;
		if (!SUCCEEDED(ret.voice->SetRate(speed))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't set tts voice rate.");
		}
		if (!SUCCEEDED(ret.voice->SetOutput(ret.sp_stream, false))) {
			fea::maybe_throw<std::runtime_error>(__FUNCTION__, __LINE__,
					"Couldn't set tts voice sp stream.");
		}
	}

	// Add flags that are driven by voice options.
	{
		if (vopts.xml_parse) {
			ret.flags |= SPF_IS_XML;
		}
	}

	return ret;
}

struct out_voice {
	auto* operator->() {
		return voice.operator->();
	}

	CComPtr<ISpStream> sp_stream{};
	CComPtr<ISpMMSysAudio> sys_audio;
	CComPtr<ISpVoice> voice{};
};

out_voice make_output_voice(
		const voice_output& vout, ISpObjectToken* device_token) {
	out_voice ret{};

	switch (vout.type) {
	case output_type_e::device: {
		assert(vout.device_idx != (std::numeric_limits<size_t>::max)());
		assert(vout.file_path.empty());

		if (!SUCCEEDED(SpCreateObjectFromToken(device_token, &ret.sys_audio))) {
			fea::maybe_throw<std::runtime_error>(__FUNCTION__, __LINE__,
					"Couldn't create audio out from token.");
		}

		// Use default format?

		// CSpStreamFormat audio_fmt;
		// if (!SUCCEEDED(audio_fmt.AssignFormat(SPSF_44kHz16BitMono))) {
		//	fea::maybe_throw<std::runtime_error>(__FUNCTION__, __LINE__,
		//			"Couldn't set audio format on stream.");
		// }

		// if (!SUCCEEDED(audio_out->SetFormat(
		//			audio_fmt.FormatId(), audio_fmt.WaveFormatExPtr()))) {
		//	fea::maybe_throw<std::runtime_error>(
		//			__FUNCTION__, __LINE__, "Couldn't set audio out format.");
		// }

		if (!SUCCEEDED(ret.voice.CoCreateInstance(CLSID_SpVoice))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't create output voice.");
		}
		if (!SUCCEEDED(ret.voice->SetOutput(ret.sys_audio, false))) {
			fea::maybe_throw<std::runtime_error>(__FUNCTION__, __LINE__,
					"Couldn't set output voice audio out.");
		}

	} break;
	case output_type_e::file: {
	} break;
	default: {
		assert(false);
	} break;
	}

	return ret;
}

} // namespace


struct engine_imp {
	engine_imp()
			: voice_tokens(make_voice_tokens())
			, voice_names(make_names(voice_tokens))
			, device_tokens(make_device_tokens())
			, device_names(make_names(device_tokens)) {
	}

	const std::vector<CComPtr<ISpObjectToken>> voice_tokens;
	const std::vector<std::wstring> voice_names;

	const std::vector<CComPtr<ISpObjectToken>> device_tokens;
	const std::vector<std::wstring> device_names;
};


engine::engine() = default;
engine::~engine() = default;
// engine& engine::operator=(engine&&) = default;
// engine& engine::operator=(const engine&) = default;
// engine::engine(engine&&) = default;
// engine::engine(const engine&) = default;


const std::vector<std::wstring>& engine::voices() const {
	return imp().voice_names;
}

const std::vector<std::wstring>& engine::devices() const {
	return imp().device_names;
}

// https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee431811(v=vs.85)
void engine::speak(const voice& vopts, const std::wstring& sentence) {
	// Error checking.
	if (vopts.voice_idx >= imp().voice_tokens.size()) {
		fea::maybe_throw<std::invalid_argument>(
				__FUNCTION__, __LINE__, "Invalid voice index.");
	}

	for (const voice_output& vout : vopts.outputs) {
		if (vout.type == output_type_e::device
				&& vout.device_idx >= imp().device_tokens.size()) {
			fea::maybe_throw<std::invalid_argument>(
					__FUNCTION__, __LINE__, "Invalid device index.");
		}

		if (vout.type == output_type_e::file && vout.file_path.empty()) {
			fea::maybe_throw<std::invalid_argument>(
					__FUNCTION__, __LINE__, "Empty file path.");
		}
	}

	// The voice that will do the tts, outputs to ispstream.
	// Other voices will play stream to various outputs.
	tts_voice tts_v
			= make_tts_voice(vopts, imp().voice_tokens.at(vopts.voice_idx));

	unsigned long flags
			= SPF_DEFAULT | SPF_PURGEBEFORESPEAK | SPF_ASYNC | tts_v.flags;

	if (!SUCCEEDED(tts_v->Speak(sentence.c_str(), flags, nullptr))) {
		fea::maybe_throw<std::invalid_argument>(
				__FUNCTION__, __LINE__, "Tts voice couldn't speak.");
	}

	// tts_v->WaitUntilDone(INFINITE);
	//   voice->SetOutput(nullptr, false);
	//   voice->SpeakStream(sp_stream, SPF_DEFAULT, nullptr);
	std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });


	// std::wstring fileout = path.wstring();

	// if (!SUCCEEDED(SPBindToFile(fileout.c_str(), SPFM_CREATE_ALWAYS,
	// &cpStream, 			&cAudioFmt.FormatId(),
	// cAudioFmt.WaveFormatExPtr()))) { 	throw
	// std::runtime_error{ "Couldn't bind stream to file." };
	// }

	std::vector<out_voice> outputs;
	for (const voice_output& vout : vopts.outputs) {
		outputs.push_back(make_output_voice(
				vout, imp().device_tokens.at(vout.device_idx)));
	}

	for (out_voice& outv : outputs) {
		if (!SUCCEEDED(outv->SpeakStream(tts_v.sp_stream, flags, nullptr))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't speak output stream.");
		}
	}

	tts_v->WaitUntilDone(INFINITE);
	for (out_voice& outv : outputs) {
		outv->WaitUntilDone(INFINITE);
	}
}

const engine_imp& engine::imp() const {
	return *_impl;
}
engine_imp& engine::imp() {
	return *_impl;
}

} // namespace wsy
