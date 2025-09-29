#include "Application.h"
#include <memory>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Unused parameters
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Create application instance
    const uint32_t windowWidth = 1280;
    const uint32_t windowHeight = 720;
    std::unique_ptr<Application> application = std::make_unique<Application>(windowWidth, windowHeight, L"D3D12 Mini Pathtracer");
    
    // Parse command line arguments
    application->ParseCommandLineArgs();

    // Run application
    int result = application->Run();
    
    return result;
}