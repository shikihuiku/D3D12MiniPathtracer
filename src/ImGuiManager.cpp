#include "ImGuiManager.h"
#include "Helper.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#include <cassert>
#include <vector>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Simple free list based allocator
struct ImGuiManager::DescriptorHeapAllocator
{
    ComPtr<ID3D12DescriptorHeap> m_heap;
    D3D12_DESCRIPTOR_HEAP_TYPE  m_heapType = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_heapStartCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_heapStartGpu = {};
    UINT                        m_heapHandleIncrement = {};
    std::vector<int>            m_freeIndices = {};

    void Create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t num)
    {
        assert(m_heap.Get() == nullptr && m_freeIndices.empty());
        
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = num;
        srvHeapDesc.Type = type;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_heap)));

        // Set debug name
        m_heap->SetName(L"ImGui Heap");

        D3D12_DESCRIPTOR_HEAP_DESC desc = m_heap->GetDesc();
        m_heapType = desc.Type;
        m_heapStartCpu = m_heap->GetCPUDescriptorHandleForHeapStart();
        m_heapStartGpu = m_heap->GetGPUDescriptorHandleForHeapStart();
        m_heapHandleIncrement = device->GetDescriptorHandleIncrementSize(m_heapType);
        m_freeIndices.reserve((int)desc.NumDescriptors);
        for (int n = desc.NumDescriptors; n > 0; n--)
            m_freeIndices.push_back(n - 1);
    }
    
    void Destroy()
    {
        m_heap.Reset();
        m_freeIndices.clear();
    }

    ComPtr<ID3D12DescriptorHeap>& Heap() { return m_heap; }

    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
    {
        assert(m_freeIndices.size() > 0);
        int idx = m_freeIndices.back();
        m_freeIndices.pop_back();
        out_cpu_desc_handle->ptr = m_heapStartCpu.ptr + (idx * m_heapHandleIncrement);
        out_gpu_desc_handle->ptr = m_heapStartGpu.ptr + (idx * m_heapHandleIncrement);
    }

    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
    {
        int cpu_idx = (int)((out_cpu_desc_handle.ptr - m_heapStartCpu.ptr) / m_heapHandleIncrement);
        int gpu_idx = (int)((out_gpu_desc_handle.ptr - m_heapStartGpu.ptr) / m_heapHandleIncrement);
        assert(cpu_idx == gpu_idx);
        m_freeIndices.push_back(cpu_idx);
    }
};

// Static member initialization
std::unique_ptr<ImGuiManager::DescriptorHeapAllocator> ImGuiManager::m_srvHeapAllcoator;

ImGuiManager::ImGuiManager()
{
}

ImGuiManager::~ImGuiManager()
{
    Shutdown();
}

void ImGuiManager::Initialize(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue *queue, uint32_t bufferCount, DXGI_FORMAT rtvFormat)
{
    if (m_initialized)
    {
        return;
    }

    m_bufferCount = bufferCount;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);

    m_srvHeapAllcoator = std::make_unique<DescriptorHeapAllocator>();
    m_srvHeapAllcoator->Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32);

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = device;
    init_info.CommandQueue = queue;
    init_info.NumFramesInFlight = static_cast<int>(bufferCount);
    init_info.RTVFormat = rtvFormat;
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
    // (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
    init_info.SrvDescriptorHeap = m_srvHeapAllcoator->Heap().Get();
    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return m_srvHeapAllcoator->Alloc(out_cpu_handle, out_gpu_handle); };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return m_srvHeapAllcoator->Free(cpu_handle, gpu_handle); };
    ImGui_ImplDX12_Init(&init_info);

    m_initialized = true;
}

void ImGuiManager::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;
}

void ImGuiManager::BeginFrame()
{
    if (!m_initialized)
    {
        return;
    }

    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndFrame()
{
    if (!m_initialized)
    {
        return;
    }

    // Rendering
    ImGui::Render();
}

void ImGuiManager::Render(ID3D12GraphicsCommandList* commandList)
{
    if (!m_initialized)
    {
        return;
    }

    // Set descriptor heaps
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvHeapAllcoator->Heap().Get() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // Render ImGui
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

LRESULT ImGuiManager::WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!m_initialized)
    {
        return 0;
    }

    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
}

bool ImGuiManager::WantCaptureMouse() const
{
    if (!m_initialized)
    {
        return false;
    }

    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiManager::WantCaptureKeyboard() const
{
    if (!m_initialized)
    {
        return false;
    }

    return ImGui::GetIO().WantCaptureKeyboard;
}
