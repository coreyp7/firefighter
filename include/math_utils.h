#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <stdbool.h>

// Simple 2D vector struct
typedef struct {
    float x;
    float y;
} Vec2;

// Calculate magnitude (length) of a 2D vector
float vec2_magnitude(float x, float y);

// Normalize a 2D vector (in-place modification via pointers)
// Returns false if the vector has zero length (no normalization performed)
bool vec2_normalize(float *x, float *y);

// Normalize a 2D vector and return as a Vec2 struct
// If the vector has zero length, returns (0, 0)
Vec2 vec2_normalized(float x, float y);

#endif
