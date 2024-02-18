#pragma once
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <vector>

namespace wsy {
enum class effect_e : uint8_t {
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
	bool xml_parse = true;
	size_t voice_idx = 0;

	void effect(effect_e fx) {
		_effect = fx;
		_compression = compression_e::none;
		_bit_depth = bit_depth_e::_16;
		_sampling_rate = sampling_rate_e::_44;
	}
	effect_e effect() const {
		return _effect;
	}

	void compression(compression_e comp) {
		assert(comp != compression_e::count);
		_effect = effect_e::count;
		_compression = comp;
	}
	compression_e compression() const {
		return _compression;
	}

	void bit_depth(bit_depth_e bd) {
		assert(bd != bit_depth_e::count);
		_effect = effect_e::count;
		_bit_depth = bd;
	}
	bit_depth_e bit_depth() const {
		return _bit_depth;
	}

	void sampling_rate(sampling_rate_e sr) {
		assert(sr != sampling_rate_e::count);
		_effect = effect_e::count;
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
	// friend struct engine;
	effect_e _effect = effect_e::count; // if set, supersedes audio settings.
	compression_e _compression = compression_e::none;
	bit_depth_e _bit_depth = bit_depth_e::_16;
	sampling_rate_e _sampling_rate = sampling_rate_e::_44;

	std::vector<voice_output> _outputs;
};
} // namespace wsy
