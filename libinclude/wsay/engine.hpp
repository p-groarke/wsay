#pragma once
#include <fea/memory/pimpl_ptr.hpp>
#include <string>
#include <vector>

namespace wsy {
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

	// Speaks the sentence using selected voice to playback outputs and file.
	// Non-blocking.
	void speak_async(const std::wstring& sentence);

	// Interrupts playback speaking.
	// Doesn't interrupt file ouput.
	void stop();

private:
	const engine_imp& imp() const;
	engine_imp& imp();
};

} // namespace wsy
