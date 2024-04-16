#include "../src/private_include/util.hpp"

#include <fea/utils/file.hpp>
#include <gtest/gtest.h>

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

// // At the top of the C++ file.
// #include <initguid.h>

// #include "objbase.h"
// #include "uiautomation.h"

// #include <chrono>
// #include <format>
// #include <iostream>
// #include <thread>

// IAccPropServices* _pAccPropServices = NULL;

// extern INT_PTR dlgproc(HWND p1, UINT p2, WPARAM p3, LPARAM p4) {
// 	// std::wcout << std::format(L"{} {} {} {}", p1, p2, p3, p4);
// 	std::wcout << p1 << p2 << p3 << p4 << std::endl;
// 	return TRUE;
// }

// LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// int wmain(int, wchar_t**, wchar_t**) {
// 	// HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, L"edit",
// 	//		L"expelliamus",
// 	//		WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 600,
// 	//		400, 200, 200, nullptr, nullptr, nullptr, nullptr);

// 	HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, L"edit", L"expelliamus",
// 			WS_VISIBLE | WS_POPUP, 600, 400, 200, 200, nullptr, nullptr,
// 			nullptr, nullptr);
// 	hwnd;

// 	// SetWindowLong(hwnd, GWL_STYLE, 0); // remove all window styles, check
// 	// MSDN
// 	//								   // for details

// 	// ShowWindow(hwnd, SW_SHOW); // display window

// 	// DialogBoxW(nullptr, L"Test", hwnd, &dlgproc);
// 	// CreateDialogW(nullptr, L"Test", hwnd, &dlgproc);

// 	int IDC_LABEL = 1;
// 	SetDlgItemTextW(hwnd, IDC_LABEL, L"Hello");

// 	// Run when the UI is created.
// 	HRESULT hr = CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC,
// 			IID_PPV_ARGS(&_pAccPropServices));

// 	// HRESULT hr = CoCreateInstance(__uuidof(CAccPropServices), nullptr,
// 	//		CLSCTX_INPROC, IID_PPV_ARGS(&_pAccPropServices));

// 	if (SUCCEEDED(hr)) {
// 		// Set up the status label to be an assertive LiveRegion. The assertive
// 		// property is used to request that the screen reader announces the
// 		// update immediately. And alternative setting of polite requests that
// 		// the screen reader completes any in-progress announcement before
// 		// announcing the LiveRegion update.

// 		VARIANT var;
// 		var.vt = VT_I4;
// 		var.lVal = Assertive;

// 		hr = _pAccPropServices->SetHwndProp(GetDlgItem(hwnd, IDC_LABEL),
// 				DWORD(OBJID_CLIENT), DWORD(CHILDID_SELF),
// 				LiveSetting_Property_GUID, var);
// 	}


// 	// Run when the text on the label changes. Raise a UIA LiveRegionChanged
// 	// event so that a screen reader is made aware of a change to the
// 	// LiveRegion. Make sure the updated text is set on the label before
// 	// making this call.

// 	NotifyWinEvent(EVENT_OBJECT_LIVEREGIONCHANGED, GetDlgItem(hwnd, IDC_LABEL),
// 			OBJID_CLIENT, CHILDID_SELF);


// 	// Run when the UI is destroyed.
// 	if (_pAccPropServices != nullptr) {
// 		// We only added the one property to the hwnd.
// 		MSAAPROPID props[] = { LiveSetting_Property_GUID };
// 		hr = _pAccPropServices->ClearHwndProps(GetDlgItem(hwnd, IDC_LABEL),
// 				DWORD(OBJID_CLIENT), DWORD(CHILDID_SELF), props,
// 				ARRAYSIZE(props));

// 		_pAccPropServices->Release();
// 		_pAccPropServices = NULL;
// 	}

// 	bool done = false;
// 	while (!done) {
// 		MSG msg;
// 		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
// 			done |= (GetAsyncKeyState(VK_ESCAPE) != 0);
// 			TranslateMessage(&msg);
// 			DispatchMessage(&msg);
// 		}
// 	}
// }
