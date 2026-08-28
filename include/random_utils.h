#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

// Initialize random number generator (call once at startup)
void random_init(void);

// Generate random float in [0.0, 1.0]
float random_float(void);

// Generate random float in [min, max]
float random_float_range(float min, float max);

// Generate random int in [min, max] inclusive
int random_int_range(int min, int max);

#endif
