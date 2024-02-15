#include "../src/util.hpp"

#include <fea/utils/file.hpp>
#include <gtest/gtest.h>
#include <wsay/wsay.hpp>

namespace {
std::filesystem::path exe_path;

// TEST(libwsay, unicode) {
//	std::filesystem::path testfiles_dir = exe_path / "tests_data/";
//	for (const std::filesystem::path& filepath :
//			std::filesystem::directory_iterator(testfiles_dir)) {
//
//		printf("%s\n", filepath.string().c_str());
//
//		std::wstring text;
//		parse_text_file(filepath, text);
//
//		// wprintf(L"%s\n", text.c_str());
//	}
// }

} // namespace

int main(int argc, char** argv) {
	exe_path = fea::executable_dir(argv[0]);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
