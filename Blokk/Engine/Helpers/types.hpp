#pragma once

#include <concepts>
#include <cstdint>
#include <iostream>
#include <stdbool.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#ifdef Blokk_Debug_Enabled
#define AssertWhenDebug(condition, msg) \
    do { if (!(condition)) throw std::runtime_error(msg); } while (0) 
#else 
// In Release mode, this turns into an empty statement and ignores 'msg' entirely
#define AssertWhenDebug(condition, msg) do { } while (0)
#endif

namespace Blokk {
   
template<typename T>
struct range {
    T Start, End;
    T GetSize() const {
        return End - Start + 1;
    }
};

struct IndexRange : range<uint32_t> {};

template <typename T = float>
    requires std::integral<T> || std::floating_point<T>
struct vec2 {
    T x, y;

    // Conversion to glm types
    operator glm::vec2() const {
        return glm::vec2(static_cast<float>(x), static_cast<float>(y));
    }

    // operator +
    vec2<T> operator+(T val) {
        return vec2<T>{x + val, y + val};
    }
    [[nodiscard]] inline
    vec2<T> operator+(vec2<T> val) {
        return vec2<T>{x + val.x, y + val.y};
    }
    
    // operator -
    [[nodiscard]] inline
    vec2<T> operator-(T val) {
        return vec2<T>{x - val, y - val};
    }
    [[nodiscard]] inline
    vec2<T> operator-(vec2<T> val) {
        return vec2<T>{x - val.x, y - val.y};
    }

    // operator *
    [[nodiscard]] inline
    vec2<T> operator*(T val) {
        return vec2<T>{x * val, y * val};
    }
    [[nodiscard]] inline
    vec2<T> operator*(vec2<T> val) {
        return vec2<T>{x * val.x, y * val.y};
    }

    // operator /
    [[nodiscard]] inline
    vec2<T> operator/(T val) {
        return vec2<T>{x / val, y / val};
    }
    [[nodiscard]] inline
    vec2<T> operator/(vec2<T> val) {
        return vec2<T>{x / val.x, y / val.y};
    }

    // operator ==
    [[nodiscard]] inline
    bool operator==(T val) {
        return x == val && y == val;
    }
    [[nodiscard]] inline
    bool operator==(vec2<T> val) {
        return x == val.x && y == val.y;
    }

    // operator +=
    inline void operator+=(T val) {
        x += val; 
        y += val;
    }
    inline void operator+=(vec2<T> val) {
        x += val.x;
        y += val.y;
    }

    // operator -=
    inline void operator-=(vec2<T> val) {
        x -= val.x;
        y -= val.y;
    }
    inline void operator-=(T val) {
        x -= val;
        y -= val;
    }

    // operator *=
    inline void operator*=(T val) {
        x *= val;
        y += val;
    }
    inline void operator*=(vec2<T> val) {
        x *= val.x;
        y *= val.y;
    }

    // operator /=
    inline void operator/=(T val) {
        x /= val;
        y /= val;
    }
    inline void operator/=(vec2<T> val) {
        x /= val.x; 
        y /= val.y;
    }

};

struct Circle {
    vec2<float> Center;
    float Radius;
};

struct Rectangle {
    float x, y;
    float width, height;
};

struct Texture2D {
    SDL_Texture* Texture = nullptr;
    int32_t width = 0;
    int32_t height = 0;
};

struct Color {
    uint8_t r, g, b, a;
};

union CollisionHit {
    Rectangle RectHitBox;
    Circle CircleHitBox;
};

struct Animation {
    std::string Name;
    std::vector<Texture2D> Frames;
    size_t CurrentFrame;
};

class ObjectAnimations {
    std::vector<Animation> Animations;
    size_t CurrentAnimation;
};

class GameObject;

struct EngineObjectCreationParams {
    bool Visible;
    vec2<float> Velocity;
    vec2<float> Position;
    GameObject& Object;
};

struct ObjectCreationParams {
    bool visible;
    vec2<float> velocity;
    vec2<float> position;
};

struct ConstantFrameData {
    uint32_t ObjectCount;
    glm::vec2 CameraPosition;
    glm::vec2 ScreenDimensions;
    float DeltaTime;
};

struct BulkObjectCreationData
{
    uint32_t ObjectCount = 0;
    std::vector<glm::vec2> Positions{};
    std::vector<glm::vec2> Velocities{};
    std::vector<uint32_t> FrameNumbers{};
    std::vector<uint32_t> AnimationNumbers{};
};

// TODO: make engine use
struct GPUPipelineCreationData {
    bool CalledByEngineEveryFrame = true;
    uint32_t MaxObjectsAtOnce{};
    uint32_t MaxObjectsCreatedAtOnce{};
    uint32_t MaxObjectsDeletedAtOnce{};
    BulkObjectCreationData StartingData{};
};

}