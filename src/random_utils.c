#include "random_utils.h"
#include <stdlib.h>
#include <time.h>

void random_init(void) {
    srand((unsigned int)time(NULL));
}

float random_float(void) {
    return (float)rand() / (float)RAND_MAX;
}

float random_float_range(float min, float max) {
    return min + random_float() * (max - min);
}

int random_int_range(int min, int max) {
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    return min + (rand() % (max - min + 1));
}
