#include "EngineClassData.hpp"
#include "GameObject.hpp"

namespace Blokk
{

GameObject::GameObject(ObjectCreationParams CP)
    :
    IsStatic(CP.velocity == 0),
    EngineIdx(0),
    IsVisible(true)
{
    Engine->ProcessAddCommand(
        EngineObjectCreationParams{
            CP.visible, // is visible?
            CP.velocity, // velocity
            CP.position, // position
            *this // instance
        }
    );
}

GameObject::GameObject(
    vec2<float> velocity,
    vec2<float> position,
    bool visible
)
    :
    IsStatic(velocity.x == 0 && velocity.y == 0),
    EngineIdx(0),
    IsVisible(true)
{
    Engine->ProcessAddCommand(
        visible,
        velocity,
        position,
        this
    );
}

GameObject::~GameObject()
{
    if (IsStatic) {
        Engine->DestroyStaticObject(EngineIdx);
    }
    else {
        Engine->DestroyDynamicObject(EngineIdx);
    }
}

/* Set the object to static
Assumes the object was dynamic at first */
void GameObject::SetToStatic()
{
    IsStatic = true;

    // Remove velocity from engine data --------------------------------------------
    Engine->SwapDynamicObjects(EngineIdx, Engine->m_ObjectCount.Dynamic - 1); // Swap with last dynamic
    Engine->SwapStaticObjects(EngineIdx, Engine->m_ObjectCount.Total - 1); // Swap with last static
    // Remove velocity
    Engine->m_XVelocities.pop_back(); 
    Engine->m_YVelocities.pop_back(); 

    // Update engine vars
    Engine->m_ObjectCount.Total++;
    Engine->m_ObjectCount.Dynamic--;
    Engine->m_ObjectCount.Static++;
}

// Set the object to dynamic
// Assumes the object was static at first
void GameObject::SetToDynamic(vec2<float> Vel)
{
    IsStatic = false;

    // Swap with first static object
    Engine->SwapStaticObjects(EngineIdx, Engine->m_ObjectCount.Dynamic);

    // Add velocity to engine data
    Engine->m_XVelocities.push_back(Vel.x);
    Engine->m_YVelocities.push_back(Vel.y);

    // Update engine vars
    Engine->m_ObjectCount.Total++;
    Engine->m_ObjectCount.Dynamic++;
    Engine->m_ObjectCount.Static--;
}

}