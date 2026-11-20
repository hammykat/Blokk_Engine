#include <cassert>
#include <iostream>

#include "types.hpp"

namespace Blokk {

    class Window {

    private:
        GLFWwindow* m_Window; // Pointer to window handle
        bool m_InitFailed; // If the initialization has failed
        bool m_Initialized; // If attempted to initialize

    public:
        friend class Renderer;

        Window(); // Initializes variables
        ~Window(); // Terminates GLFW, destroys window
        bool Init(); // Initializes GLFW, creates window, handles states

        bool ShouldClose(); // Returns if the window should be closed
        void Clear(Color c); // Clear the screen with a color
        void Update(); // Updates the window by rendering and polling events

        void Maximize(); // Maximizes the window on the current monitor
        void Minimize(); // Minimizes the window to the taskbar

        void Restore(); // Restores the window from being minimized or maximized
        void SetTitle(const char* title); // Sets the window's title

        uint32_t GetWidth(); // Get the width of the window
        uint32_t GetHeight(); // Get the height of the window
        glm::vec2 GetDimensions(); // Get the dimensions of the window
        float GetAspectRatio(); // Get the aspect ratio (width/height)

        // Gets the window handle (for manual control, other libraries, etc)
        GLFWwindow* GetNativeWindow() const;
    };
}
