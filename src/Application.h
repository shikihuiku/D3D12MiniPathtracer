#pragma once

#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <memory>

// D3D12 headers
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <directxmath.h>

// D3D12 helper headers
#include <wrl/client.h>

// Use Microsoft::WRL::ComPtr
using Microsoft::WRL::ComPtr;

// Forward declarations
class ImGuiManager;

class Application
{
public:
    Application(uint32_t width, uint32_t height, const std::wstring& name);
    virtual ~Application();

    // Initialize the application
    void OnInit();
    
    // Update frame
    void OnUpdate();
    
    // Render frame
    void OnRender();
    
    // Clean up resources
    void OnDestroy();

    // Window message handler
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Accessors
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    const WCHAR* GetTitle() const { return m_title.c_str(); }

    // Parse command line arguments
    void ParseCommandLineArgs();

    // Main loop
    int Run();

    // Check if application is still running
    bool IsRunning() const { return m_isRunning.load(); }

protected:
    // Message pump thread function
    void MessagePumpThread();

    // Create window (called from message pump thread)
    void CreateApplicationWindow();

    // Window handles
    HWND m_hwnd;
    
    // Window dimensions
    uint32_t m_width;
    uint32_t m_height;
    float m_aspectRatio;

    // Window title
    std::wstring m_title;

    // Thread management
    std::thread m_messageThread;
    std::atomic<bool> m_isRunning;
    std::atomic<bool> m_windowCreated;
    
    // Synchronization
    std::mutex m_windowMutex;
    std::condition_variable m_windowCreatedCV;
    
    // Window instance handle
    HINSTANCE m_hInstance;

    // D3D12 objects
    static const uint32_t SWAP_CHAIN_BUFFER_COUNT = 3;
    
    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[SWAP_CHAIN_BUFFER_COUNT];
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    
    // Render targets
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12Resource> m_renderTargets[SWAP_CHAIN_BUFFER_COUNT];
    uint32_t m_rtvDescriptorSize;
    
    // Synchronization objects
    ComPtr<ID3D12Fence> m_fence;
    uint64_t m_fenceValues[SWAP_CHAIN_BUFFER_COUNT];
    HANDLE m_fenceEvent;
    uint32_t m_currentBackBufferIndex;
    
    // Viewport and scissor rect
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    
    // Frame counter for animations
    uint64_t m_frameCounter;
    
    // ImGui Manager
    std::unique_ptr<ImGuiManager> m_imguiManager;

private:
    // Helper functions
    void CreateDevice();
    void CreateCommandQueue();
    void CreateSwapChain();
    void CreateRtvDescriptorHeap();
    void CreateFrameResources();
    void CreateSynchronizationObjects();
    void WaitForGpu();
    void MoveToNextFrame();
    void ResizeSwapChain();
    void CleanupRenderTargets();
};

