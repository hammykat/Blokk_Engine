#pragma once

#include <SDL3/SDL.h>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <stdbool.h>
#include <stdlib.h>
#include <string>
#include <vector>

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

template <typename T>
struct vector2 {
    T x, y;

    // operator +
    vector2<T> operator+(T val) {
        return vector2<T>{x + val, y + val};
    }
    [[nodiscard]] inline
    vector2<T> operator+(vector2<T> val) {
        return vector2<T>{x + val.x, y + val.y};
    }
    
    // operator -
    [[nodiscard]] inline
    vector2<T> operator-(T val) {
        return vector2<T>{x - val, y - val};
    }
    [[nodiscard]] inline
    vector2<T> operator-(vector2<T> val) {
        return vector2<T>{x - val.x, y - val.y};
    }

    // operator *
    [[nodiscard]] inline
    vector2<T> operator*(T val) {
        return vector2<T>{x * val, y * val};
    }
    [[nodiscard]] inline
    vector2<T> operator*(vector2<T> val) {
        return vector2<T>{x * val.x, y * val.y};
    }

    // operator /
    [[nodiscard]] inline
    vector2<T> operator/(T val) {
        return vector2<T>{x / val, y / val};
    }
    [[nodiscard]] inline
    vector2<T> operator/(vector2<T> val) {
        return vector2<T>{x / val.x, y / val.y};
    }

    // operator ==
    [[nodiscard]] inline
    bool operator==(T val) {
        return x == val && y == val;
    }
    [[nodiscard]] inline
    bool operator==(vector2<T> val) {
        return x == val.x && y == val.y;
    }

    // operator +=
    inline void operator+=(T val) {
        x += val; 
        y += val;
    }
    inline void operator+=(vector2<T> val) {
        x += val.x;
        y += val.y;
    }

    // operator -=
    inline void operator-=(vector2<T> val) {
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
    inline void operator*=(vector2<T> val) {
        x *= val.x;
        y *= val.y;
    }

    // operator /=
    inline void operator/=(T val) {
        x /= val;
        y /= val;
    }
    inline void operator/=(vector2<T> val) {
        x /= val.x; 
        y /= val.y;
    }

};

struct Circle {
    vector2<float> Center;
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
    bool visible;
    vector2<float> velocity;
    vector2<float> position;
    GameObject& Object;
};

struct ObjectCreationParams {
    bool visible;
    vector2<float> velocity;
    vector2<float> position;
};

struct ConstantFrameData {
    int32_t objectCounts[3]; // Total, Static, Dynamic
    glm::vec2 cameraPosition;
    glm::vec2 screenDimensions;
    float deltaTime;
};

}