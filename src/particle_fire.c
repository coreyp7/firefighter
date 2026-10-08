#include "particle_fire.h"
#include "random_utils.h"
#include <math.h>
#include <string.h>

// Simple multi-frequency sine wave to approximate Perlin noise
static float smooth_noise(float seed, float time) {
    float value = 0.0f;
    value += sinf(seed + time) * 0.5f;
    value += sinf(seed * 2.0f + time * 2.0f) * 0.25f;
    value += sinf(seed * 4.0f + time * 4.0f) * 0.125f;
    return value;
}

static float clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static void spawn_particle_from_emitter(ParticleFireSystem *system, FireEmitter *emitter) {
    // Find inactive particle slot
    for (int i = 0; i < MAX_FIRE_PARTICLES; i++) {
        if (!system->particles[i].active) {
            FireParticle *p = &system->particles[i];

            // Position with horizontal spread
            p->x = emitter->x + (random_float() - 0.5f) * emitter->spawn_spread * 2.0f;
            p->y = emitter->y + (random_float() - 0.5f) * 5.0f;  // Vertical spread too

            // Initial velocity with variation
            p->velocity_x = (random_float() - 0.5f) * 0.5f;  // Small random horizontal velocity
            p->velocity_y = -1.5f - random_float() * 1.0f;  // Upward: -1.5 to -2.5

            // Lifetime
            //p->max_lifetime = 1.5f + random_float() * 2.5f;  // 1.5-4 seconds
            p->max_lifetime = random_float();
            p->lifetime = p->max_lifetime;

            // Noise seed for unique sway pattern (IMPORTANT: each particle different!)
            p->noise_seed = random_float() * 10000.0f;

            // Initial intensity
            p->intensity = 0.7f + random_float() * 0.3f;

            p->active = true;
            system->active_particle_count++;
            return;
        }
    }
}

static void get_fire_color(float age, uint8_t *r, uint8_t *g, uint8_t *b) {
    // age: 0.0 (new) to 1.0 (dying)

    if (age < 0.2f) {
        // Young/hot - white/yellow
        *r = 255;
        *g = 255;
        *b = 200;
    } else if (age < 0.5f) {
        // Middle - yellow
        *r = 255;
        *g = 220;
        *b = 100;
    } else if (age < 0.8f) {
        // Cooling - orange
        *r = 255;
        *g = 140;
        *b = 0;
    } else {
        // Dying - dark red
        *r = 200;
        *g = 50;
        *b = 0;
    }
}

void particle_fire_init(ParticleFireSystem *system) {
    memset(system, 0, sizeof(ParticleFireSystem));

    system->time = 0.0f;
    system->active_particle_count = 0;

    // Initialize all particles as inactive
    for (int i = 0; i < MAX_FIRE_PARTICLES; i++) {
        system->particles[i].active = false;
    }
}

void particle_fire_cleanup(ParticleFireSystem *system) {
    // Nothing to clean up for now
    (void)system;
}

void particle_fire_update(ParticleFireSystem *system, Fire *fires_buf, int max_fires, float dt) {
    system->time += dt;

    // Iterate through all fires
    for (int i = 0; i < max_fires; i++) {
        Fire *fire = &fires_buf[i];

        // Skip inactive or dead fires
        if (!fire->active || fire->health <= 0) {
            continue;
        }

        // Update emitter position (in case fire moved)
        // fire->emitter.x = fire->x + fire->w / 2.0f;
        // fire->emitter.y = fire->y + fire->h / 2.0f;

        // Calculate desired particles per frame based on health
        float health_ratio = fire->health / fire->max_health;
        //float particles_per_frame = 0.2f + (health_ratio * 7.8f);  // 0.2 - 8.0 range
        float particles_per_frame = health_ratio * 2.5;

        // Convert to spawn interval (seconds between spawns)
        // At 60fps, 1 frame = 1/60 second
        // particles_per_frame = particles / (1/60 seconds)
        // spawn_interval = (1/60) / particles_per_frame
        fire->emitter.spawn_interval = (1.0f / 60.0f) / particles_per_frame;

        // Accumulate time
        fire->emitter.time_since_spawn += dt;

        // Spawn particles while enough time has passed
        while (fire->emitter.time_since_spawn >= fire->emitter.spawn_interval) {
            spawn_particle_from_emitter(system, &fire->emitter);
            fire->emitter.time_since_spawn -= fire->emitter.spawn_interval;
        }

        // Clamp time_since_spawn to prevent unbounded growth
        if (fire->emitter.time_since_spawn > fire->emitter.spawn_interval * 10.0f) {
            fire->emitter.time_since_spawn = fire->emitter.spawn_interval * 10.0f;
        }
    }

    // Update existing particles
    for (int i = 0; i < MAX_FIRE_PARTICLES; i++) {
        FireParticle *p = &system->particles[i];

        if (!p->active) continue;

        // 1. Decay lifetime
        p->lifetime -= dt;
        if (p->lifetime <= 0.0f) {
            p->active = false;
            system->active_particle_count--;
            continue;
        }

        // 2. Move upward (buoyancy) - add acceleration over time
        p->velocity_y -= 0.5f * dt;  // Rise faster as it goes up
        p->y += p->velocity_y * dt * 60.0f;

        // 3. Horizontal sway using smooth noise - use noise_seed to make each particle unique
        float sway = smooth_noise(p->noise_seed, system->time * 3.0f) * 2.5f;
        p->x += sway * dt * 60.0f;

        // Add some turbulence (random horizontal impulses)
        if (random_float() < 0.05f) {  // 5% chance per frame
            p->velocity_x += (random_float() - 0.5f) * 2.0f;
        }

        // Apply horizontal velocity and dampen it
        p->x += p->velocity_x * dt * 60.0f;
        p->velocity_x *= 0.98f;  // Damping

        // 4. Flicker intensity
        float flicker = sinf(system->time * 10.0f) * 0.3f;
        p->intensity = 0.8f + flicker;

        // 5. Fade near death
        float age = 1.0f - (p->lifetime / p->max_lifetime);
        if (age > 0.8f) {
            // Fade out in final 20% of lifetime
            float fade_factor = (1.0f - age) / 0.2f;
            p->intensity *= fade_factor;
        }

        // Clamp intensity
        p->intensity = clampf(p->intensity, 0.0f, 1.0f);
    }
}

void particle_fire_render(ParticleFireSystem *system, SDL_Renderer *renderer, Camera camera) {
    // Render particles (no sorting for now - simple version)
    for (int i = 0; i < MAX_FIRE_PARTICLES; i++) {
        FireParticle *p = &system->particles[i];

        if (!p->active) continue;

        // Get color based on age
        float age = 1.0f - (p->lifetime / p->max_lifetime);
        uint8_t r, g, b;
        get_fire_color(age, &r, &g, &b);

        // Apply intensity as alpha
        uint8_t alpha = (uint8_t)(p->intensity * 255.0f);

        // Draw particle as a small circle (using filled rect for simplicity)
        SDL_SetRenderDrawColor(renderer, r, g, b, alpha);

        // Vary particle size based on age - larger when young, smaller when old
        //float size_factor = 1.0f - (age * 0.5f);  // 1.0 to 0.5
        float size_factor = 0.5f - (age * 0.25f);  // 1.0 to 0.5
        float radius = 3.0f + random_float() * 3.0f * size_factor;

        // Convert world position to camera position
        SDL_FPoint camera_pos = convert_pos_to_camera_pos(camera, p->x, p->y);

        SDL_FRect rect = {
            camera_pos.x - radius,
            camera_pos.y - radius,
            radius * 2.0f,
            radius * 2.0f
        };

        SDL_RenderFillRect(renderer, &rect);
    }
}


