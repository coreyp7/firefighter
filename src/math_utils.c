#include "math_utils.h"
#include <math.h>
#include <stddef.h>

float vec2_magnitude(float x, float y) {
    return sqrtf(x * x + y * y);
}

bool vec2_normalize(float *x, float *y) {
    if (x == NULL || y == NULL) {
        return false;
    }

    float mag = vec2_magnitude(*x, *y);

    // Handle zero-length vector edge case
    if (mag < 0.0001f) {  // Use small epsilon for float comparison
        return false;
    }

    *x = *x / mag;
    *y = *y / mag;
    return true;
}

Vec2 vec2_normalized(float x, float y) {
    Vec2 result = {0.0f, 0.0f};

    float mag = vec2_magnitude(x, y);

    // Handle zero-length vector edge case
    if (mag < 0.0001f) {
        return result;  // Return (0, 0)
    }

    result.x = x / mag;
    result.y = y / mag;
    return result;
}
