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
#include <locale>
#include <memory>
#include <ns_getopt/ns_getopt.h>
#include <string>
#include <string_view>
#include <wil/resource.h>
#include <wil/result.h>

using unique_couninitialize_call
		= wil::unique_call<decltype(&::CoUninitialize), ::CoUninitialize>;

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
	size_t chosen_voice = 0;
	std::string sentence;
	std::array<opt::argument, 4> args = { {
			// clang-format
			{ "\"sentence\"", opt::type::raw_arg,
					[&](std::string_view f) {
						sentence = f;
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
			{ "output_file", opt::type::no_arg,
					[&]() {
						output_to_file = true;
						return true;
					},
					"Outputs to out.wav", 'o' },
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
	} };

	bool succeeded = opt::parse_arguments(argc, argv, args);
	if (!succeeded)
		return -1;

	if (chosen_voice != 0) {
		voice->SetVoice(available_voices[chosen_voice - 1]);
	}

	if (output_to_file) {
		CComPtr<ISpStream> cpStream;
		CSpStreamFormat cAudioFmt;
		RETURN_IF_FAILED(cAudioFmt.AssignFormat(SPSF_44kHz16BitMono));

		std::wstring pwd
				= (std::filesystem::current_path() / L"out.wav").wstring();
		SPBindToFile(pwd.c_str(), SPFM_CREATE_ALWAYS, &cpStream,
				&cAudioFmt.FormatId(), cAudioFmt.WaveFormatExPtr());
		voice->SetOutput(cpStream, TRUE);
	}

	std::wstring wsentence{ sentence.begin(), sentence.end() };
	voice->Speak(wsentence.c_str(), 0, nullptr);
}
