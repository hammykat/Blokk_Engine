#include "GameObject.hpp" 
#include "EngineClassData.hpp"
 
namespace Blokk { 
 
// Set 
void GameObject::SetPosition(float TargetX, float TargetY)  
{ 
    // Update X and Y 
    Engine->m_XPositions[EngineIdx] = TargetX;
    Engine->m_YPositions[EngineIdx] = TargetY;
} 

void GameObject::SetPosition(vector2<float> TargetPos) 
{ 
    SetPosition(TargetPos.x, TargetPos.y); 
} 
 
void GameObject::SetXPosition(float Target)  
{ 
    Engine->m_XPositions[EngineIdx] = Target;
} 

void GameObject::SetYPosition(float Target)  
{ 
    Engine->m_YPositions[EngineIdx] = Target;
} 
 
// Get 
vector2<float> GameObject::GetPosition()  
{ 
    return vector2<float>( 
        Engine->m_XPositions[EngineIdx], 
        Engine->m_YPositions[EngineIdx] 
    ); 
} 
 
// Change 
void GameObject::ChangePosition(float ChangeX, float ChangeY)  
{ 
    Engine->m_XPositions[EngineIdx] += ChangeX;
    Engine->m_YPositions[EngineIdx] += ChangeY;
} 

void GameObject::ChangePosition(vector2<float> Change) 
{ 
    ChangePosition(Change.x, Change.y); 
} 
 
void GameObject::ChangeXPosition(float X)  
{ 
    Engine->m_XPositions[EngineIdx] += X;
} 

void GameObject::ChangeYPosition(float Y)  
{ 
    Engine->m_YPositions[EngineIdx] += Y;
} 
 
}