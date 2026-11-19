#include <GLFW/glfw3.h>
#include <cassert>
#include <iostream>

#include "Window.hpp"
#include "types.hpp"

namespace Blokk {
    Window::Window() {
        m_Window = nullptr;
        m_Initialized = false;
        m_InitFailed = false;
    }

    Window::~Window() {
        if (m_Window != nullptr) 
            glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    bool Window::Init() {

        assert(!m_InitFailed && "Blokk: GLFW Initialization failed, but tried to re-initialize");
        if (m_Initialized) {
            return true;
        }

        // Initialize GLFW
        if (!glfwInit()) { // Return error if fails
            std::cerr << "Blokk: Failed to initialize GLFW \n";
            m_InitFailed = true;
            return false;
        }

        // Create the actual window
        m_Window = glfwCreateWindow(800, 600, "Blokk Engine", NULL, NULL);
        if (!m_Window) { // If fails return an error and terminate
            std::cerr << "Blokk: Failed to create GLFW window \n";
            glfwTerminate();
            m_InitFailed = true;
            return false;
        }

        //-------------------------------------------------------------------
        m_Initialized = true;
        return true;
    }

    // Returns if the window should be closed
    bool Window::ShouldClose() {
        // Initialize, if fails, throw an error
        assert(Init() && "Blokk: GLFW Initialization failed, but tried to run function");

        // Get value from GLFW
        return glfwWindowShouldClose(m_Window);
    }


    // Updates the window by rendering and polling events
    // TODO: Allow user to handle the events and do stuff with them
    void Window::Update() {
        assert(m_Initialized && !m_InitFailed && "Blokk Error: Cannot update an uninitialized window!");
        glfwSwapBuffers(m_Window); // Flipped drawing onto the screen
        glfwPollEvents(); // Checks for keys, dragging, and resizing
    }


    // Instantly maximizes the window on the current monitor
    void Window::Maximize() {
        assert(m_Window != nullptr);
        glfwMaximizeWindow(m_Window);
    }

    // Minimizes the window to the taskbar
    void Window::Minimize() {
        assert(m_Window != nullptr);
        glfwIconifyWindow(m_Window);
    }

    // Restores the window from being minimized or maximized
    void Window::Restore() {
        assert(m_Window != nullptr);
        glfwRestoreWindow(m_Window);
    }
        
    // Sets the window's title
    void Window::SetTitle(const char* title)
    {
        assert(m_Initialized && m_Window != nullptr);
        glfwSetWindowTitle(m_Window, title);
    }

    // Get the width of the window
    uint32_t Window::GetWidth() {
        assert(m_Window != nullptr && "Blokk: Window is not correctly initialized before using Window::getWidth");
        int32_t width;
        glfwGetWindowSize(m_Window, &width, nullptr);
        return width;
    }

    // Get the height of the window
    uint32_t Window::GetHeight() {
        assert(m_Window != nullptr && "Blokk: Window is not correctly initialized before using Window::getHeight");
        int32_t height;
        glfwGetWindowSize(m_Window, nullptr, &height);
        return height;
    }

    // Get the dimensions of the window
    glm::vec2 Window::GetDimensions() {
        assert(m_Window != nullptr && "Blokk: Window is not correctly initialized before using Window::getDimensions");
        glm::ivec2 dimensions{};
        glfwGetWindowSize(m_Window, &dimensions.x, &dimensions.y);
        return dimensions;
    }

    // Get the aspect ratio (width/height)
    float Window::GetAspectRatio() {
        assert(m_Window != nullptr && "Blokk: Window is not correctly initialized before using Window::getAspectRatio");
        glm::vec2 dimensions = GetDimensions();
        return static_cast<float>(dimensions.x) / static_cast<float>(dimensions.y);
    }

    // Gets the window handle (for manual control, other libraries, etc)
    GLFWwindow* Window::GetNativeWindow() const {
        assert(m_Window != nullptr);
        return m_Window;
    }
}
