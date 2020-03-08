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
#include <fea_utils/fea_utils.hpp>
#include <filesystem>
#include <functional>
#include <iostream>
#include <ns_getopt/ns_getopt.h>
#include <string>
#include <unordered_set>
#include <wil/resource.h>
#include <wil/result.h>

namespace {
using unique_couninitialize_call
		= wil::unique_call<decltype(&::CoUninitialize), ::CoUninitialize>;

const std::wstring exit_cmd = L"!exit";
const std::wstring shutup_cmd = L"!stop";

enum class output_t : unsigned {
	fileout,
	device,
	count,
};

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
		ret.push_back(std::move(cpVoiceToken));
	}

	return ret;
}

std::vector<CComPtr<ISpObjectToken>> get_playback_devices() {
	std::vector<CComPtr<ISpObjectToken>> ret;
	CComPtr<IEnumSpObjectTokens> cpEnum;

	// Enumerate the available audio output devices.
	if (!SUCCEEDED(SpEnumTokens(SPCAT_AUDIOOUT, nullptr, nullptr, &cpEnum))) {
		return ret;
	}

	unsigned long device_count = 0;
	// Get the number of audio output devices.
	if (!SUCCEEDED(cpEnum->GetCount(&device_count))) {
		return ret;
	}

	// Obtain a list of available audio output tokens,
	// set the output to the token, and call Speak.
	for (size_t i = 0; i < device_count; ++i) {
		CComPtr<ISpObjectToken> cpAudioOutToken;

		if (!SUCCEEDED(cpEnum->Next(1, &cpAudioOutToken, nullptr))) {
			return ret;
		}
		ret.push_back(std::move(cpAudioOutToken));
	}

	return ret;
}

CComPtr<ISpVoice> create_voice() {
	CComPtr<ISpVoice> ret;
	if (!SUCCEEDED(ret.CoCreateInstance(CLSID_SpVoice))) {
		fprintf(stderr, "Couldn't initialize voice.\n");
		std::exit(-1);
	}
	return ret;
}

void setup_fileout(CComPtr<ISpVoice>& voice,
		const std::filesystem::path& output_filepath) {
	CComPtr<ISpStream> cpStream;
	CSpStreamFormat cAudioFmt;
	if (!SUCCEEDED(cAudioFmt.AssignFormat(SPSF_44kHz16BitMono))) {
		fprintf(stderr, "Couldn't set audio format (44kHz, 16bit, mono).\n");
		std::exit(-1);
	}

	std::wstring fileout;
	if (!output_filepath.empty()) {
		fileout = output_filepath.wstring();
	} else {
		fileout = (std::filesystem::current_path() / L"out.wav").wstring();
	}

	if (!SUCCEEDED(SPBindToFile(fileout.c_str(), SPFM_CREATE_ALWAYS, &cpStream,
				&cAudioFmt.FormatId(), cAudioFmt.WaveFormatExPtr()))) {
		fprintf(stderr, "Couldn't bind stream to file.\n");
		std::exit(-1);
	}

	if (!SUCCEEDED(voice->SetOutput(cpStream, TRUE))) {
		fprintf(stderr, "Couldn't set voice output.\n");
		std::exit(-1);
	}
}

} // namespace

namespace std {
template <>
struct hash<std::pair<output_t, unsigned>> {
	static_assert(sizeof(std::pair<output_t, unsigned>) == 8,
			"must reimplement hash");

	size_t operator()(const std::pair<output_t, unsigned>& p) const noexcept {
		size_t h1 = size_t(p.first);
		size_t h2 = size_t(p.second);
		size_t ret = h1 ^ (h2 << 32);
		return ret;
	}
};
} // namespace std

int main(int argc, char** argv) {
	RETURN_IF_FAILED(::CoInitialize(nullptr));
	unique_couninitialize_call cleanup;

	// nothing specified : default voice
	// file out specified : default voice -> stream to file
	// device specified : default voice -> output device
	// N devices specified : multi voices -> output devices
	// device specified + file out : multi voices -> file stream + output
	// N devices specified + file out : multi voices -> file stream + N output

	std::vector<CComPtr<ISpObjectToken>> available_voices
			= get_voices(SPCAT_VOICES);
	{
		std::vector<CComPtr<ISpObjectToken>> extra_voices
				= get_voices(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_"
							 L"OneCore\\Voices");
		available_voices.insert(available_voices.end(), extra_voices.begin(),
				extra_voices.end());
	}

	std::vector<CComPtr<ISpObjectToken>> available_devices
			= get_playback_devices();

	bool interactive_mode = false;
	size_t chosen_voice = 0;
	std::string speech_text;
	std::filesystem::path output_filepath;
	std::filesystem::path input_text_filepath;

	// the extra unsigned is used only for device playback identification
	std::unordered_set<std::pair<output_t, unsigned>> selected_output_types;

	std::array<opt::argument, 8> args = { {
			// clang-format
			{ "\"sentence\"", opt::type::raw_arg,
					[&](std::string_view f) {
						speech_text = f;
						return true;
					},
					"Sentence to say. You can use speech xml." },
			{ "list_voices", opt::type::no_arg,
					[&]() {
						size_t device_num = 1;
						for (const CComPtr<ISpObjectToken>& t :
								available_voices) {
							LPWSTR str = nullptr;
							t->GetStringValue(nullptr, &str);
							wprintf(L"%zu : %s\n", device_num, str);
							++device_num;
						}
						return true;
					},
					"Lists available voices.", 'l' },
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
			{ "output_file", opt::type::optional_arg,
					[&](std::string_view f) {
						if (!f.empty()) {
							output_filepath = { f };
						}
						selected_output_types.insert({ output_t::fileout,
								std::numeric_limits<unsigned>::max() });
						return true;
					},
					"Outputs to wav file. Uses 'out.wav' if no filename is "
					"provided.",
					'o' },
			{ "interactive", opt::type::no_arg,
					[&]() {
						interactive_mode = true;
						return true;
					},
					"Enter interactive mode. Type sentences, they will be "
					"spoken when you press enter.\nUse 'ctrl+c' or "
					"type '!exit' to quit.",
					'I' },
			{ "list_devices", opt::type::no_arg,
					[&]() {
						size_t device_num = 1;
						for (const CComPtr<ISpObjectToken>& t :
								available_devices) {
							LPWSTR str = nullptr;
							t->GetStringValue(nullptr, &str);
							wprintf(L"%zu : %s\n", device_num, str);
							++device_num;
						}
						return true;
					},
					"List detected playback devices.", 'd' },
			{ "playback_device", opt::type::multi_arg,
					[&](const opt::multi_array& arr, size_t size) {
						for (size_t i = 0; i < size; ++i) {
							std::string t{ arr[i] };
							unsigned chosen_device = std::stoul(t);
							if (chosen_device == 0
									|| chosen_device
											> available_devices.size()) {
								fprintf(stderr,
										"beep boop, wrong device "
										"selected.\n\n");
								return false;
							}

							selected_output_types.insert(
									{ output_t::device, chosen_device });
						}

						return true;
					},
					"Specify a playback device. Use the number "
					"provided by --list_devices.\nYou can provide more than 1 "
					"playback device, seperate the numbers with spaces.\nYou "
					"can also mix output to file + playback.",
					'p' },
	} };

	std::string help_outro = "wsay\nversion ";
	help_outro += WSAY_VERSION;
	help_outro += "\nhttps://github.com/p-groarke/wsay/releases\n";
	help_outro += "Philippe Groarke <philippe.groarke@gmail.com>";

	bool succeeded = opt::parse_arguments(argc, argv, args, { "", help_outro });
	if (!succeeded)
		return -1;

	std::vector<CComPtr<ISpVoice>> voices;

	if (selected_output_types.empty()) {
		voices.push_back(create_voice());
	} else {
		for (std::pair<output_t, size_t> type_pair : selected_output_types) {
			output_t t = type_pair.first;

			switch (t) {
			case output_t::fileout: {
				voices.push_back(create_voice());
				setup_fileout(voices.back(), output_filepath);
			} break;
			case output_t::device: {
				voices.push_back(create_voice());
				size_t selected_device = type_pair.second;
				assert(selected_device != 0
						&& selected_device <= available_devices.size());

				voices.back()->SetOutput(
						available_devices[selected_device - 1], TRUE);
			} break;
			default: {
				fprintf(stderr, "Something went terribly wrong.\n");
			} break;
			}
		}
	}


	if (chosen_voice != 0) {
		for (CComPtr<ISpVoice>& voice : voices) {
			voice->SetVoice(available_voices[chosen_voice - 1]);
		}
	}

	if (interactive_mode) {
		printf("[Info] Type sentences, press enter to speak them.\n");
		wprintf(L"[Command] '%s' : Exit interactive mode.\n", exit_cmd.c_str());
		wprintf(L"[Command] '%s' : Interrupt speaking.\n", shutup_cmd.c_str());
		printf("\n");

		std::wstring wsentence;
		while (std::getline(std::wcin, wsentence)) {
			if (wsentence == exit_cmd) {
				break;
			}

			if (wsentence == shutup_cmd) {
				for (CComPtr<ISpVoice>& voice : voices) {
					voice->Speak(L"",
							SPF_DEFAULT | SPF_PURGEBEFORESPEAK | SPF_ASYNC,
							nullptr);
				}
				continue;
			}

			for (CComPtr<ISpVoice>& voice : voices) {
				voice->Speak(
						wsentence.c_str(), SPF_DEFAULT | SPF_ASYNC, nullptr);
			}
		}
	} else {
		std::wstring wsentence{ speech_text.begin(), speech_text.end() };
		for (CComPtr<ISpVoice>& voice : voices) {
			voice->Speak(wsentence.c_str(), SPF_DEFAULT | SPF_ASYNC, nullptr);
		}

		for (CComPtr<ISpVoice>& voice : voices) {
			voice->WaitUntilDone(INFINITE);
		}
	}
}
