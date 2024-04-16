#include "private_include/fx.hpp"

#include <cmath>
#include <cstdint>
#include <fea/meta/static_for.hpp>
#include <fea/utils/error.hpp>
#include <fea/utils/intrinsics.hpp>
#include <numbers>
#include <random>
#include <span>
#include <type_traits>

namespace wsay {
namespace {
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

	if constexpr (Vol == 0.f) {
		return sample;
	} else {
		return (sample * (1.f - Vol)) + (float(dis(gen)) * Vol);
	}
}

struct biquad_state {
	float z1 = 0.f;
	float z2 = 0.f;
};

template <biquad_args Args>
	requires(Args.type == biquad_type_e::count)
[[nodiscard]] float biquad(float sample, biquad_state&) {
	return sample;
}
template <biquad_args Args>
	requires(Args.type != biquad_type_e::count)
[[nodiscard]] float biquad(float sample, biquad_state& state) {
	static_assert(
			Args.type != biquad_type_e::count, "Should never end up here.");

	// static const float v = std::pow(10.f, std::abs(Args.gain) / 20.f);
	static const float k = std::tan(std::numbers::pi_v<float> * Args.freq);
	static const float norm = 1.f / (1.f + k / Args.q + k * k);

	struct vals {
		constexpr vals() {
			// https://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
			if constexpr (Args.type == biquad_type_e::lowpass) {
				a0 = k * k * norm;
				a1 = 2.f * a0;
				a2 = a0;
				b1 = 2.f * (k * k - 1.f) * norm;
				b2 = (1.f - k / Args.q + k * k) * norm;
			} else if constexpr (Args.type == biquad_type_e::highpass) {
				a0 = 1.f * norm;
				a1 = -2.f * a0;
				a2 = a0;
				b1 = 2.f * (k * k - 1.f) * norm;
				b2 = (1.f - k / Args.q + k * k) * norm;
			} else if constexpr (Args.type == biquad_type_e::bandbass) {
				a0 = k / Args.q * norm;
				a1 = 0.f;
				a2 = -a0;
				b1 = 2.f * (k * k - 1.f) * norm;
				b2 = (1.f - k / Args.q + k * k) * norm;
			} else if constexpr (Args.type == biquad_type_e::notch) {
				a0 = (1.f + k * k) * norm;
				a1 = 2.f * (k * k - 1.f) * norm;
				a2 = a0;
				b1 = a1;
				b2 = (1.f - k / Args.q + k * k) * norm;
			}
		}

		float a0 = 1.f;
		float a1 = 0.f;
		float a2 = 0.f;
		float b1 = 0.f;
		float b2 = 0.f;
	};
	static const vals v{};

	// Do that actual processing.
	float ret = sample * v.a0 + state.z1;
	state.z1 = sample * v.a1 + state.z2 - v.b1 * ret;
	state.z2 = sample * v.a2 - v.b2 * ret;
	return ret;
}


template <radio_preset_e EffectE, bool DisableWhitenoise>
constexpr fx_args get_fx_args() {
	if constexpr (DisableWhitenoise) {
		fx_args ret = radio_presets[EffectE];
		ret.noise_vol = 0.f;
		return ret;
	} else {
		return radio_presets[EffectE];
	}
}

template <radio_preset_e EffectE, bool DisableWhitenoise>
void fx(const voice& vopts, std::span<float> samples) {
	// constexpr fx_args args = effect_args[EffectE];
	constexpr fx_args args = get_fx_args<EffectE, DisableWhitenoise>();

	// std::atan not constexpr.
	const float dist_norm = distortion_norm<args.dist_drive>();
	const float global_vol = float(vopts.volume) * 0.01f;

	biquad_state bi_state = {};

	// We only process samples that will be kept by resampling.
	const size_t idx_range
			= to_value(vopts.sampling_rate()) / to_value(args.sampling_rate);
	const size_t loop_remainder = samples.size() % idx_range;

	auto process = [&](float* begin, size_t size) {
		float& s = *begin;
		if constexpr (!args.noise_after_bitcrush) {
			s = white_noise<args.noise_vol>(global_vol, s);
		}
		s = bit_crush<args.bit_depth>(s);
		if constexpr (args.noise_after_bitcrush) {
			s = white_noise<args.noise_vol>(global_vol, s);
		}
		s = distort<args.dist_drive>(dist_norm, s);
		s = biquad<args.biquad>(s, bi_state);
		s *= args.gain;

		std::fill(begin + 1, begin + size, s);
	};

	for (size_t i = 0; i < samples.size() - loop_remainder; i += idx_range) {
		process(&samples[i], idx_range);
	}

	// Process remainder.
	size_t last_idx = samples.size() - loop_remainder;
	if (last_idx < samples.size()) {
		process(&samples[last_idx], samples.size() - last_idx);
	}
}
} // namespace

void process_fx(const voice& vopts, CComPtr<IStream>& stream,
		std::vector<std::byte>& byte_buffer,
		std::vector<float>& sample_buffer) {
	if (vopts.radio_effect() == radio_preset_e::count) {
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
		if (!SUCCEEDED(stream->Read(byte_buffer.data(),
					uint32_t(byte_buffer.size()), &bytes_read))) {
			fea::maybe_throw(__FUNCTION__, __LINE__,
					"Couldn't read tts stream to process effects.");
		}
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
	fea::static_for<size_t(radio_preset_e::count)>([&](auto const_i) {
		constexpr radio_preset_e fx_e = radio_preset_e(size_t(const_i));
		if (fx_e == vopts.radio_effect()) {
			if (vopts.radio_effect_disable_whitenoise) {
				fx<fx_e, true>(vopts, { sample_buffer });
			} else {
				fx<fx_e, false>(vopts, { sample_buffer });
			}
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
		if (!SUCCEEDED(stream->Write(byte_buffer.data(),
					uint32_t(byte_buffer.size()), &bytes_written))) {
			fea::maybe_throw(__FUNCTION__, __LINE__,
					"Couldn't write effects to tts stream.");
		}
		assert(bytes_written == byte_buffer.size());

		// Leave in playable state.
		if (!SUCCEEDED(IStream_Reset(stream))) {
			fea::maybe_throw(__FUNCTION__, __LINE__,
					"Couldn't reset tts stream playhead.");
		}
	}
}

} // namespace wsay
