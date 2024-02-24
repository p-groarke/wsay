#include "private_include/util.hpp"

#include <fea/string/string.hpp>
#include <fea/terminal/utf8.hpp>
#include <fea/utils/error.hpp>
#include <fea/utils/file.hpp>
#include <fea/utils/scope.hpp>
#include <fstream>
#include <iostream>
#include <regex>
#include <windows.h>

bool parse_text_file(
		const std::filesystem::path& path, std::wstring& out_text) {

	if (!std::filesystem::exists(path)) {
		fwprintf(stderr, L"Input text file doesn't exist : '%s'\n",
				path.wstring().c_str());
		return false;
	}

	if (path.extension() != ".txt") {
		fwprintf(stderr,
				L"Text option only supports '.txt' "
				"files.\n");
		return false;
	}

	std::ifstream ifs{ path };
	if (!ifs.is_open()) {
		return false;
	}

	std::u32string text = fea::open_text_file_with_bom(ifs);
	out_text = fea::utf32_to_utf16_w(text);

	if (out_text.empty()) {
		fwprintf(stderr,
				L"Couldn't parse text file or there is no "
				"text in file.\n");
		return false;
	}

	return true;
}

std::wstring get_clipboard_text() {
	if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		fwprintf(stderr, L"Clipboard doesn't contain text.\n");
		return {};
	}

	if (!OpenClipboard(nullptr)) {
		fwprintf(stderr, L"Couldn't open clipboard for reading.\n");
		std::error_code ec = fea::last_os_error();
		std::wstring wmsg = fea::utf8_to_utf16_w(ec.message());
		fwprintf(stderr, L"Error message : '%s'\n", wmsg.c_str());
		return {};
	}
	fea::on_exit e = []() { CloseClipboard(); };

	HANDLE data = GetClipboardData(CF_UNICODETEXT);
	if (data == nullptr) {
		fwprintf(stderr, L"Couldn't read clipboard data.\n");
		std::error_code ec = fea::last_os_error();
		std::wstring wmsg = fea::utf8_to_utf16_w(ec.message());
		fwprintf(stderr, L"Error message : '%s'\n", wmsg.c_str());
		return {};
	}

	const wchar_t* clip_text = static_cast<const wchar_t*>(GlobalLock(data));
	if (clip_text == nullptr) {
		fwprintf(stderr, L"Couldn't convert clipboard data to string.\n");
		return {};
	}
	fea::on_exit e2 = [&]() { GlobalUnlock(data); };

	return std::wstring{ clip_text };
}

namespace {
const std::wregex spaces_re{
	L"[ \\t\\v]+",
	std::regex_constants::optimize | std::regex_constants::icase,
};
const std::wregex line_endings_re{
	L"\\n+",
	std::regex_constants::optimize | std::regex_constants::icase,
};

constexpr size_t paragraph_silence_msec = 100;
const std::wstring paragraph_xml
		= std::format(L"\n<silence msec=\"{}\"/>", paragraph_silence_msec);
} // namespace

void add_speech_xml(std::wstring& text) {
	// First, cleanup.
	{
		fea::replace_all_inplace(text, L'\r', L'\n');
		fea::replace_all_inplace(text, L'\f', L'\n');
		text = std::regex_replace(text, spaces_re, L" ");
		text = std::regex_replace(text, line_endings_re, L"\n");
	}

	// Now, add speech xml.
	fea::replace_all_inplace(text, L"\n", paragraph_xml);
}
