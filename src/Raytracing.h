#pragma once

#include <d3d12.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <string>
#include <memory>
#include <vector>

using Microsoft::WRL::ComPtr;

class Scene;

class Raytracing
{
public:
    Raytracing();
    ~Raytracing();
    
    // Initialize raytracing pipeline
    void Initialize(ID3D12Device5* device, uint32_t width, uint32_t height, uint32_t swapChainBufferCount);
    
    // Update descriptor heap with scene resources
    void UpdateDescriptorHeap(Scene* scene, uint32_t frameIndex);
    
    // Render the scene using raytracing
    void Render(ID3D12GraphicsCommandList4* commandList, uint32_t frameIndex);
    
    // Copy raytracing output to render target
    void CopyToRenderTarget(ID3D12GraphicsCommandList4* commandList, ID3D12Resource* renderTarget);
    
    // Resize output buffer
    void Resize(uint32_t width, uint32_t height);
    
    // Getters
    ID3D12Resource* GetOutputResource() const { return m_raytracingOutput.Get(); }
    
private:
    // Helper functions
    void CreateRaytracingPipeline();
    void CreateDescriptorHeap();
    void CreateRaytracingOutputResource();
    void CreateShaderTable();
    ComPtr<IDxcBlob> CompileShader(const std::wstring& filename, const std::wstring& entryPoint, const std::wstring& target);
    
private:
    enum DescHeapEntries : uint32_t {
        SRV_TLAS = 0,
        UAV_Output,
        Count
    };

    // Device reference (not owned)
    ID3D12Device5* m_device;
    
    // Dimensions
    uint32_t m_width;
    uint32_t m_height;
    
    // DXR pipeline objects
    ComPtr<ID3D12StateObject> m_rtPipelineState;
    ComPtr<ID3D12RootSignature> m_rtGlobalRootSignature;
    
    // Resources
    ComPtr<ID3D12Resource> m_raytracingOutput;
    std::vector<ComPtr<ID3D12DescriptorHeap>> m_descHeaps;
    uint32_t m_CBVSRVUAVdescHeapSize;
    ComPtr<ID3D12Resource> m_shaderTable;
    uint32_t m_shaderTableEntrySize;
    uint32_t m_swapChainBufferCount;
};
