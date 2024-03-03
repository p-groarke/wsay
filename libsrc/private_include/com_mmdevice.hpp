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
#include <iostream>
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