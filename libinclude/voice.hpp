#pragma once
#include <cstdint>
#include <filesystem>
#include <limits>
#include <vector>

namespace wsy {
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

enum class compression_e : uint8_t {
	none,
	alaw,
	ulaw,
	adpcm,
	gsm,
	count,
};

enum class output_type_e : uint8_t {
	device,
	file,
	count,
};

struct output {
	output_type_e type = output_type_e::count;
	size_t device_idx = (std::numeric_limits<size_t>::max)();
	std::filesystem::path file_path;
};

struct voice {
	bit_depth_e bit_depth = bit_depth_e::_16;
	sampling_rate_e sampling_rate = sampling_rate_e::_44;
	compression_e compression = compression_e::none;
	uint8_t volume = 100; // 0-100
	uint8_t speed = 50; // 0-100
	bool xml = true;

	void add_output_device(size_t dev_idx) {
		outputs.push_back(output{
				.type = output_type_e::device,
				.device_idx = dev_idx,
		});
	}

	void add_output_file(const std::filesystem::path& f) {
		outputs.push_back(output{
				.type = output_type_e::file,
				.device_idx = (std::numeric_limits<size_t>::max)(),
				.file_path = f,
		});
	}

private:
	friend struct engine;
	std::vector<output> outputs;
};
} // namespace wsy
