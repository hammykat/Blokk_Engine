#include <utility>
#include <vector>

#include "EngineClassData.hpp"
#include "GameObject.hpp"
#include "types.hpp"

namespace Blokk {

void EngineCore::DestroyStaticObject(uint32_t ObjIdx)
{
    // Swap with the last item
    SwapStaticObjects(ObjIdx, m_ObjectCount.Total - 1);

    // Remove the object
    m_XPositions.pop_back();
    m_YPositions.pop_back();
    m_ObjectInstances.pop_back();

    m_ObjectCount.Static--;
    m_ObjectCount.Total--;
}

void EngineCore::DestroyDynamicObject(uint32_t ObjIdx)
{
    // Swap with the last item
    SwapDynamicObjects(ObjIdx, m_ObjectCount.Dynamic - 1);

    if(m_ObjectCount.Static > 0)
    {
        // Swap with the last static item
        SwapStaticObjects(m_ObjectCount.Dynamic - 1, m_ObjectCount.Total - 1);
    }

    // Remove the object from the back
    m_XPositions.pop_back();
    m_YPositions.pop_back();
    m_XVelocities.pop_back();
    m_YVelocities.pop_back();
    m_ObjectInstances.pop_back();

    m_ObjectCount.Dynamic--;
    m_ObjectCount.Total--;
}

void EngineCore::ProcessAddCommand(bool Visible, vec2<float> Velocity, vec2<float> Position, GameObject* Object)
{
    if (Object == nullptr) {
        throw std::runtime_error("BlokkForge: GameObject pointer passed as nullptr");
    }

    float XVel = Velocity.x;
    float YVel = Velocity.y;

    // If creating a static object
    if(XVel == 0 && YVel == 0)
    {
        Object->EngineIdx = m_XPositions.size();

        // Add position
        m_XPositions.push_back(Position.x);
        m_YPositions.push_back(Position.y);
        
        // Add instance pointer
        m_ObjectInstances.push_back(Object);

        #ifdef Blokk_Rendering_Enabled
        // Add visibility
        IsVisible.push_back(Visible);
        #endif

        m_ObjectCount.Static++;
    } 
    else // If creating a dynamic object
    {
        // Add position
        m_XPositions.push_back(Position.x);
        m_YPositions.push_back(Position.y);

        // Add instance pointer
        m_ObjectInstances.push_back(Object);

        #ifdef Blokk_Rendering_Enabled
        // Add visibility
        IsVisible.push_back(Visible);
        #endif

        if(m_ObjectCount.Static > 0)
        {
            // Swap position with first static member to keep with dynamic object data
            SwapStaticObjects(m_ObjectCount.Dynamic, m_ObjectCount.Total - 1);
        } 
        else {
            // Add index
            Object->EngineIdx = m_ObjectCount.Dynamic;
        }

        // Add velocity
        m_XVelocities.push_back(XVel);
        m_YVelocities.push_back(YVel);

        m_ObjectCount.Dynamic++;
    }

    m_ObjectCount.Total++;
}

void EngineCore::ProcessAddCommand(EngineObjectCreationParams Fields) {
    ProcessAddCommand(Fields.Velocity, Fields.Position, Fields.Object, Fields.Visible);
}

void EngineCore::SwapStaticObjects(uint32_t Obj1, uint32_t Obj2)
{
    // Swap X and Y
    std::swap(m_XPositions[Obj1], m_XPositions[Obj2]);
    std::swap(m_YPositions[Obj1], m_YPositions[Obj2]);

    // Swap instance pointers
    std::swap(m_ObjectInstances[Obj1], m_ObjectInstances[Obj2]);

    #ifdef Blokk_Rendering_Enabled
    // Swap visibility
    std::swap(IsVisible[Obj1], IsVisible[Obj2]);
    #endif

    // Update object's internal vars
    m_ObjectInstances[Obj1]->EngineIdx = Obj1;
    m_ObjectInstances[Obj2]->EngineIdx = Obj2;
}

void EngineCore::SwapDynamicObjects(uint32_t Obj1, uint32_t Obj2)
{
    // Swap X and Y
    std::swap(m_XPositions[Obj1], m_XPositions[Obj2]);
    std::swap(m_YPositions[Obj1], m_YPositions[Obj2]);

    // Swap velocities
    std::swap(m_XVelocities[Obj1], m_XVelocities[Obj2]);
    std::swap(m_YVelocities[Obj1], m_YVelocities[Obj2]);

    #ifdef Blokk_Rendering_Enabled
    // Swap visibility
    std::swap(IsVisible[Obj1], IsVisible[Obj2]);
    #endif

    // Swap instance pointers
    std::swap(m_ObjectInstances[Obj1], m_ObjectInstances[Obj2]);

    // Update object's internal vars
    m_ObjectInstances[Obj1]->EngineIdx = Obj1;
    m_ObjectInstances[Obj2]->EngineIdx = Obj2;
}

}