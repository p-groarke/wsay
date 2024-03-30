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

	// Format speak sentence according to input vopts.
	std::wstring format_sentence(const std::wstring& in) const;

	// Option flags.
	unsigned long flags = 0;
	// Option format string, if applicable.
	std::wstring fmt{};
	// Our backing data stream (bytes).
	CComPtr<IStream> data_stream{};
	// Points to data stream.
	CComPtr<ISpStream> sp_stream{};
	// The initialized voice.
	CComPtr<ISpVoice> voice{};
};

struct device_output {
	auto* operator->() {
		return voice.operator->();
	}

	// Clone of tts_voice data_stream.
	CComPtr<IStream> data_stream_clone{};
	// Clone of tts_voice sp_stream.
	CComPtr<ISpStream> sp_stream_clone{};
	// The file output stream.
	CComPtr<ISpStream> file_stream{};
	// The audio output format.
	CComPtr<ISpMMSysAudio> sys_audio;
	// Our voice, instantiated with sys_audio format.
	CComPtr<ISpVoice> voice{};
};

// Convert vopts enums to windows stream format.
extern SPSTREAMFORMAT to_spstreamformat(compression_e compression,
		bit_depth_e bit_depth, sampling_rate_e sampling_rate);
extern SPSTREAMFORMAT to_spstreamformat(const voice& vopts);

// Creates all voice tokens found on PC.
extern std::vector<CComPtr<ISpObjectToken>> make_voice_tokens();

// Creates all device output tokens found on PC.
extern std::vector<CComPtr<ISpObjectToken>> make_device_tokens();

// Creates a matching list of names for given tokens.
extern std::vector<std::wstring> make_names(
		const std::vector<CComPtr<ISpObjectToken>>& ptrs);

// Creates a tts_voice according to vopts options.
extern tts_voice make_tts_voice(const voice& vopts,
		const std::vector<CComPtr<ISpObjectToken>>& voice_tokens);

// Creates a device_out according to vout options.
extern device_output make_device_output(const voice_output& vout,
		const std::vector<CComPtr<ISpObjectToken>>& device_tokens);

// Creates everything needed for speaking.
extern void make_everything(
		const std::vector<CComPtr<ISpObjectToken>>& voice_tokens,
		const std::vector<CComPtr<ISpObjectToken>>& device_tokens,
		const std::vector<std::wstring>& device_names, const voice& vopts,
		tts_voice& tts, std::vector<device_output>& device_outputs);

// Clones input tts stream into device output streams.
// Clones have same bytes but independent playhead.
extern void clone_input_stream(
		const tts_voice& tts, std::vector<device_output>& device_outputs);
} // namespace wsy
