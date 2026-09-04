/******************************************************************************
*  System volume control
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#include "pch.h"
#include "Commands.h"

using Microsoft::WRL::ComPtr;

namespace Volume {

// Get the volume-control interface of the default audio render endpoint
static ComPtr<IAudioEndpointVolume> GetEndpointVolume() {
    ComPtr<IMMDeviceEnumerator> enumerator;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                IID_PPV_ARGS(&enumerator))))
        return nullptr;

    ComPtr<IMMDevice> device;
    if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device)))
        return nullptr;

    ComPtr<IAudioEndpointVolume> volume;
    if (FAILED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                                nullptr, reinterpret_cast<void**>(volume.GetAddressOf()))))
        return nullptr;
    return volume;
}

int Set(int pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    auto vol = GetEndpointVolume();
    if (!vol) return 1;
    return SUCCEEDED(vol->SetMasterVolumeLevelScalar(pct / 100.0f, nullptr)) ? 0 : 1;
}

int Change(int step) {
    auto vol = GetEndpointVolume();
    if (!vol) return 1;
    float cur = 0.0f;
    if (FAILED(vol->GetMasterVolumeLevelScalar(&cur))) return 1;
    float next = cur + step / 100.0f;
    if (next < 0.0f) next = 0.0f;
    if (next > 1.0f) next = 1.0f;
    return SUCCEEDED(vol->SetMasterVolumeLevelScalar(next, nullptr)) ? 0 : 1;
}

int MuteOn() {
    auto vol = GetEndpointVolume();
    if (!vol) return 1;
    return SUCCEEDED(vol->SetMute(TRUE, nullptr)) ? 0 : 1;
}

int MuteOff() {
    auto vol = GetEndpointVolume();
    if (!vol) return 1;
    return SUCCEEDED(vol->SetMute(FALSE, nullptr)) ? 0 : 1;
}

int MuteToggle() {
    auto vol = GetEndpointVolume();
    if (!vol) return 1;
    BOOL muted = FALSE;
    if (FAILED(vol->GetMute(&muted))) return 1;
    return SUCCEEDED(vol->SetMute(!muted, nullptr)) ? 0 : 1;
}

}
