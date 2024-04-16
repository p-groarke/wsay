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
#include <fea/memory/pimpl_ptr.hpp>
#include <string>
#include <vector>

namespace wsay {
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

} // namespace wsay
