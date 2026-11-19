#include "EngineClassData.hpp"
#include "GameObject.hpp"

#include <cmath>


namespace Blokk {

// Set
void GameObject::SetVelocity(float xvel, float YVel) 
{
    // If being set to static
    if(xvel == 0 && YVel == 0) {

        // If Dynamic
        if(!IsStatic) 
        {
            // Set to static
            SetToStatic();
        }
    } 
    else // If being set to dynamic 
    {
        // If static
        if(IsStatic) 
        {
            // Set to dynamic
            SetToDynamic(vec2<float>{xvel, YVel});
        }
        else // if dynamic
        {
            // Set X and Y
            Engine->m_XVelocities[EngineIdx] = xvel;
            Engine->m_YVelocities[EngineIdx] = YVel;
        }
    }
}

void GameObject::SetVelocity(vec2<float> Velocity) 
{
    SetVelocity(Velocity.x, Velocity.y);
}

void GameObject::SetVelocityX(float XVel) 
{
    // If being set to static
    if(XVel == 0) 
    {
        // If static
        if(IsStatic) 
        {
            // Do nothing
            return;
        } 
        else // If dynamic
        {
            // If being set to static
            if(GetVelocityY() == 0) 
            {
                // Set to static
                SetToStatic();
            }
            else // If staying dynamic
            {
                // Set X
                Engine->m_XVelocities[EngineIdx] = XVel;
            }
        }
    } 
    else // If being set to dynamic 
    {
        // If static
        if(IsStatic) 
        {
            // Set to dynamic with vel
            SetToDynamic(vec2<float>(XVel, 0));
        }
        else // If dynamic
        {
            // Set X
            Engine->m_XVelocities[EngineIdx] = XVel;
        }
    }
}

void GameObject::SetVelocityY(float YVel) 
{
    // If being set to static
    if(YVel == 0) 
    {
        // If static
        if(IsStatic) 
        {
            // Do nothing
            return;
        } 
        else // If dynamic
        {
            // If being set to static
            if(GetVelocityX() == 0) 
            {
                // Set to static
                SetToStatic();
            }
            else // If staying dynamic
            {
                // Set Y
                Engine->m_YVelocities[EngineIdx] = YVel;
            }
        }
    } 
    else // If being set to dynamic 
    {
        // If static
        if(IsStatic) 
        {
            // Set to dynamic with vel
            SetToDynamic(vec2<float>(0, YVel));
        }
        else // If dynamic
        {
            // Set Y
            Engine->m_YVelocities[EngineIdx] = YVel;
        }
    }
}

// Get
vec2<float> GameObject::GetVelocity() const
{
    // If the object is static
    if(IsStatic)
    {
        // Give the static velocity (Always 0)
        return vec2<float>(0, 0);
    }
    else // If the object is dynamic
    {
        // Get the velocity from the manager
        return {
            Engine->m_XVelocities[EngineIdx], 
            Engine->m_YVelocities[EngineIdx]
        };
    }
}

float GameObject::GetVelocityX() const 
{
    // If the object is static
    if(IsStatic)
    {
        return 0;
    }
    else // If the object is dynamic
    {
        // Get the vel from the manager's data
        return Engine->m_XVelocities[EngineIdx];
    }
}

float GameObject::GetVelocityY() const 
{
    // If the object is static
    if(IsStatic)
    {
        return 0;
    }
    else // If the object is dynamic
    {
        // Get the vel from the manager's data
        return Engine->m_YVelocities[EngineIdx];
    }
}

// Get the speed
float GameObject::GetSpeed() const
{
    // If static return 0, not moving
    if(IsStatic) return 0;

    float XVel = Engine->m_XVelocities[EngineIdx];
    float YVel = Engine->m_YVelocities[EngineIdx];

    return sqrt(XVel * XVel + YVel * YVel);
}

float GameObject::GetSpeedSquared() const 
{
    // If static return 0, not moving
    if(IsStatic) return 0;

    float XVel = Engine->m_XVelocities[EngineIdx];
    float YVel = Engine->m_YVelocities[EngineIdx];

    return XVel * XVel + YVel * YVel;
}

// If not moving, vel != 0
bool GameObject::IsMoving() const
{
    return !IsStatic;
}

// Stop, set to 0
void GameObject::StopVelocity() const
{
    // Remove its idx if present
    if(IsStatic)
    {
        return;
    }
    else // If dynamic
    {
        // Set to static
        SetToStatic();
    }
}

// Reverse
void GameObject::ReverseVelocity() const
{
    // If static do nothing
    if(IsStatic) return;

    // Reverse X and Y
    Engine->m_XVelocities[EngineIdx] *= -1;
    Engine->m_YVelocities[EngineIdx] *= -1;
}

void GameObject::ReverseVelocityX() const
{
    // If static do nothing
    if(IsStatic) return;

    // Reverse X
    Engine->m_XVelocities[EngineIdx] *= -1;
}

void GameObject::ReverseVelocityY() const
{
    // If static do nothing
    if(IsStatic) return;

    // Reverse Y
    Engine->m_YVelocities[EngineIdx] *= -1;
}

// Multiply ------------------------------------------------

void GameObject::MultiplyVelocity(float FactorX, float FactorY)
{
    // Return if 0
    if(IsStatic)
        return;

    // Find new X and Y
    float NewX = GetVelocityX() * FactorX;
    float NewY = GetVelocityY() * FactorY;

    if(NewX == 0 && NewY == 0)
    {
        SetToStatic();
        return;
    }

    // Update data
    Engine->m_XVelocities[EngineIdx] *= FactorX;
    Engine->m_YVelocities[EngineIdx] *= FactorY;
}

void GameObject::MultiplyVelocity(vec2<float> Factor) 
{
    MultiplyVelocity(Factor.x, Factor.y);
}

void GameObject::MultiplyVelocity(float Factor) 
{
    MultiplyVelocity(Factor, Factor);
}

// Individual
void GameObject::MultiplyXVelocity(float Factor) 
{
    MultiplyVelocity(Factor, 1);
}

void GameObject::MultiplyYVelocity(float Factor) 
{
    MultiplyVelocity(1, Factor);
}

// Add --------------------------------------------------

void GameObject::AddVelocity(float XVel, float YVel) 
{
    // Don't do anything if not changing
    if(XVel == 0 && YVel == 0) return;

    // If the object is static
    if(IsStatic)
    {
        // Set to dynamic
        SetToDynamic(vec2<float>{XVel, YVel});
    }
    else // If the object is dynamic
    {
        // Add
        Engine->m_XVelocities[EngineIdx] += XVel;
        Engine->m_YVelocities[EngineIdx] += YVel;
    }
}

void GameObject::AddVelocity(vec2<float> Vel) 
{
    AddVelocity(Vel.x, Vel.y);
}

// Divide -----------------------------------------------------

void GameObject::DivideVelocity(float XVel, float YVel) const
{
    // Avoid bugs with dividing by 0
    if(XVel == 0 || YVel == 0 || IsStatic) return;

    // Divide
    Engine->m_XVelocities[EngineIdx] /= XVel;
    Engine->m_YVelocities[EngineIdx] /= YVel;
}

void GameObject::DivideVelocity(vec2<float> Vel) const
{
    DivideVelocity(Vel.x, Vel.y);
}

void GameObject::DivideVelocityX(float Vel) const
{
    DivideVelocity(Vel, 1);
}

void GameObject::DivideVelocityY(float Vel) const 
{
    DivideVelocity(1, Vel);
}

}