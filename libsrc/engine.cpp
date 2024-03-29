#include "wsay/engine.hpp"
#include "private_include/com.hpp"
#include "private_include/fx.hpp"
#include "wsay/voice.hpp"

#include <cassert>
#include <fea/utils/throw.hpp>
#include <string_view>
#include <thread>


namespace wsy {
struct async_token_imp {
	voice vopts;
	tts_voice tts;
	std::vector<device_output> device_outputs;
};

struct engine_imp {
	engine_imp()
			: voice_tokens(make_voice_tokens())
			, voice_names(make_names(voice_tokens))
			, device_tokens(make_device_tokens())
			, device_names(make_names(device_tokens)) {
		assert(voice_tokens.size() == voice_names.size());
		assert(device_tokens.size() == device_names.size());
	}

	const std::vector<CComPtr<ISpObjectToken>> voice_tokens;
	const std::vector<std::wstring> voice_names;

	const std::vector<CComPtr<ISpObjectToken>> device_tokens;
	const std::vector<std::wstring> device_names;

	std::vector<std::byte> scratch_buffer;
	std::vector<float> scratch_samples;
};


async_token::async_token() = default;
async_token::async_token(async_token&&) = default;
async_token::~async_token() = default;
async_token& async_token::operator=(async_token&&) = default;

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
	async_token tok = make_async_token(vopts);
	speak_async(sentence, tok);

	for (device_output& outv : tok._impl->device_outputs) {
		outv->WaitUntilDone(INFINITE);
	}
}

async_token engine::make_async_token(const voice& v) {
	async_token ret;
	ret._impl->vopts = v;
	make_everything(imp().voice_tokens, imp().device_tokens, imp().device_names,
			v, ret._impl->tts, ret._impl->device_outputs);
	return ret;
}

void engine::speak_async(const std::wstring& sentence, async_token& t) {
	async_token_imp& tok = *t._impl;

	// Clear the currently playing stream.
	{
		// Seek beginning.
		if (!SUCCEEDED(IStream_Reset(tok.tts.data_stream))) {
			fea::maybe_throw(__FUNCTION__, __LINE__,
					"Couldn't reset tts stream playhead.");
		}

		// Clear.
		tok.tts.data_stream->SetSize({ 0 });
	}

	// Fill the stream with tts.
	unsigned long flags
			= SPF_DEFAULT | SPF_ASYNC | SPF_PURGEBEFORESPEAK | tok.tts.flags;
	if (!SUCCEEDED(tok.tts->Speak(sentence.c_str(), flags, nullptr))) {
		fea::maybe_throw<std::invalid_argument>(
				__FUNCTION__, __LINE__, "Tts voice couldn't speak.");
	}
	tok.tts->WaitUntilDone(INFINITE);

	process_fx(tok.vopts, tok.tts.data_stream, imp().scratch_buffer,
			imp().scratch_samples);

	// Clone the input stream to output streams. They have an independent
	// playhead but same data.
	clone_input_stream(tok.tts, tok.device_outputs);

	// Play the stream on all output devices.
	for (device_output& outv : tok.device_outputs) {
		if (!SUCCEEDED(outv->SpeakStream(outv.sp_stream_clone,
					SPF_DEFAULT | SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't speak output stream.");
		}
	}
}

void engine::stop(async_token& t) {
	async_token_imp& tok = *t._impl;

	for (device_output& outv : tok.device_outputs) {
		if (!SUCCEEDED(outv->Speak(L"",
					SPF_DEFAULT | SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr))) {
			fea::maybe_throw<std::runtime_error>(
					__FUNCTION__, __LINE__, "Couldn't speak output stream.");
		}
	}
}

const engine_imp& engine::imp() const {
	return *_impl;
}
engine_imp& engine::imp() {
	return *_impl;
}

} // namespace wsy
