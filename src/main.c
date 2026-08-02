#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdint.h>

typedef uint32_t uint32;

static float PLAYER_WALK_SPEED = 500.0;

bool initSDL(void);
void cleanupSDL(void);
bool loadImage(SDL_Renderer *renderer, SDL_Texture **texture, char* path);
void processInput(bool *isRunning);

typedef struct Player {
    float x;
    float y;
    float xvel;
    float yvel;
    SDL_FPoint mouse_pos;
} Player;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (!initSDL()) {
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("ff", 1080, 720, 0);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
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

    Player player = {50.0, 400.0, 0.0, 0.0, {0.0, 0.0}};
    float dt = 0.0;

    SDL_ShowWindow(window);

    SDL_Event event;

    uint32 last_state_update = 0;

    while(isRunning){
        uint32 start_ticks = SDL_GetTicks();

        // Input
        while(SDL_PollEvent(&event)){
            // TODO: check for input of left & right arrows.
            switch(event.type){
                case SDL_EVENT_QUIT:
                    isRunning = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if(event.key.key == SDLK_RIGHT){
                        player.xvel = PLAYER_WALK_SPEED;
                    } else if(event.key.key == SDLK_LEFT){
                        player.xvel = -PLAYER_WALK_SPEED;
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    if(event.key.key == SDLK_RIGHT){
                        player.xvel = 0;
                    } else if(event.key.key == SDLK_LEFT){
                        player.xvel = 0;
                    }
                    break;
            }
        }


        // Update state
        dt = (SDL_GetTicks() - last_state_update) / 1000.f;
        last_state_update = SDL_GetTicks();

        player.x += player.xvel * dt;
        player.y += player.yvel * dt;


        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_FRect player_rect = {player.x, player.y, 95, 95};
        SDL_RenderTextureRotated(renderer, player_texture, NULL, &player_rect, 0.0, NULL, SDL_FLIP_NONE);
        SDL_RenderPresent(renderer);

        uint32 time_of_frame = SDL_GetTicks() - start_ticks;
        uint32 required_length_of_frame = 1000.0 / 60.0; // 60 fps
        if(time_of_frame < required_length_of_frame){
            uint32 time_to_wait = required_length_of_frame - time_of_frame;
            SDL_Delay(time_to_wait);
        }
    }

    SDL_DestroyTexture(player_texture);
    SDL_DestroyWindow(window);
    cleanupSDL();

    return 0;
}

bool initSDL(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return false;
    }

    // sdl3 doesn't require init anymore? look into this.
    // int imgFlags = IMG_INIT_PNG;
    // if (!(IMG_Init(imgFlags) & imgFlags)) {
    //     SDL_Log("SDL_image initialization failed: %s", IMG_GetError());
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
        SDL_Log("Failed to load image: %s", SDL_GetError());
        return false;
    }

    *texture = SDL_CreateTextureFromSurface(renderer, img_surface);
    if (!(*texture)) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        SDL_DestroySurface(img_surface);
        return false;
    }

    SDL_DestroySurface(img_surface);
    return true;
}

