#include "private_include/fx.hpp"

#include <cmath>
#include <cstdint>
#include <fea/enum/enum_array.hpp>
#include <fea/meta/static_for.hpp>
#include <fea/utils/intrinsics.hpp>
#include <random>
#include <span>
#include <type_traits>

namespace wsy {
namespace {
template <bit_depth_e>
struct bit_depth_type;
template <>
struct bit_depth_type<bit_depth_e::_8> {
	using type = int8_t;
};
template <>
struct bit_depth_type<bit_depth_e::_16> {
	using type = int16_t;
};
template <bit_depth_e BD>
using bit_depth_t = typename bit_depth_type<BD>::type;


constexpr size_t to_value(bit_depth_e bd) {
	switch (bd) {
	case bit_depth_e::_8: {
		return 8;
	} break;
	case bit_depth_e::_16: {
		return 16;
	} break;
	default: {
		assert(false);
	} break;
	}
	return (std::numeric_limits<size_t>::max)();
}

constexpr size_t to_value(sampling_rate_e sr) {
	switch (sr) {
	case sampling_rate_e::_8: {
		return 8000;
	} break;
	case sampling_rate_e::_11: {
		return 11025;
	} break;
	case sampling_rate_e::_22: {
		return 22050;
	} break;
	case sampling_rate_e::_44: {
		return 44100;
	} break;
	default: {
		assert(false);
	} break;
	}
	return (std::numeric_limits<size_t>::max)();
}

template <class Func>
void bit_depth_type_rt(Func&& func, bit_depth_e bit_depth) {
	switch (bit_depth) {
	case bit_depth_e::_8: {
		std::forward<Func>(func).operator()<int8_t>();
	} break;
	case bit_depth_e::_16: {
		std::forward<Func>(func).operator()<int16_t>();
	} break;
	default: {
		assert(false);
	} break;
	}
}

template <size_t BitDepth>
[[nodiscard]] float bit_crush(float sample) {
	constexpr float bit_mul
			= float(fea::make_bitmask<uint32_t, (BitDepth - 1)>());
	constexpr float bit_div = 1.f / bit_mul;

	return std::floor(sample * bit_mul) * bit_div;
}

template <float Drive>
[[nodiscard]] float distortion_norm() {
	static_assert(Drive >= 0.f && Drive <= 1.f, "Invalid drive.");
	constexpr float d = Drive * 100.f;
	return 1.f / (std::atanf(d));
}

template <float Drive>
[[nodiscard]] float distort(float norm, float sample) {
	static_assert(Drive >= 0.f && Drive <= 1.f, "Invalid drive.");
	constexpr float d = Drive * 100.f;
	constexpr float atten = (1.f - (Drive * Drive + (0.9f - Drive)));
	if constexpr (Drive == 0.f) {
		return sample;
	} else {
		return std::atan(d * sample) * norm * atten;
	}
}

template <float Vol>
[[nodiscard]] float white_noise(float global_vol, float sample) {
	static_assert(Vol >= 0.f && Vol <= 1.f, "Invalid volume.");
	static std::random_device rd;
	static std::mt19937 gen{ rd() };
	static std::uniform_real_distribution<> dis(-global_vol, global_vol);

	return (sample * (1.f - Vol)) + (float(dis(gen)) * Vol);
}

struct fx_args {
	size_t bit_depth;
	sampling_rate_e sampling_rate;
	float dist_drive;
	float noise_vol;
	bool noise_after_bitcrush;
};

inline constexpr fea::enum_array<fx_args, effect_e> effect_args{
	// radio 1
	fx_args{
			.bit_depth = 5,
			.sampling_rate = sampling_rate_e::_8,
			.dist_drive = 0.2f,
			.noise_vol = 0.00001f,
			.noise_after_bitcrush = false,
	},
	// radio 2
	fx_args{
			.bit_depth = 6,
			.sampling_rate = sampling_rate_e::_8,
			.dist_drive = 0.f,
			.noise_vol = 0.01f,
			.noise_after_bitcrush = false,
	},
	// radio 3
	fx_args{
			.bit_depth = 16,
			.sampling_rate = sampling_rate_e::_44,
			.dist_drive = 1.f,
			.noise_vol = 0.01f,
			.noise_after_bitcrush = false,
	},
	// radio 4
	fx_args{
			.bit_depth = 16,
			.sampling_rate = sampling_rate_e::_22,
			.dist_drive = 0.9f,
			.noise_vol = 0.001f,
			.noise_after_bitcrush = false,
	},
	// radio 5
	fx_args{
			.bit_depth = 3,
			.sampling_rate = sampling_rate_e::_8,
			.dist_drive = 0.f,
			.noise_vol = 0.1f,
			.noise_after_bitcrush = true,
	},
	// radio 6
	fx_args{
			.bit_depth = 4,
			.sampling_rate = sampling_rate_e::_44,
			.dist_drive = 0.f,
			.noise_vol = 0.f,
			.noise_after_bitcrush = false,
	},
	// radio 7 -> radio 1 no whitenoise
	fx_args{
			.bit_depth = 5,
			.sampling_rate = sampling_rate_e::_8,
			.dist_drive = 0.2f,
			.noise_vol = 0.f,
			.noise_after_bitcrush = false,
	},
	// radio 8 -> radio 2 no whitenoise
	fx_args{
			.bit_depth = 6,
			.sampling_rate = sampling_rate_e::_8,
			.dist_drive = 0.f,
			.noise_vol = 0.f,
			.noise_after_bitcrush = false,
	},
	// radio 9 -> radio 3 no whitenoise
	fx_args{
			.bit_depth = 16,
			.sampling_rate = sampling_rate_e::_44,
			.dist_drive = 1.f,
			.noise_vol = 0.f,
			.noise_after_bitcrush = false,
	},
	// radio 10 -> radio 4 no whitenoise
	fx_args{
			.bit_depth = 16,
			.sampling_rate = sampling_rate_e::_22,
			.dist_drive = 0.9f,
			.noise_vol = 0.f,
			.noise_after_bitcrush = false,
	},
	// radio 11 -> radio 5 no whitenoise
	fx_args{
			.bit_depth = 3,
			.sampling_rate = sampling_rate_e::_8,
			.dist_drive = 0.f,
			.noise_vol = 0.f,
			.noise_after_bitcrush = true,
	},
};

template <effect_e EffectE>
void fx(const voice& vopts, std::span<float> samples) {
	constexpr fx_args args = effect_args[EffectE];

	// std::atan not constexpr.
	const float dist_norm = distortion_norm<args.dist_drive>();
	const float global_vol = float(vopts.volume) * 0.01f;

	// We only process samples that will be kept by resampling.
	const size_t idx_range
			= to_value(vopts.sampling_rate()) / to_value(args.sampling_rate);
	const size_t loop_remainder = samples.size() % idx_range;

	for (size_t i = 0; i < samples.size() - loop_remainder; i += idx_range) {
		float& s = samples[i];
		if constexpr (!args.noise_after_bitcrush) {
			s = white_noise<args.noise_vol>(global_vol, s);
		}
		s = bit_crush<args.bit_depth>(s);
		if constexpr (args.noise_after_bitcrush) {
			s = white_noise<args.noise_vol>(global_vol, s);
		}
		s = distort<args.dist_drive>(dist_norm, s);

		std::fill(samples.begin() + i + 1, samples.begin() + i + idx_range, s);
	}

	// Process remainder.
	size_t last_idx = samples.size() - loop_remainder;
	if (last_idx < samples.size()) {
		float last_s = samples[last_idx];
		last_s = bit_crush<args.bit_depth>(last_s);
		last_s = distort<args.dist_drive>(dist_norm, last_s);
		std::fill(samples.begin() + last_idx + 1, samples.end(), last_s);
	}
}
} // namespace

void process_fx(const voice& vopts, CComPtr<IStream>& stream,
		std::vector<std::byte>& byte_buffer,
		std::vector<float>& sample_buffer) {
	if (vopts.effect() == effect_e::count) {
		return;
	}

	// Read stream data bytes.
	{
		if (!SUCCEEDED(IStream_Reset(stream))) {
			fea::maybe_throw(__FUNCTION__, __LINE__,
					"Couldn't reset tts stream playhead.");
		}

		STATSTG stats{};
		if (!SUCCEEDED(stream->Stat(&stats, STATFLAG_NONAME))) {
			fea::maybe_throw(
					__FUNCTION__, __LINE__, "Couldn't get stream stats.");
		}
		size_t byte_size = stats.cbSize.QuadPart;
		byte_buffer.resize(byte_size);

		unsigned long bytes_read = 0;
		stream->Read(
				byte_buffer.data(), uint32_t(byte_buffer.size()), &bytes_read);
		assert(bytes_read == byte_buffer.size());
	}

	// Convert to float.
	bit_depth_type_rt(
			[&]<class IntT>() {
				size_t sample_size = byte_buffer.size() / sizeof(IntT);
				sample_buffer.resize(sample_size);

				constexpr float norm
						= 1.f / float((std::numeric_limits<IntT>::max)());
				const IntT* in_samples
						= reinterpret_cast<const IntT*>(byte_buffer.data());

				for (size_t i = 0; i < sample_buffer.size(); ++i) {
					sample_buffer[i] = float(in_samples[i]) * norm;
				}
			},
			vopts.bit_depth());

	// Process samples.
	fea::static_for<size_t(effect_e::count)>([&](auto const_i) {
		constexpr effect_e fx_e = effect_e(size_t(const_i));
		if (fx_e == vopts.effect()) {
			fx<fx_e>(vopts, { sample_buffer });
		}
	});

	// Convert back to bytes.
	bit_depth_type_rt(
			[&]<class IntT>() {
				assert(byte_buffer.size()
						== sample_buffer.size() * sizeof(IntT));

				constexpr float norm
						= float((std::numeric_limits<IntT>::max)());
				IntT* out_bytes = reinterpret_cast<IntT*>(byte_buffer.data());

				for (size_t i = 0; i < sample_buffer.size(); ++i) {
					out_bytes[i] = IntT(sample_buffer[i] * norm);
				}
			},
			vopts.bit_depth());

	// Write back to stream.
	{
		if (!SUCCEEDED(IStream_Reset(stream))) {
			fea::maybe_throw(__FUNCTION__, __LINE__,
					"Couldn't reset tts stream playhead.");
		}

		unsigned long bytes_written = 0;
		stream->Write(byte_buffer.data(), uint32_t(byte_buffer.size()),
				&bytes_written);
		assert(bytes_written == byte_buffer.size());

		// Leave in playable state.
		if (!SUCCEEDED(IStream_Reset(stream))) {
			fea::maybe_throw(__FUNCTION__, __LINE__,
					"Couldn't reset tts stream playhead.");
		}
	}
}

} // namespace wsy
