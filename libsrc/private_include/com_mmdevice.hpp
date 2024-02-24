#define _ATL_APARTMENT_THREADED
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <functiondiscoverykeys.h>
#include <mmdeviceapi.h>
#include <sapi.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#include <sphelper.h>
#pragma warning(pop)

#include <cassert>
#include <fea/string/string.hpp>
#include <memory>
#include <vector>
#include <wil/resource.h>
#include <wil/result.h>

namespace wsy {
size_t default_output_device_idx(
		const std::vector<std::wstring>& device_names) {
	CComPtr<IMMDeviceEnumerator> dev_enum;
	if (!SUCCEEDED(dev_enum.CoCreateInstance(__uuidof(MMDeviceEnumerator)))) {
		return 0;
	}

	CComPtr<IMMDevice> default_device;
	if (!SUCCEEDED(dev_enum->GetDefaultAudioEndpoint(
				eRender, eMultimedia, &default_device))) {
		return 0;
	}

	wil::unique_cotaskmem_string id_str;
	if (!SUCCEEDED(default_device->GetId(&id_str))) {
		return 0;
	}

	CComPtr<IPropertyStore> prop_store;
	if (!SUCCEEDED(default_device->OpenPropertyStore(STGM_READ, &prop_store))) {
		return 0;
	}

	wil::unique_prop_variant name;
	if (!SUCCEEDED(prop_store->GetValue(PKEY_Device_FriendlyName, &name))) {
		return 0;
	}
	if (name.vt == VT_EMPTY) {
		return 0;
	}
	assert(name.pwszVal != nullptr);

	// Search for device name in our token names.
	{
		auto it = std::find(
				device_names.begin(), device_names.end(), name.pwszVal);
		if (it != device_names.end()) {
			return size_t(std::distance(device_names.begin(), it));
		}
	}

	// Try lower case.
	{
		std::wstring name_str = name.pwszVal;
		assert(!name_str.empty());
		fea::to_lower_ascii_inplace(name_str);

		std::vector<std::wstring> cpy = device_names;
		for (std::wstring& str : cpy) {
			fea::to_lower_ascii_inplace(str);
		}

		auto it = std::find(cpy.begin(), cpy.end(), name_str);
		if (it != cpy.end()) {
			return size_t(std::distance(cpy.begin(), it));
		}
	}

	// Q : Are there situations where this isn't enough?
	std::wcout << "Couldn't detect default audio device, using first.\nPlease "
				  "report this so I can improve default device detection, ty!"
			   << std::endl;
	return 0;
}

} // namespace wsy