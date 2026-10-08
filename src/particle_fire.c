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

static void spawn_particle(ParticleFireSystem *system) {
    // Pick a random emitter
    if (system->emitter_count == 0) return;

    int emitter_index = random_int_range(0, system->emitter_count - 1);
    FireEmitter *emitter = &system->emitters[emitter_index];

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

void particle_fire_init(ParticleFireSystem *system, float emitter_x, float emitter_y) {
    memset(system, 0, sizeof(ParticleFireSystem));

    system->time = 0.0f;
    system->active_particle_count = 0;
    system->emitter_count = 0;

    // Initialize all particles as inactive
    for (int i = 0; i < MAX_FIRE_PARTICLES; i++) {
        system->particles[i].active = false;
    }

    // Add initial emitter if position is provided
    if (emitter_x != 0.0f || emitter_y != 0.0f) {
        particle_fire_add_emitter(system, emitter_x, emitter_y);
    }
}

void particle_fire_cleanup(ParticleFireSystem *system) {
    // Nothing to clean up for now
    (void)system;
}

void particle_fire_update(ParticleFireSystem *system, float dt) {
    system->time += dt;

    // This is dependent on fps right now but we have this locked to 60
    // TODO: could we use a ticks timer instead? I guess that'd limit it to
    // 1 particle a frame.
    for (int e = 0; e < system->emitter_count; e++) {
        FireEmitter *emitter = &system->emitters[e];
        for (int i = 0; i < emitter->particles_per_frame; i++) {
            // TODO: calculate how many particles should be spawned this frame
            // dependent on health.
            // How do we do this? We don't associate emitters with their fires.
            // Why don't we include the emitters in the fire struct so we can
            // just iterate through the fires?
            spawn_particle(system);
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

void particle_fire_set_emitter_position(ParticleFireSystem *system, float x, float y) {
    if (system->emitter_count > 0) {
        system->emitters[0].x = x;
        system->emitters[0].y = y;
    }
}

void particle_fire_set_spawn_rate(ParticleFireSystem *system, int particles_per_frame) {
    if (system->emitter_count > 0) {
        system->emitters[0].particles_per_frame = particles_per_frame;
    }
}

void particle_fire_add_emitter(ParticleFireSystem *system, float x, float y) {
    if (system->emitter_count >= MAX_FIRE_EMITTERS) return;

    FireEmitter *emitter = &system->emitters[system->emitter_count];
    emitter->x = x;
    emitter->y = y;
    emitter->particles_per_frame = 3;  // Fewer per emitter since we have many
    emitter->spawn_spread = 15.0f;

    system->emitter_count++;
}

void particle_fire_clear_emitters(ParticleFireSystem *system) {
    system->emitter_count = 0;
}
