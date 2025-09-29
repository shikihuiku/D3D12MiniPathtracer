#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;

// Helper functions for Direct3D12
inline std::wstring HrToString(HRESULT hr)
{
    wchar_t s_str[64] = {};
    swprintf_s(s_str, L"HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::wstring(s_str);
}

// Helper macro to check HRESULT and display error message on failure
#define ThrowIfFailed(hr)                                              \
{                                                                      \
    HRESULT hr_ = (hr);                                               \
    if (FAILED(hr_))                                                   \
    {                                                                  \
        std::wstring errorMsg = L"D3D12 Error: " + HrToString(hr_);   \
        MessageBoxW(nullptr, errorMsg.c_str(), L"Error", MB_OK | MB_ICONERROR); \
        ExitProcess(1);                                                \
    }                                                                  \
}
