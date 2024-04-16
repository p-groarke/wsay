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
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <vector>

namespace wsay {
enum class radio_preset_e : uint8_t {
	radio1,
	radio2,
	radio3,
	radio4,
	radio5,
	radio6,
	count,
};

enum class compression_e : uint8_t {
	none,
	alaw,
	ulaw,
	adpcm,
	gsm610,
	count,
};

enum class bit_depth_e : uint8_t {
	_8,
	_16,
	count,
};

enum class sampling_rate_e : uint8_t {
	_8,
	_11,
	_22,
	_44,
	count,
};

enum class output_type_e : uint8_t {
	device,
	file,
	count,
};

struct voice_output {
	output_type_e type = output_type_e::count;
	size_t device_idx = (std::numeric_limits<size_t>::max)();
	std::filesystem::path file_path;
};

struct voice {
	uint8_t volume = 100; // 0-100
	uint8_t speed = 50; // 0-100
	uint8_t pitch = 10; // 0-20
	bool xml_parse = true;
	bool radio_effect_disable_whitenoise = false;
	uint16_t paragraph_pause_ms = (std::numeric_limits<uint16_t>::max)();
	size_t voice_idx = 0;

	void radio_effect(radio_preset_e fx) {
		_radio_effect = fx;
		_compression = compression_e::none;
		_bit_depth = bit_depth_e::_16;
		_sampling_rate = sampling_rate_e::_44;
	}
	radio_preset_e radio_effect() const {
		return _radio_effect;
	}

	void compression(compression_e comp) {
		assert(comp != compression_e::count);
		_radio_effect = radio_preset_e::count;
		_compression = comp;
	}
	compression_e compression() const {
		return _compression;
	}

	void bit_depth(bit_depth_e bd) {
		assert(bd != bit_depth_e::count);
		_radio_effect = radio_preset_e::count;
		_bit_depth = bd;
	}
	bit_depth_e bit_depth() const {
		return _bit_depth;
	}

	void sampling_rate(sampling_rate_e sr) {
		assert(sr != sampling_rate_e::count);
		_radio_effect = radio_preset_e::count;
		_sampling_rate = sr;
	}
	sampling_rate_e sampling_rate() const {
		return _sampling_rate;
	}

	void add_output_device(size_t dev_idx) {
		_outputs.push_back(voice_output{
				.type = output_type_e::device,
				.device_idx = dev_idx,
		});
	}

	void add_output_file(const std::filesystem::path& f) {
		_outputs.push_back(voice_output{
				.type = output_type_e::file,
				.device_idx = (std::numeric_limits<size_t>::max)(),
				.file_path = f,
		});
	}

	const std::vector<voice_output>& outputs() const {
		return _outputs;
	}

private:
	// If set, supersedes audio settings.
	radio_preset_e _radio_effect = radio_preset_e::count;

	// Audio settings.
	compression_e _compression = compression_e::none;
	bit_depth_e _bit_depth = bit_depth_e::_16;
	sampling_rate_e _sampling_rate = sampling_rate_e::_44;

	// Audio outputs.
	std::vector<voice_output> _outputs;
};

inline constexpr size_t radio_preset_count() {
	return size_t(radio_preset_e::count);
}

} // namespace wsay
