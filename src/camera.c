#include "camera.h"

SDL_FPoint convert_pos_to_camera_pos(Camera camera, float x, float y) {
    float newx = x - camera.x;
    float newy = y - camera.y;
    SDL_FPoint newpos = {newx, newy};
    return newpos;
}

void convert_camera_pos_to_global(Camera camera, float *x, float *y){
    // float newx = x + camera.x;
    // float newy = y + camera.y;
    // SDL_FPoint newpos = {newx, newy};
    // return newpos;
    *x = *x + camera.x;
    *y = *y + camera.y;
}
