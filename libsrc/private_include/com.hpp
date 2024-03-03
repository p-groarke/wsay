/**
 * Copyright (c) 2024, Philippe Groarke
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#include "private_include/com_mmdevice.hpp"
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
#include <iostream>
#include <string_view>
#include <thread>
#include <vector>
#include <wil/resource.h>
#include <wil/result.h>

namespace wsy {
using namespace fea::literals;

constexpr inline compression_e output_compression = compression_e::none;
constexpr inline bit_depth_e output_bit_depth = bit_depth_e::_16;
constexpr inline sampling_rate_e output_sample_rate = sampling_rate_e::_44;

struct coinit {
	coinit() {
		THROW_IF_FAILED_MSG(::CoInitializeEx(nullptr, COINIT_MULTITHREADED),
				"Couldn't initialize COM.\n");
	}
	~coinit() {
		::CoUninitialize();
	}
};
inline coinit _coinit;

struct tts_voice {
	auto* operator->() {
		return voice.operator->();
	}

	unsigned long flags = 0;
	CComPtr<IStream> data_stream{};
	CComPtr<ISpStream> sp_stream{};
	CComPtr<ISpVoice> voice{};
};

struct device_output {
	auto* operator->() {
		return voice.operator->();
	}

	CComPtr<IStream> data_stream_clone{};
	CComPtr<ISpStream> sp_stream_clone{};
	CComPtr<ISpStream> file_stream{};
	CComPtr<ISpMMSysAudio> sys_audio;
	CComPtr<ISpVoice> voice{};
};


SPSTREAMFORMAT to_spstreamformat(compression_e compression,
		bit_depth_e bit_depth, sampling_rate_e sampling_rate) {
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

	return map.at(compression).at(bit_depth).at(sampling_rate);
}
SPSTREAMFORMAT to_spstreamformat(const voice& vopts) {
	return to_spstreamformat(
			vopts.compression(), vopts.bit_depth(), vopts.sampling_rate());
}

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

tts_voice make_tts_voice(const voice& vopts,
		const std::vector<CComPtr<ISpObjectToken>>& voice_tokens) {
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
		if (!SUCCEEDED(ret.voice->SetVoice(voice_tokens.at(vopts.voice_idx)))) {
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
		} else {
			ret.flags |= SPF_IS_NOT_XML;
		}
	}

	return ret;
}

device_output make_device_output(const voice_output& vout,
		const std::vector<CComPtr<ISpObjectToken>>& device_tokens) {
	device_output ret{};

	// All outputs use same prescribed format.
	CSpStreamFormat audio_fmt;
	SPSTREAMFORMAT fmt_e = to_spstreamformat(
			output_compression, output_bit_depth, output_sample_rate);
	if (!SUCCEEDED(audio_fmt.AssignFormat(fmt_e))) {
		fea::maybe_throw<std::runtime_error>(__FUNCTION__, __LINE__,
				"Couldn't set audio format on device output.");
	}

	switch (vout.type) {
	case output_type_e::device: {
		assert(vout.device_idx != (std::numeric_limits<size_t>::max)());
		assert(vout.file_path.empty());

		if (!SUCCEEDED(SpCreateObjectFromToken(
					device_tokens.at(vout.device_idx), &ret.sys_audio))) {
			fea::maybe_throw<std::runtime_error>(__FUNCTION__, __LINE__,
					"Couldn't create audio out from token.");
		}

		if (!SUCCEEDED(ret.sys_audio->SetFormat(
					audio_fmt.FormatId(), audio_fmt.WaveFormatExPtr()))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't set audio out format.");
		}

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
		assert(vout.device_idx == (std::numeric_limits<size_t>::max)());
		assert(!vout.file_path.empty());

		if (!SUCCEEDED(SPBindToFile(vout.file_path.c_str(), SPFM_CREATE_ALWAYS,
					&ret.file_stream, &audio_fmt.FormatId(),
					audio_fmt.WaveFormatExPtr()))) {
			throw std::runtime_error{ "Couldn't bind voice to file." };
		}

		if (!SUCCEEDED(ret.voice.CoCreateInstance(CLSID_SpVoice))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't create output voice.");
		}
		if (!SUCCEEDED(ret.voice->SetOutput(ret.file_stream, false))) {
			fea::maybe_throw<std::runtime_error>(__FUNCTION__, __LINE__,
					"Couldn't set output voice audio out.");
		}
	} break;
	default: {
		fea::maybe_throw<std::runtime_error>(__FUNCTION__, __LINE__,
				"Invalid output type. Please report this bug.");
	} break;
	}

	return ret;
}

void make_everything(const std::vector<CComPtr<ISpObjectToken>>& voice_tokens,
		const std::vector<CComPtr<ISpObjectToken>>& device_tokens,
		const std::vector<std::wstring>& device_names, const voice& vopts,
		tts_voice& tts, std::vector<device_output>& device_outputs) {
	// Error checking.
	if (vopts.voice_idx >= voice_tokens.size()) {
		fea::maybe_throw<std::invalid_argument>(
				__FUNCTION__, __LINE__, "Invalid voice index.");
	}

	for (const voice_output& vout : vopts.outputs()) {
		if (vout.type == output_type_e::device
				&& vout.device_idx >= device_tokens.size()) {
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
	tts = make_tts_voice(vopts, voice_tokens);

	// Create output voices. Either devices or output files.
	for (const voice_output& vout : vopts.outputs()) {
		device_outputs.push_back(make_device_output(vout, device_tokens));
	}

	// Make a default voice if we have no outputs.
	if (device_outputs.empty()) {
		voice_output vout;
		vout.type = output_type_e::device;
		vout.device_idx = default_output_device_idx(device_names);

		device_outputs.push_back(make_device_output(vout, device_tokens));
	}
}

// Clones input tts stream into device output streams.
// Clones have same bytes but independent playhead.
void clone_input_stream(
		const tts_voice& tts, std::vector<device_output>& device_outputs) {
	GUID fmt_guid{};
	wil::unique_cotaskmem_ptr<WAVEFORMATEX> wave_fmt;
	if (!SUCCEEDED(tts.sp_stream->GetFormat(
				&fmt_guid, wil::out_param(wave_fmt)))) {
		fea::maybe_throw<std::runtime_error>(
				__FUNCTION__, __LINE__, "Couldn't get input stream format.");
	}

	for (device_output& outv : device_outputs) {
		if (!SUCCEEDED(tts.data_stream->Clone(&outv.data_stream_clone))) {
			fea::maybe_throw<std::runtime_error>(__FUNCTION__, __LINE__,
					"Couldn't clone input data stream format.");
		}

		if (!SUCCEEDED(outv.sp_stream_clone.CoCreateInstance(CLSID_SpStream))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't create clone sp stream.");
		}

		if (!SUCCEEDED(outv.sp_stream_clone->SetBaseStream(
					outv.data_stream_clone, fmt_guid, wave_fmt.get()))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't set clone base stream.");
		}
	}
}
} // namespace wsy
