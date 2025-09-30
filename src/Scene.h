#pragma once

#include <d3d12.h>
#include <directxmath.h>
#include <wrl/client.h>
#include <memory>
#include <vector>

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
    ID3D12Resource* GetTLAS() const { return m_topLevelAS.Get(); }
    ID3D12Resource* GetBLAS() const { return m_bottomLevelAS.Get(); }
    ID3D12Resource* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
    ID3D12Resource* GetIndexBuffer() const { return m_indexBuffer.Get(); }
    
    
    // Get vertex and index counts
    uint32_t GetVertexCount() const { return m_vertexCount; }
    uint32_t GetIndexCount() const { return m_indexCount; }

private:
    // Device reference (not owned)
    ID3D12Device5* m_device;
    
    // Acceleration structures
    ComPtr<ID3D12Resource> m_topLevelAS;
    ComPtr<ID3D12Resource> m_bottomLevelAS;
    
    // Geometry buffers
    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_indexBuffer;
    
    // Geometry info
    uint32_t m_vertexCount;
    uint32_t m_indexCount;
    
    // Build flags
    bool m_isBuilt;
    
    // Temporary resources for AS build (must be kept alive until GPU finishes)
    ComPtr<ID3D12Resource> m_blasScratchBuffer;
    ComPtr<ID3D12Resource> m_tlasScratchBuffer;
    ComPtr<ID3D12Resource> m_instanceDescBuffer;
    
    // Post-build info buffers (GPU writable)
    ComPtr<ID3D12Resource> m_blasPostBuildInfoBuffer;
    ComPtr<ID3D12Resource> m_tlasPostBuildInfoBuffer;
    
    // Readback buffers for post-build info
    ComPtr<ID3D12Resource> m_blasPostBuildInfoReadback;
    ComPtr<ID3D12Resource> m_tlasPostBuildInfoReadback;
    
    // Private methods
    void CreateCornellBoxGeometry();
    void CreateBottomLevelAS(ID3D12GraphicsCommandList4* commandList);
    void CreateTopLevelAS(ID3D12GraphicsCommandList4* commandList);
    void ReadbackPostBuildInfo();
    
    // Helper method to wait for GPU completion
    void WaitForGPU(ID3D12CommandQueue* commandQueue);
};
