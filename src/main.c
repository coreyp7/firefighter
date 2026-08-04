#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdint.h>
#include <math.h>

#include "water_particles.h"
#include "debug.h"

typedef uint32_t uint32;

/**
TODO: A general list of improvements we want to make for player movement.
- Add gravity to simulation of player
- basic collision detection to let player stand on something.
between the player and a simple box (below the player)
- allow player to jump
- player direction is dependent on where the cursor position is relative to player
(if to right of player, face right, if to left of player, face left). Update the
texture to be flipped appropriately.

As a part of the above changes:
- move game state stuff into its own module (all simulate functions)
    - water particles should be a part of this, move the array into this
    module instead of where it is right now
        - could put all state into a single struct so that we can pass it to the renderer or something

Future state:
- shooting water should have a tangible physics feedback for gamefeel and fun.
Allow player to gain speed by shooting water behind them.
- idea: allow player to walk on water shot? Maybe a different material could
allow the player to walk on it, powerup?
*/
typedef struct Player {
    float x;
    float y;
    float xvel;
    float yvel;
    float cursor_x;
    float cursor_y;
} Player;

bool initSDL(void);
void cleanupSDL(void);
bool loadImage(SDL_Renderer *renderer, SDL_Texture **texture, char* path);
void processInput(Player *player, bool *isRunning);
void update_player(Player *player, float dt);
void render_player(SDL_Renderer *renderer, SDL_Texture *player_texture, Player *player);
void render_water_stream(SDL_Renderer *renderer);

const float PLAYER_WALK_SPEED = 500.f;
const float GRAVITY = 500.f;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    initSDL();

    SDL_Window *window = SDL_CreateWindow("ff", 1080, 720, 0);
    if (!window) {
        cleanupSDL();
        return 1;
    }
    bool isRunning = true;

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture *player_texture = NULL;

    if (!loadImage(renderer, &player_texture, "img/player.webp")) {
        SDL_DestroyWindow(window);
        cleanupSDL();
        return 1;
    }

    SDL_Texture *bush_sprite_sheet = NULL;
    if (!loadImage(renderer, &bush_sprite_sheet, "img/bushes.png")) {
        SDL_DestroyWindow(window);
        cleanupSDL();
        return 2;
    }

    Player player = {50.0, 400.0, 0.0, 0.0};
    float dt = 0.0;
    uint32 last_state_update = SDL_GetTicks();

    SDL_ShowWindow(window);
    init_water_particles();
    init_debug(renderer);

    while(isRunning){
        uint32 start_ticks = SDL_GetTicks();

        // Input
        processInput(&player, &isRunning);

        // State
        dt = (SDL_GetTicks() - last_state_update) / 1000.f;
        last_state_update = SDL_GetTicks();

        update_player(&player, dt);
        simulate_water_particles(dt);

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render_player(renderer, player_texture, &player);

        SDL_FRect srcrect = {288, 32, 200, 180};
        SDL_FRect destrect = {250, 75, 150, 150};
        SDL_RenderTexture(
            renderer,
            bush_sprite_sheet,
            &srcrect,
            &destrect
        );
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderRect(renderer, &destrect);

        render_water_stream(renderer);

        // vsync
        uint32 time_of_frame = SDL_GetTicks() - start_ticks;
        int active_particles = get_active_water_particle_count();
        debug_render(renderer, (float)time_of_frame, active_particles, player.x, player.y);

        SDL_RenderPresent(renderer);
        uint32 required_length_of_frame = 1000.0 / 60.0; // 60 fps
        if(time_of_frame < required_length_of_frame){
            uint32 time_to_wait = required_length_of_frame - time_of_frame;
            SDL_Delay(time_to_wait);
        }
    }

    SDL_DestroyTexture(player_texture);
    SDL_DestroyWindow(window);
    cleanupSDL();
    cleanup_water_particles();
    cleanup_debug();

    return 0;
}

bool initSDL(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return false;
    }

    // sdl3 doesn't require init anymore? look into this.
    // int imgFlags = IMG_INIT_PNG;
    // if (!(IMG_Init(imgFlags) & imgFlags)) {
    //     SDL_Quit();
    //     return false;
    // }

    return true;
}

void cleanupSDL(void) {
    //IMG_Quit();
    SDL_Quit();
}

bool loadImage(SDL_Renderer *renderer, SDL_Texture **texture, char *path) {
    SDL_Surface *img_surface = IMG_Load(path);
    if (!img_surface) {
        return false;
    }

    *texture = SDL_CreateTextureFromSurface(renderer, img_surface);
    if (!(*texture)) {
        SDL_DestroySurface(img_surface);
        return false;
    }

    SDL_DestroySurface(img_surface);
    return true;
}

void processInput(Player *player, bool *isRunning) {
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_EVENT_QUIT:
                *isRunning = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if(event.key.key == SDLK_RIGHT) {
                    player->xvel = PLAYER_WALK_SPEED;
                } else if(event.key.key == SDLK_LEFT) {
                    player->xvel = -PLAYER_WALK_SPEED;
                }
                break;
            case SDL_EVENT_KEY_UP:
                if(event.key.key == SDLK_RIGHT || event.key.key == SDLK_LEFT) {
                    player->xvel = 0;
                }
                break;
        }
    }


    // Update player cursor with mouse
    float x, y;
    SDL_MouseButtonFlags state = SDL_GetMouseState(&x, &y);
    player->cursor_x = x;
    player->cursor_y = y;

    // spacebar/water shoot
    const bool *key_states = SDL_GetKeyboardState(NULL);
    if (key_states[SDL_SCANCODE_SPACE]) {
        float dx = player->cursor_x - player->x;
        float dy = player->cursor_y - player->y;
        float angle = atan2f(dx, dy);
        shoot_water_particle(player->x, player->y, angle);

    }
}

// TODO: this needs to be done in a gamestate module so that we can
// check collisions of player with anything.
// For now, just loop through all existing "platforms" and AABB collision check
// with them & the player. If the player is colliding, then resolve the collision in the y.
void update_player(Player *player, float dt) {
    player->yvel += 150 * dt;

    player->x += player->xvel * dt;
    player->y += player->yvel * dt;
}

void render_player(SDL_Renderer *renderer, SDL_Texture *player_texture, Player *player) {
    SDL_FRect player_rect = {player->x, player->y, 95, 95};
    SDL_RenderTextureRotated(renderer, player_texture, NULL, &player_rect, 0.0, NULL, SDL_FLIP_NONE);
}

void render_water_stream(SDL_Renderer *renderer) {
    render_water_particles(renderer);
}
