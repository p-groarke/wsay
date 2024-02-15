#pragma once
#include <fea/memory/pimpl_ptr.hpp>
#include <string>
#include <vector>

namespace wsy {
struct engine_imp;
struct engine : fea::pimpl_ptr<engine_imp> {
	engine();
	~engine();

	engine(const engine&);
	engine(engine&&);
	engine& operator=(const engine&);
	engine& operator=(engine&&);

	// Returns the available voices. Use matching indexes when referring to a
	// voice.
	const std::vector<std::wstring>& voices() const;

	// Returns the available devices. Use matching indexes when referring to a
	// voice.
	const std::vector<std::wstring>& devices() const;

	// Speaks the sentence using selected voice to playback outputs and file.
	// Blocking.
	void speak(const std::wstring& sentence);

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
