#pragma once
#include <fea/memory/pimpl_ptr.hpp>
#include <string>
#include <vector>

namespace wsy {
struct async_token_imp;
struct async_token : fea::pimpl_ptr<async_token_imp> {
	async_token();
	~async_token();
	async_token(async_token&&);
	async_token& operator=(async_token&&);

	async_token(const async_token&) = delete;
	async_token& operator=(const async_token&) = delete;

	friend struct engine;
};

struct voice;
struct engine_imp;
struct engine : fea::pimpl_ptr<engine_imp> {
	engine();
	~engine();

	engine(const engine&) = delete;
	engine(engine&&) = delete;
	engine& operator=(const engine&) = delete;
	engine& operator=(engine&&) = delete;

	// Returns the available voices. Use matching indexes when referring to a
	// voice.
	const std::vector<std::wstring>& voices() const;

	// Returns the available devices. Use matching indexes when referring to a
	// voice.
	const std::vector<std::wstring>& devices() const;

	// Speaks the sentence using selected voice to playback outputs and file.
	// Blocking.
	void speak(const voice& v, const std::wstring& sentence);

	// You need an async token to use async calls.
	// This token should be used in all consecutive async calls of a specific
	// voice.
	async_token make_async_token(const voice& v);

	// Speaks the sentence using selected voice to playback outputs and file.
	// Non-blocking.
	void speak_async(const std::wstring& sentence, async_token& t);

	// Interrupts playback speaking.
	// Doesn't interrupt file ouput.
	void stop(async_token& t);

private:
	const engine_imp& imp() const;
	engine_imp& imp();
};

} // namespace wsy
