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
#include "private_include/com.hpp"
#include "wsay/voice.hpp"

#include <fea/enum/enum_array.hpp>
#include <vector>
#include <wil/resource.h>
#include <wil/result.h>

namespace wsay {
enum class biquad_type_e : uint8_t {
	lowpass,
	highpass,
	bandbass,
	notch,
	count,
};

struct biquad_args {
	biquad_type_e type = biquad_type_e::count;
	float freq = 0.5f;
	float q = 0.707f;
	// float gain = 0.f;
};

struct fx_args {
	size_t bit_depth;
	sampling_rate_e sampling_rate;
	float dist_drive;
	float noise_vol;
	bool noise_after_bitcrush;
	biquad_args biquad;
	float gain;
};

inline constexpr fea::enum_array<fx_args, radio_preset_e> radio_presets{
	// radio 1
	fx_args{
			.bit_depth = 5,
			.sampling_rate = sampling_rate_e::_8,
			.dist_drive = 0.2f,
			.noise_vol = 0.00001f,
			.noise_after_bitcrush = false,
			.biquad = biquad_args{
				.type = biquad_type_e::bandbass,
				.freq = 0.2f,
			},
			.gain = 1.3f,
	},
	// radio 2
	fx_args{
			.bit_depth = 6,
			.sampling_rate = sampling_rate_e::_8,
			.dist_drive = 0.f,
			.noise_vol = 0.01f,
			.noise_after_bitcrush = false,
			.biquad = biquad_args{
				.type = biquad_type_e::highpass,
				.freq = 0.1f,
				.q = 1.f,
			},
			.gain = 1.3f,
	},
	// radio 3
	fx_args{
			.bit_depth = 16,
			.sampling_rate = sampling_rate_e::_44,
			.dist_drive = 1.f,
			.noise_vol = 0.01f,
			.noise_after_bitcrush = false,
			.biquad = biquad_args{
				.type = biquad_type_e::bandbass,
				.freq = 0.05f,
				.q = 1.f,
			},
			.gain = 2.f,
	},
	// radio 4
	fx_args{
			.bit_depth = 16,
			.sampling_rate = sampling_rate_e::_22,
			.dist_drive = 0.9f,
			.noise_vol = 0.001f,
			.noise_after_bitcrush = false,
			.biquad = biquad_args{
				.type = biquad_type_e::lowpass,
				.freq = 0.05f,
				.q = 2.f,
			},
			.gain = 1.f,
	},
	// radio 5
	fx_args{
			.bit_depth = 3,
			.sampling_rate = sampling_rate_e::_8,
			.dist_drive = 0.f,
			.noise_vol = 0.1f,
			.noise_after_bitcrush = true,
			.biquad = biquad_args{
				.type = biquad_type_e::notch,
				.freq = 0.02f,
				.q = 0.5f,
			},
			.gain = 0.7f,
	},
	// radio 6
	fx_args{
			.bit_depth = 4,
			.sampling_rate = sampling_rate_e::_44,
			.dist_drive = 0.f,
			.noise_vol = 0.f,
			.noise_after_bitcrush = true,
			.biquad = biquad_args{
				.type = biquad_type_e::bandbass,
				.freq = 0.04f,
				.q = 0.5f,
			},
			.gain = 1.f,
	},
};


// Processes audio according to the vopts options.
// Provide byte and sample buffers, they will be reused to minimize allocations.
extern void process_fx(const voice& vopts, CComPtr<IStream>& stream,
		std::vector<std::byte>& byte_buffer, std::vector<float>& sample_buffer);
} // namespace wsay