#pragma once
#include <filesystem>
#include <string>

// Reads a text files, figures out utf, converts to utf16 wstring.
extern bool parse_text_file(
		const std::filesystem::path& path, std::wstring& out_text);

// If available, returns text in windows clipboard.
extern std::wstring get_clipboard_text();

// Applies speech xml to input text.
// For example, adds silence after paragraphs.
extern void add_speech_xml(std::wstring& text);
