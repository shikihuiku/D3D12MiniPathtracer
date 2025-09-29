#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <memory>

// Use Microsoft::WRL::ComPtr
using Microsoft::WRL::ComPtr;

class ImGuiManager
{
private:
    // Forward declaration of internal allocator
    struct DescriptorHeapAllocator;
    
    // SRV heap for ImGui font texture
    static std::unique_ptr<DescriptorHeapAllocator> m_srvHeapAllcoator;

public:
    ImGuiManager();
    ~ImGuiManager();

    // Initialize ImGui with D3D12 device and window handle
    void Initialize(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* queue, uint32_t bufferCount, DXGI_FORMAT rtvFormat);
    
    // Shutdown ImGui
    void Shutdown();
    
    // Begin new ImGui frame
    void BeginFrame();
    
    // End ImGui frame (builds draw data)
    void EndFrame();
    
    // Render ImGui draw data
    void Render(ID3D12GraphicsCommandList* commandList);
    
    // Handle Win32 messages
    LRESULT WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Check if ImGui wants to capture input
    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;

private:
   
    // Flag to track initialization state
    bool m_initialized = false;
    
    // Number of swap chain buffers
    uint32_t m_bufferCount = 0;
};
