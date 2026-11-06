#include "EngineClassData.hpp"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

namespace Blokk {

class Renderer
{
public:
    friend class EngineCore;

    Renderer(EngineCore& Manager_, SDL_Window* Window_) :
        Manager(Manager_),
        Window(Window_)
    {
        SDLRenderer = SDL_CreateRenderer(Window, nullptr);

        if (!SDLRenderer)
        {
            throw std::runtime_error(
                std::string("Failed to create renderer: ") + SDL_GetError()
            );
        }
    }

    ~Renderer()
    {
        for (auto& Animation : Manager.Frames)
        {
            for (SDL_Texture* Texture : Animation)
            {
                SDL_DestroyTexture(Texture);
            }
        }

        SDL_DestroyRenderer(SDLRenderer);
        SDL_DestroyWindow(Window);
    }

    void init()
    {
        // 1. Define 4 points of a square (X, Y coordinates from -1.0 to 1.0)
        float vertices[] = {
            -0.5f,  0.5f,  // Top Left
            -0.5f, -0.5f,  // Bottom Left
             0.5f, -0.5f,  // Bottom Right
             0.5f,  0.5f   // Top Right
        };

        // Indices connect the dots to make two triangles forming our quad
        unsigned int indices[] = {
            0, 1, 3, // First Triangle
            1, 2, 3  // Second Triangle
        };

        // 2. Generate a Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
        // This is just a unique integer address ID that points to a slot in GPU memory
        unsigned int VAO, VBO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // 3. Bind and upload the data arrays directly across the PCIe bus into GPU memory
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // 4. Tell OpenGL how to read our data layout (2 floats per position item)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Unbind our structures cleanly so we don't accidentally overwrite them
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void RenderObjects()
    {
        for (auto Idx : Manager.RenderObjectIdxs)
        {
            uint32_t Anim = Manager.AnimNums[Idx];
            uint32_t FrameCount = Manager.AnimFrameCounts[Anim];

            if (FrameCount == 0)
                continue;

            uint32_t Frame = Manager.FrameNums[Idx] % FrameCount;

            SDL_Texture* Texture =
                Manager.Frames[Anim][Frame];

            #ifdef Blokk_CamEnabled
                float x =
                    static_cast<float>(
                        Manager.XPositions[Idx] -
                        Manager.Camera.GetXPosition()
                    );

                float y =
                    static_cast<float>(
                        Manager.YPositions[Idx] -
                        Manager.Camera.GetYPosition()
                    );
            #else
                float x =
                    static_cast<float>(Manager.XPositions[Idx]);

                float y =
                    static_cast<float>(Manager.YPositions[Idx]);
            #endif

            SDL_FRect Destination{
                x,
                y,
                static_cast<float>(Manager.FrameWidths[Anim][Frame]),
                static_cast<float>(Manager.FrameHeights[Anim][Frame])
            };

            SDL_RenderTexture(
                SDLRenderer,
                Texture,
                nullptr,
                &Destination
            );
        }
    }

    void SetClearColor(
        uint8_t R,
        uint8_t G,
        uint8_t B,
        uint8_t A = 255
    )
    {
        SDL_SetRenderDrawColor(SDLRenderer, R, G, B, A );
    }

    
    [[nodiscard]] 
    double TimeRenderObjects()
    {
        // Start time
        auto StartTime = std::chrono::steady_clock::now();

        RenderObjects();

        // End time
        auto EndTime = std::chrono::steady_clock::now();

        // Calculate total time
        auto TotalTime =
            std::chrono::duration<double, std::milli>(
                EndTime - StartTime
            ).count();

        #ifdef Blokk_Diagnostics
            Manager.RenderTime = TotalTime;
        #endif

        return TotalTime;
    }

    void ClearScreen() {
        SDL_RenderClear(SDLRenderer);
    }

    void Present()
    {
        SDL_RenderPresent(SDLRenderer);
    }

private:

    EngineCore& Manager;
    SDL_Window* Window;
    SDL_Renderer* SDLRenderer;

    SDL_Texture* LoadTexture(const std::string& FilePath)
    {
        SDL_Texture* Texture =
            IMG_LoadTexture(SDLRenderer, FilePath.c_str());

        if (!Texture)
        {
            throw std::runtime_error(
                std::string("Failed to load texture: ") + FilePath +
                " - " + SDL_GetError()
            );
        }

        return Texture;
    }

public:

    void CreateNewEmptyAnimation(const std::string& Name)
    {
        Manager.AnimNames[Name] = Manager.Frames.size();

        Manager.Frames.emplace_back();
        Manager.FrameWidths.emplace_back();
        Manager.FrameHeights.emplace_back();
        Manager.AnimFrameCounts.push_back(0);
    }

    void AddFrameToAnimation(
        const std::string& Name,
        const std::string& FilePath
    )
    {
        uint32_t AnimIdx = Manager.AnimNames[Name];

        SDL_Texture* Texture = LoadTexture(FilePath);

        Manager.Frames[AnimIdx].push_back(Texture);

        float Width;
        float Height;

        SDL_GetTextureSize(
            Texture,
            &Width,
            &Height
        );

        Manager.FrameWidths[AnimIdx].push_back(
            static_cast<uint32_t>(Width)
        );

        Manager.FrameHeights[AnimIdx].push_back(
            static_cast<uint32_t>(Height)
        );

        Manager.AnimFrameCounts[AnimIdx]++;
    }

    void CreateAnimation(
        const std::string& Name,
        const std::vector<std::string>& FilePaths
    )
    {
        CreateNewEmptyAnimation(Name);

        for (const std::string& FilePath : FilePaths)
        {
            AddFrameToAnimation(Name, FilePath);
        }
    }
};


} // namespace Blokk
