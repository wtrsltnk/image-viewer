#ifndef APPLICATION_H
#define APPLICATION_H

#include <filesystem>
#include <tuple>
#include <vector>

class AbstractApplication
{
public:
    virtual ~AbstractApplication();
    
    static void ActivateLoadingContext();
    static void DeactivateLoadingContext();
        
    virtual bool Setup(
        const std::filesystem::path &imagefile,
        const std::vector<std::tuple<int, int, int, int>> &windowRects) = 0;
    virtual void Render3d() = 0;
    virtual void Render2d() = 0;
    virtual void Cleanup() = 0;
    
    virtual void OnResize(
        int w,
        int h) = 0;
    virtual void OnPreviousPressed() = 0;
    virtual void OnNextPressed() = 0;
    virtual void OnHomePressed() = 0;
    virtual void OnEndPressed() = 0;
    virtual void OnZoom(
        int amount) = 0;
    virtual void OnMousePositionChanged(
        int mousePositionX,
        int mousePositionY) = 0;
};

#endif // APPLICATION_H
