#define _ATL_APARTMENT_THREADED
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <sapi.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#include <sphelper.h>
#pragma warning(pop)

#include <array>
#include <codecvt>
#include <fea_utils/fea_utils.hpp>
#include <filesystem>
#include <functional>
#include <iostream>
#include <locale>
#include <memory>
#include <ns_getopt/ns_getopt.h>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <wil/resource.h>
#include <wil/result.h>

namespace {
using unique_couninitialize_call
		= wil::unique_call<decltype(&::CoUninitialize), ::CoUninitialize>;

const std::wstring exit_cmd = L"!exit";

std::vector<CComPtr<ISpObjectToken>> get_voices(const wchar_t* registry_key) {
	std::vector<CComPtr<ISpObjectToken>> ret;

	CComPtr<IEnumSpObjectTokens> cpEnum;
	if (!SUCCEEDED(SpEnumTokens(registry_key, nullptr, nullptr, &cpEnum))) {
		return ret;
	}

	unsigned long num_voices = 0;
	if (!SUCCEEDED(cpEnum->GetCount(&num_voices))) {
		return ret;
	}

	for (size_t i = 0; i < num_voices; ++i) {

		CComPtr<ISpObjectToken> cpVoiceToken;
		if (!SUCCEEDED(cpEnum->Next(1, &cpVoiceToken, nullptr))) {
			return ret;
		}

		LPWSTR str = nullptr;
		cpVoiceToken->GetStringValue(nullptr, &str);
		ret.push_back(std::move(cpVoiceToken));
	}

	return ret;
}

void exec_interactive_mode(CComPtr<ISpVoice> voice) {
	fea::mtx_safe<std::queue<std::wstring>> wsentences;

	auto get_work = [&]() {
		std::wstring wsentence;
		while (std::getline(std::wcin, wsentence)) {
			bool exit = wsentence == exit_cmd;

			wsentences.write([&](std::queue<std::wstring>& q) {
				q.push(std::move(wsentence));
			});

			if (exit) {
				return;
			}
		}
	};

	std::thread t{ get_work };

	bool exit = false;
	while (!exit) {
		std::wstring wsentence;
		wsentences.write([&](std::queue<std::wstring>& q) {
			if (q.empty()) {
				return;
			}
			while (!q.empty()) {
				if (q.front() == exit_cmd) {
					exit = true;
					break;
				}
				wsentence += std::move(q.front());
				wsentence += '\n';
				q.pop();
			}
		});

		if (wsentence.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
			continue;
		}

		voice->Speak(wsentence.c_str(), 0, nullptr);
		wsentence.clear();
	}
	t.join();
}
} // namespace

int main(int argc, char** argv) {
	RETURN_IF_FAILED(::CoInitialize(nullptr));
	unique_couninitialize_call cleanup;

	CComPtr<ISpVoice> voice;
	RETURN_IF_FAILED(voice.CoCreateInstance(CLSID_SpVoice));

	std::vector<CComPtr<ISpObjectToken>> available_voices
			= get_voices(SPCAT_VOICES);
	{
		std::vector<CComPtr<ISpObjectToken>> extra_voices
				= get_voices(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_"
							 L"OneCore\\Voices");
		available_voices.insert(available_voices.end(), extra_voices.begin(),
				extra_voices.end());
	}

	bool output_to_file = false;
	bool interactive_mode = false;
	size_t chosen_voice = 0;
	std::string speech_text;
	std::filesystem::path output_filepath;
	std::filesystem::path input_text_filepath;
	std::array<opt::argument, 6> args = { {
			// clang-format
			{ "\"sentence\"", opt::type::raw_arg,
					[&](std::string_view f) {
						speech_text = f;
						return true;
					},
					"Sentence to say. You can use speech xml." },
			{ "voice", opt::type::required_arg,
					[&](std::string_view f) {
						std::string t{ f };
						chosen_voice = std::stoull(t);
						if (chosen_voice == 0
								|| chosen_voice > available_voices.size()) {
							fprintf(stderr,
									"beep boop, wrong voice selected.\n\n");
							return false;
						}
						return true;
					},
					"Choose a different voice. Use the voice number printed "
					"using --list_voices.",
					'v' },
			{ "output_file", opt::type::optional_arg,
					[&](std::string_view f) {
						if (!f.empty()) {
							output_filepath = { f };
						}
						output_to_file = true;
						return true;
					},
					"Outputs to wav file. Uses 'out.wav' if no filename is "
					"provided.",
					'o' },
			{ "list_voices", opt::type::no_arg,
					[&]() {
						size_t voice_num = 1;
						for (const CComPtr<ISpObjectToken>& t :
								available_voices) {
							LPWSTR str = nullptr;
							t->GetStringValue(nullptr, &str);
							wprintf(L"%zu : %s\n", voice_num, str);
							++voice_num;
						}
						return true;
					},
					"Lists available voices.", 'l' },
			{ "input_text", opt::type::required_arg,
					[&](std::string_view f) {
						input_text_filepath = { f };
						if (!std::filesystem::exists(input_text_filepath)) {
							fprintf(stderr,
									"Input text file doesn't exist : '%s'\n",
									input_text_filepath.string().c_str());
							return false;
						}

						if (input_text_filepath.extension() != ".txt") {
							fprintf(stderr,
									"Text option only supports '.txt' "
									"files.\n");
							return false;
						}

						std::vector<std::string> sentences;
						if (!fea::open_text_file(
									input_text_filepath, sentences)) {
							return false;
						}

						for (const std::string& str : sentences) {
							if (str.empty()) {
								continue;
							}

							speech_text += str;
							speech_text += '\n';
						}

						if (speech_text.empty()) {
							fprintf(stderr,
									"Couldn't parse text file or there is no "
									"text in file.\n");
							return false;
						}

						return true;
					},
					"Play text from '.txt' file. Supports speech xml.", 'i' },
			{ "interactive", opt::type::no_arg,
					[&]() {
						interactive_mode = true;
						return true;
					},
					"Enter interactive mode. Type sentences, they will be "
					"spoken when you press enter.\nUse 'ctrl+c' to exit, or "
					"type '!exit' (without quotes).",
					'I' },
	} };

	std::string help_outro = "wsay\nversion ";
	help_outro += WSAY_VERSION;
	help_outro += "\nPhilippe Groarke <philippe.groarke@gmail.com>";

	bool succeeded = opt::parse_arguments(argc, argv, args, { "", help_outro });
	if (!succeeded)
		return -1;

	if (chosen_voice != 0) {
		voice->SetVoice(available_voices[chosen_voice - 1]);
	}

	if (output_to_file) {
		CComPtr<ISpStream> cpStream;
		CSpStreamFormat cAudioFmt;
		RETURN_IF_FAILED(cAudioFmt.AssignFormat(SPSF_44kHz16BitMono));

		std::wstring fileout;
		if (!output_filepath.empty()) {
			fileout = output_filepath.wstring();
		} else {
			fileout = (std::filesystem::current_path() / L"out.wav").wstring();
		}
		SPBindToFile(fileout.c_str(), SPFM_CREATE_ALWAYS, &cpStream,
				&cAudioFmt.FormatId(), cAudioFmt.WaveFormatExPtr());
		voice->SetOutput(cpStream, TRUE);
	}

	if (interactive_mode) {
		if (output_to_file) {
			printf("[Info]\tYou are using file output while in "
				   "interactive mode. You will not hear any sound.\n\n");
		}
		exec_interactive_mode(voice);
	} else {
		std::wstring wsentence{ speech_text.begin(), speech_text.end() };
		voice->Speak(wsentence.c_str(), 0, nullptr);
	}
}
