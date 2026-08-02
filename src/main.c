#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

bool initSDL(void);
void cleanupSDL(void);
bool loadImage(SDL_Renderer *renderer, SDL_Texture **texture);
void processInput(bool *isRunning);

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (!initSDL()) {
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL3 Window", 640, 480, 0);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        cleanupSDL();
        return 1;
    }
    bool isRunning = true;

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture *gib_texture = NULL;

    if (!loadImage(renderer, &gib_texture)) {
        SDL_DestroyWindow(window);
        cleanupSDL();
        return 1;
    }

    SDL_ShowWindow(window);

    while(isRunning){
        processInput(&isRunning);

        SDL_FRect gib_rect;
        gib_rect.x = 0.0;
        gib_rect.y = 0.0;
        // gib_rect.w = gib_texture->w;
        // gib_rect.h = gib_texture->h;
        gib_rect.w = 200;
        gib_rect.h = 200;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTextureRotated(renderer, gib_texture, NULL, &gib_rect, 0.0, NULL, SDL_FLIP_NONE);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(gib_texture);
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

bool loadImage(SDL_Renderer *renderer, SDL_Texture **texture) {
    SDL_Surface *img_surface = IMG_Load("img/gibraltar.png");
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

void processInput(bool *isRunning) {
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_EVENT_QUIT:
                *isRunning = false;
                break;
        }
    }
}
