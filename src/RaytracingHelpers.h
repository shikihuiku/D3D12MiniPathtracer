#pragma once

#include <d3d12.h>
#include <directxmath.h>
#include <vector>

using namespace DirectX;

// Vertex structure for Cornell Box
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT4 color;
};

