// Global root signature
RaytracingAccelerationStructure Scene : register(t0, space0);
RWTexture2D<float4> RenderTarget : register(u0, space0);

// Ray payload structure
struct RayPayload
{
    float3 color;
};

// Ray attributes
struct RayAttributes
{
    float2 barycentrics;
};

// Vertex data
struct Vertex
{
    float3 position;
    float3 normal;
    float4 color;
};

// Ray generation shader
[shader("raygeneration")]
void RayGenShader()
{
    uint2 dispatchDim = DispatchRaysDimensions().xy;
    uint2 dispatchIndex = DispatchRaysIndex().xy;

    // Calculate normalized screen coordinates
    float2 screenCoord = (float2(dispatchIndex) + 0.5f) / float2(dispatchDim);
    screenCoord = screenCoord * 2.0f - 1.0f;
    screenCoord.y = -screenCoord.y; // Flip Y coordinate
    
    // Fixed camera data for testing
    float3 cameraPosition = float3(-10.0f, 3.0f, -5.0f);
    float3 cameraTarget = float3(0.0f, 0.0f, 0.0f);
    float3 cameraUp = float3(0.0f, 1.0f, 0.0f);
    
    // Calculate camera basis
    float3 forward = normalize(cameraTarget - cameraPosition);
    float3 right = normalize(cross(forward, cameraUp));
    float3 up = cross(right, forward);
    
    // Calculate ray direction
    float aspectRatio = float(dispatchDim.x) / float(dispatchDim.y);
    float fov = 45.0f * 3.14159265f / 180.0f; // 45 degrees in radians
    float tanHalfFov = tan(fov * 0.5f);
    
    float3 rayDirection = normalize(
        forward + 
        right * screenCoord.x * aspectRatio * tanHalfFov +
        up * screenCoord.y * tanHalfFov
    );
    
    // Setup ray
    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = rayDirection;
    ray.TMin = 0.001f;
    ray.TMax = 10000.0f;
    
    // Trace ray
    RayPayload payload = { float3(0.0f, 0.0f, 0.0f) };
    TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, ~0, 0, 1, 0, ray, payload);
    
    // Write to render target
    RenderTarget[dispatchIndex] = float4(payload.color, 1.0f);
}

// Closest hit shader
[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in RayAttributes attr)
{
    // Get hit information
    float3 barycentrics = float3(1.0f - attr.barycentrics.x - attr.barycentrics.y, 
                                 attr.barycentrics.x, 
                                 attr.barycentrics.y);
    
    // For now, just return the instance color
    // In a full implementation, you would access vertex data here
    float3 lightDir = normalize(float3(0.5f, 1.0f, 0.5f));
    float3 normal = float3(0.0f, 1.0f, 0.0f); // Placeholder normal
    
    // Simple diffuse lighting
    float NdotL = max(0.0f, dot(normal, lightDir));
    payload.color = float3(0.8f, 0.8f, 0.8f) * NdotL + float3(0.1f, 0.1f, 0.1f);
}

// Miss shader
[shader("miss")]
void MissShader(inout RayPayload payload)
{
    // Sky color
    payload.color = float3(0.2f, 0.4f, 0.6f);
}
