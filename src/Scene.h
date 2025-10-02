#pragma once

#include <d3d12.h>
#include <directxmath.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <HeapManager.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// Forward declaration
struct AccelerationStructureBuffers;

class Scene
{
public:
    Scene();
    ~Scene();

    // Initialize the scene with device
    void Initialize(ID3D12Device5* device);

    // Build acceleration structures
    void BuildAccelerationStructures(ID3D12GraphicsCommandList4* commandList,
                                   ID3D12CommandAllocator* commandAllocator,
                                   ID3D12CommandQueue* commandQueue);

    // Accessors
    // BLAS and TLAS should be allocated in the default heap
    D3D12_GPU_VIRTUAL_ADDRESS GetTLAS() const { return m_ASHeapManager.GetGPUVirtualAddress(m_topLevelASOffset); }
    D3D12_GPU_VIRTUAL_ADDRESS GetBLAS() const { return m_ASHeapManager.GetGPUVirtualAddress(m_bottomLevelASOffset); }
    
private:
    // Device reference (not owned)
    ID3D12Device5* m_device;

    HeapManager m_ASHeapManager;
    HeapManager m_defaultTemporaryHeapManager;
    HeapManager m_uploadTemporaryHeapManager;

    ReadbackHeapManager m_readbackHeapManager;
    
    // Acceleration structures
    uint32_t m_topLevelASOffset;
    uint32_t m_bottomLevelASOffset; 
   
    // Geometry buffers
    uint32_t m_vertexBufferOffset;
    uint32_t m_indexBufferOffset;
    
    // Geometry info
    uint32_t m_vertexCount;
    uint32_t m_indexCount;
    
    // Build flags
    bool m_isBuilt;

    // Temporary resources for AS build (must be kept alive until GPU finishes)
    uint32_t m_blasScratchBufferOffset;
    uint32_t m_tlasScratchBufferOffset;
    uint32_t m_instanceDescBufferOffset;

    // Post-build info buffers (GPU writable)
    uint32_t m_blasPostBuildInfoBufferOffset;
    uint32_t m_tlasPostBuildInfoBufferOffset;

    // Readback buffers for post-build info
    uint32_t m_blasPostBuildInfoReadbackOffset;
    uint32_t m_tlasPostBuildInfoReadbackOffset;
    
    // Private methods
    void CreateCornellBoxGeometry();
    void CreateBottomLevelAS(ID3D12GraphicsCommandList4* commandList);
    void CreateTopLevelAS(ID3D12GraphicsCommandList4* commandList);
    void ReadbackPostBuildInfo();
    void FreeTemporaryResources();
    
    // Helper method to wait for GPU completion
    void WaitForGPU(ID3D12CommandQueue* commandQueue);
};
