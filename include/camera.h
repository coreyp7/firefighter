#ifndef CAMERA_H
#define CAMERA_H

#include <SDL3/SDL.h>

// Camera is just an SDL_FRect representing the camera's view in world space.
// In future, if I need the camera to store more information than this, then
// we can just make it a struct proper.
typedef SDL_FRect Camera;

SDL_FPoint convert_pos_to_camera_pos(Camera camera, float x, float y);
void convert_camera_pos_to_global(Camera camera, float *x, float *y);

#endif
