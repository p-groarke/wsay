#include "util.hpp"

#include <fea/terminal/win_term.hpp>
#include <fea/utils/error.hpp>
#include <fea/utils/file.hpp>
#include <fea/utils/scope.hpp>
#include <fstream>
#include <iostream>
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

std::wstring get_pipe_text() {
	// To fix pipe input, use U8TEXT (and not U16).
	int res = _setmode(_fileno(stdin), _O_U8TEXT);
	res = _setmode(_fileno(stdout), _O_U8TEXT);
	fea::unused(res);

	fea::on_exit e = []() {
		// Reset afterwards.
		std::wcin.clear();

		int res = _setmode(_fileno(stdin), _O_U16TEXT);
		res = _setmode(_fileno(stdout), _O_U16TEXT);
		fea::unused(res);
	};

	// Check if we have anything in cin.
	std::wcin.seekg(0, std::wcin.end);
	std::streamoff cin_count = std::wcin.tellg();
	std::wcin.seekg(0, std::wcin.beg);

	if (cin_count <= 0) {
		return {};
	}

	// wprintf(L"Pipe detected, enabling pipe mode.\n");
	std::wstring ret;
	std::wstring pipe;
	while (std::getline(std::wcin, pipe)) {
		ret.insert(ret.end(), pipe.begin(), pipe.end());
		ret += L'\n';
	}
	// wprintf(L"Pipe text :\n%s\n", ret.c_str());
	return ret;
}