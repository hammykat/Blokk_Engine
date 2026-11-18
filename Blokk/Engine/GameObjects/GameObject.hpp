#pragma once

#include <cstdint>
#include <string>

// TODO: Clean up Cached and Uncached folders; nothing's in them!

#include "types.hpp"

namespace Blokk
{

class EngineCore;

class GameObject
{
public:

    friend class EngineCore;

    static inline EngineCore* Engine = nullptr;

    uint32_t EngineIdx;

    // Points to index in Engine->AnimNames
    uint32_t CurrentAnim; 

    bool IsStatic;
    bool IsVisible;
    bool IsAnimated;

    GameObject(
        ObjectCreationParams CP = {true, {0, 0}, {0, 0}}
    );

    GameObject(
        vec2<float> Velocity,
        vec2<float> Position,
        bool Visible = true
    );

    ~GameObject();

    void SetToStatic();
    void SetToDynamic(vec2<float> Vel);

    void SetVelocity(float XVel, float YVel);
    void SetVelocity(vec2<float> Velocity);
    void SetVelocityX(float XVel);
    void SetVelocityY(float YVel);

    vec2<float> GetVelocity();
    float GetVelocityX();
    float GetVelocityY();
    float GetSpeed();
    float GetSpeedSquared();
    bool IsMoving();
    void StopVelocity();

    void ReverseVelocity();
    void ReverseVelocityX();
    void ReverseVelocityY();

    void MultiplyVelocity(float FactorX, float FactorY);
    void MultiplyVelocity(vec2<float> Factor);
    void MultiplyVelocity(float Factor);

    void MultiplyXVelocity(float Factor);
    void MultiplyYVelocity(float Factor);

    void AddVelocity(float XVel, float YVel);
    void AddVelocity(vec2<float> Vel);

    void DivideVelocity(float XVel, float YVel);
    void DivideVelocity(vec2<float> Vel);
    void DivideVelocityX(float Vel);
    void DivideVelocityY(float Vel);

    // Set position
    void SetPosition(float TargetX, float TargetY);
    void SetPosition(vec2<float> TargetPos);
    void SetXPosition(float Target);
    void SetYPosition(float Target);

    vec2<float> GetPosition();

    // Change position
    void ChangePosition(vec2<float> Change);
    void ChangePosition(float ChangeX, float ChangeY);
    void ChangeXPosition(float X);
    void ChangeYPosition(float Y);

    // Visibility
    void Hide();
    void Show();
    bool IsCurrentlyVisible();
    void ToggleVisibility();

    void SetAnimation(std::string AnimName);
    void SetFrameNum(uint32_t FrameNum);

protected:
    // The function the user can override for the engine to run every frame
    virtual void _Update(float delta) {}

    // Sets the update priority level
    void SetUpdatePriorityLevel(const uint8_t level) const
    {
        // Priority levels are stored in the EngineCore, no use in storing as 
        // member in class (less memory usage). Stored inside UpdatePriorities (vector<uint8_t>)
        Engine->m_UpdatePriorities[EngineIdx] = level;
    }

};

};

