// src/main.cpp
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <random>
#include <string>
#include <vector>
#include <cstdlib>

namespace {
constexpr int LOGICAL_WIDTH  = 640;
constexpr int LOGICAL_HEIGHT = 384;
constexpr int DEFAULT_WINDOW_SCALE = 2;

constexpr float DEFAULT_LAUNCH_SPEED   = 160.0f;
constexpr float MIN_LAUNCH_SPEED       = 90.0f;
constexpr float MAX_LAUNCH_SPEED       = 260.0f;
constexpr float POWER_ADJUST_RATE      = 110.0f;

constexpr int DAMAGE_MORTAR            = 24;
constexpr int DAMAGE_CLUSTER           = 16;
constexpr int DAMAGE_CLUSTER_SHARD     = 12;
constexpr int DAMAGE_NAPALM_DIRECT     = 18;
constexpr int TANK_HP = 100;
constexpr float RELOAD_TIME = 0.45f;
constexpr float GRAVITY = 120.0f;
constexpr float TURRET_ROT_SPEED = 120.0f;
constexpr float MAX_TURRET_SWING = 90.0f;
constexpr float DEG2RAD = 0.0174532925f;
constexpr float PI = 3.14159265f;

constexpr float TERRAIN_BASELINE = LOGICAL_HEIGHT - 70.0f;

constexpr float TANK_COLLISION_WIDTH = 9.0f;
constexpr float TANK_COLLISION_HEIGHT = 5.0f;

constexpr float TANK_SCALE = 0.28f;

constexpr float HULL_TEXTURE_WIDTH = 72.0f;
constexpr float HULL_TEXTURE_HEIGHT = 28.0f;
constexpr float HULL_DRAW_WIDTH = HULL_TEXTURE_WIDTH * TANK_SCALE;
constexpr float HULL_DRAW_HEIGHT = HULL_TEXTURE_HEIGHT * TANK_SCALE;
constexpr float HULL_OFFSET_X = (HULL_DRAW_WIDTH - TANK_COLLISION_WIDTH) * 0.5f;
constexpr float HULL_OFFSET_Y = 10.0f * TANK_SCALE;

constexpr float TURRET_TEXTURE_WIDTH = 64.0f;
constexpr float TURRET_TEXTURE_HEIGHT = 24.0f;
constexpr float TURRET_DRAW_WIDTH = TURRET_TEXTURE_WIDTH * TANK_SCALE;
constexpr float TURRET_DRAW_HEIGHT = TURRET_TEXTURE_HEIGHT * TANK_SCALE;
constexpr float TURRET_PIVOT_X = 18.0f * TANK_SCALE;
constexpr float TURRET_PIVOT_Y = 16.0f * TANK_SCALE;

constexpr float TURRET_PIVOT_WORLD_OFFSET_Y = -1.4f;
constexpr float MUZZLE_LENGTH = 32.0f * TANK_SCALE;

constexpr float RADIUS_MORTAR = 3.2f;
constexpr float RADIUS_CLUSTER = 3.0f;
constexpr float RADIUS_CLUSTER_SHARD = 2.2f;
constexpr float RADIUS_NAPALM = 3.8f;

constexpr float CLUSTER_SPLIT_TIME = 0.45f;
constexpr float CLUSTER_SPREAD = 0.22f;

constexpr float NAPALM_BURN_DURATION = 1.2f;
constexpr float NAPALM_EROSION_RATE = 32.0f;
constexpr float EXPLOSION_DURATION = 0.45f;
constexpr float TANK_EXPLOSION_DURATION = 1.2f;

constexpr int GLYPH_WIDTH = 6;
constexpr int GLYPH_HEIGHT = 7;
constexpr int DEFAULT_GLYPH_PIXEL = 3;

enum class ProjectileKind { Mortar, Cluster, ClusterShard, Napalm };

enum class SceneryKind { Tower };

std::mt19937& rng() {
    static std::mt19937 engine{ std::random_device{}() };
    return engine;
}

float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng());
}

ProjectileKind nextAmmoType(ProjectileKind current) {
    switch (current) {
        case ProjectileKind::Mortar: return ProjectileKind::Cluster;
        case ProjectileKind::Cluster: return ProjectileKind::Napalm;
        case ProjectileKind::Napalm: return ProjectileKind::Mortar;
        case ProjectileKind::ClusterShard: return ProjectileKind::Mortar;
    }
    return ProjectileKind::Mortar;
}

const char* ammoDisplayName(ProjectileKind kind) {
    switch (kind) {
        case ProjectileKind::Mortar: return "Mortar";
        case ProjectileKind::Cluster: return "Cluster";
        case ProjectileKind::Napalm: return "Napalm";
        case ProjectileKind::ClusterShard: return "Cluster";
    }
    return "Mortar";
}

struct Assets {
    SDL_Texture* hull{};
    SDL_Texture* turret{};
};

void destroyAssets(Assets& assets) {
    if (assets.hull) {
        SDL_DestroyTexture(assets.hull);
        assets.hull = nullptr;
    }
    if (assets.turret) {
        SDL_DestroyTexture(assets.turret);
        assets.turret = nullptr;
    }
}

void fillSurfaceRect(SDL_Surface* surface, int x, int y, int w, int h, const SDL_Color& color) {
    SDL_Rect r{ x, y, w, h };
    SDL_FillRect(surface, &r, SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a));
}

SDL_Texture* createTankHullTexture(SDL_Renderer* renderer) {
    const int w = static_cast<int>(HULL_TEXTURE_WIDTH);
    const int h = static_cast<int>(HULL_TEXTURE_HEIGHT);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) return nullptr;

    SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

    const SDL_Color trackBase{ 56, 48, 42, 255 };
    const SDL_Color trackHighlight{ 124, 108, 90, 255 };
    const SDL_Color hullBase{ 148, 126, 98, 255 };
    const SDL_Color hullShadow{ 102, 84, 63, 255 };
    const SDL_Color hullHighlight{ 215, 198, 164, 255 };
    const SDL_Color scrawl{ 86, 64, 54, 255 };

    for (int i = 0; i < 5; ++i) {
        int offset = 4 + i * 12;
        fillSurfaceRect(surface, offset, 22 + ((i % 2) ? 1 : 0), 14, 4, trackBase);
        fillSurfaceRect(surface, offset, 20 + ((i + 1) % 2), 14, 2, trackHighlight);
    }

    fillSurfaceRect(surface, 6, 14, 60, 10, hullBase);
    fillSurfaceRect(surface, 6, 12, 52, 4, hullBase);
    fillSurfaceRect(surface, 8, 12, 56, 2, hullHighlight);
    fillSurfaceRect(surface, 10, 18, 44, 3, hullShadow);

    fillSurfaceRect(surface, 12, 9, 40, 5, hullBase);
    fillSurfaceRect(surface, 12, 8, 40, 2, hullHighlight);
    fillSurfaceRect(surface, 18, 6, 20, 3, hullBase);

    const SDL_Color panel{ 173, 153, 120, 255 };
    fillSurfaceRect(surface, 16, 13, 12, 4, panel);
    fillSurfaceRect(surface, 36, 13, 14, 5, panel);
    fillSurfaceRect(surface, 50, 14, 8, 3, hullShadow);

    for (int i = 0; i < 6; ++i) {
        int x = 10 + i * 8;
        int y = 16 + ((i % 2) ? 0 : 1);
        fillSurfaceRect(surface, x, y, 6, 2, scrawl);
    }

    const SDL_Color bolts{ 72, 56, 46, 255 };
    fillSurfaceRect(surface, 18, 11, 2, 2, bolts);
    fillSurfaceRect(surface, 30, 10, 2, 2, bolts);
    fillSurfaceRect(surface, 46, 11, 2, 2, bolts);

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (tex) SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(surface);
    return tex;
}

SDL_Texture* createTankTurretTexture(SDL_Renderer* renderer) {
    const int w = static_cast<int>(TURRET_TEXTURE_WIDTH);
    const int h = static_cast<int>(TURRET_TEXTURE_HEIGHT);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) return nullptr;

    SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

    const SDL_Color turretBase{ 150, 128, 100, 255 };
    const SDL_Color turretShade{ 112, 94, 74, 255 };
    const SDL_Color turretHighlight{ 218, 196, 160, 255 };
    const SDL_Color barrelShade{ 80, 64, 56, 255 };
    const SDL_Color scribble{ 94, 78, 64, 255 };

    fillSurfaceRect(surface, 6, 6, 32, 12, turretBase);
    fillSurfaceRect(surface, 8, 4, 22, 6, turretBase);
    fillSurfaceRect(surface, 8, 4, 22, 2, turretHighlight);
    fillSurfaceRect(surface, 6, 12, 30, 3, turretShade);

    fillSurfaceRect(surface, 28, 10, 30, 5, turretBase);
    fillSurfaceRect(surface, 28, 9, 30, 2, turretHighlight);
    fillSurfaceRect(surface, 54, 9, 6, 7, barrelShade);

    for (int i = 0; i < 5; ++i) {
        int x = 10 + i * 6;
        fillSurfaceRect(surface, x, 8, 4, 1, turretHighlight);
    }

    fillSurfaceRect(surface, 16, 6, 6, 4, scribble);
    fillSurfaceRect(surface, 22, 8, 6, 3, scribble);
    fillSurfaceRect(surface, 14, 12, 10, 2, scribble);

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (tex) SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(surface);
    return tex;
}

bool loadAssets(SDL_Renderer* renderer, Assets& assets) {
    assets.hull = createTankHullTexture(renderer);
    if (!assets.hull) return false;
    assets.turret = createTankTurretTexture(renderer);
    if (!assets.turret) {
        destroyAssets(assets);
        return false;
    }
    return true;
}

struct Projectile {
    SDL_FPoint position{};
    SDL_FPoint velocity{};
    float radius{RADIUS_MORTAR};
    ProjectileKind kind{};
    int damage{};
    int owner{};
    bool alive{true};
    float age{0.0f};
    bool spawnedChildren{false};
};

struct Explosion {
    SDL_FPoint position{};
    float timer{EXPLOSION_DURATION};
    float duration{EXPLOSION_DURATION};
    float maxRadius{22.0f};
    bool isTankExplosion{false};
};

struct NapalmPatch {
    SDL_FPoint position{};
    float radius{28.0f};
    float currentRadius{0.0f};
    float timer{NAPALM_BURN_DURATION};
};

struct SceneryObject {
    SDL_FRect rect{};
    SceneryKind kind{};
    float health{100.0f};
    float maxHealth{100.0f};
    bool alive{true};
};

struct Tank {
    SDL_FRect rect{};
    float turretAngleDeg{45.0f};
    float reloadTimer{0.0f};
    float launchSpeed{DEFAULT_LAUNCH_SPEED};
    float verticalVelocity{0.0f};
    ProjectileKind selected{ProjectileKind::Mortar};
    int hp{TANK_HP};
    SDL_Scancode aimUp{};
    SDL_Scancode aimDown{};
    SDL_Scancode powerUp{};
    SDL_Scancode powerDown{};
    SDL_Scancode fire{};
    SDL_Scancode nextAmmo{};
    int id{};
    bool facingRight{true};
    bool exploding{false};
    float explosionTimer{0.0f};
    bool ammoSwitchHeld{false};
};

struct GameState {
    Tank player1{};
    Tank player2{};
    std::vector<Projectile> projectiles{};
    std::vector<Explosion> explosions{};
    std::vector<NapalmPatch> napalmPatches{};
    std::vector<SceneryObject> scenery{};
    std::vector<int> terrainHeights{};
    std::vector<int> terrainSubstrate{};
    bool matchOver{false};
    int winner{0};
    float resetTimer{2.0f};
};

SDL_FRect makeTankRect(float x, float y) {
    return SDL_FRect{ x, y, TANK_COLLISION_WIDTH, TANK_COLLISION_HEIGHT };
}

float terrainHeightAt(const std::vector<int>& heights, float x) {
    if (heights.empty()) return static_cast<float>(LOGICAL_HEIGHT - 1);
    float clamped = std::clamp(x, 0.0f, static_cast<float>(LOGICAL_WIDTH - 1));
    int x0 = static_cast<int>(std::floor(clamped));
    int x1 = std::min(x0 + 1, LOGICAL_WIDTH - 1);
    float t = clamped - static_cast<float>(x0);
    return static_cast<float>(heights[x0]) + (static_cast<float>(heights[x1]) - static_cast<float>(heights[x0])) * t;
}

float substrateHeightAt(const std::vector<int>& substrate, float x) {
    if (substrate.empty()) return static_cast<float>(LOGICAL_HEIGHT - 1);
    float clamped = std::clamp(x, 0.0f, static_cast<float>(LOGICAL_WIDTH - 1));
    int x0 = static_cast<int>(std::floor(clamped));
    int x1 = std::min(x0 + 1, LOGICAL_WIDTH - 1);
    float t = clamped - static_cast<float>(x0);
    return static_cast<float>(substrate[x0]) + (static_cast<float>(substrate[x1]) - static_cast<float>(substrate[x0])) * t;
}

void generateTerrain(std::vector<int>& surface, std::vector<int>& substrate) {
    surface.resize(LOGICAL_WIDTH);
    substrate.resize(LOGICAL_WIDTH);

    const int segments = 10;
    std::array<float, segments + 1> controls{};
    const float baseLine = TERRAIN_BASELINE - randomFloat(4.0f, 10.0f);
    for (int i = 0; i <= segments; ++i) {
        controls[i] = baseLine + randomFloat(-8.0f, 8.0f);
    }

    for (int v = 0; v < 2; ++v) {
        int idx = std::clamp(static_cast<int>(randomFloat(1.0f, static_cast<float>(segments - 1))), 1, segments - 1);
        controls[idx] += randomFloat(28.0f, 40.0f);
    }
    for (int c = 0; c < 2; ++c) {
        int idx = std::clamp(static_cast<int>(randomFloat(1.0f, static_cast<float>(segments - 1))), 1, segments - 1);
        controls[idx] -= randomFloat(18.0f, 30.0f);
    }

    float segmentWidth = static_cast<float>(LOGICAL_WIDTH) / segments;
    for (int x = 0; x < LOGICAL_WIDTH; ++x) {
        float fx = static_cast<float>(x);
        int seg = std::min(static_cast<int>(fx / segmentWidth), segments - 1);
        float t = (fx - seg * segmentWidth) / segmentWidth;
        float start = controls[seg];
        float end = controls[seg + 1];
        float base = start + (end - start) * t;
        base += std::sin(fx * 0.07f + controls[seg] * 0.02f) * 3.0f;
        base += std::sin(fx * 0.18f + controls[seg + 1] * 0.015f) * 2.0f;
        surface[x] = static_cast<int>(std::round(base));
    }

    for (int pass = 0; pass < 2; ++pass) {
        std::vector<int> temp = surface;
        for (int x = 1; x < LOGICAL_WIDTH - 1; ++x) {
            temp[x] = static_cast<int>(std::round(surface[x] * 0.6f + surface[x - 1] * 0.2f + surface[x + 1] * 0.2f));
        }
        surface.swap(temp);
    }

    for (int& h : surface) {
        h = std::clamp(h, LOGICAL_HEIGHT - 118, LOGICAL_HEIGHT - 32);
    }

    for (int x = 0; x < LOGICAL_WIDTH; ++x) {
        float substrateBase = static_cast<float>(surface[x]) + randomFloat(14.0f, 22.0f);
        substrate[x] = static_cast<int>(std::round(std::min(substrateBase, static_cast<float>(LOGICAL_HEIGHT - 14))));
        substrate[x] = std::max(substrate[x], surface[x] + 10);
    }
}

SDL_Color palette(int index) {
    switch (index) {
        case 0: return SDL_Color{ 34, 17, 51, 255 };
        case 1: return SDL_Color{ 83, 135, 59, 255 };
        case 2: return SDL_Color{ 196, 217, 161, 255 };
        case 3: return SDL_Color{ 217, 87, 99, 255 };
        case 4: return SDL_Color{ 44, 54, 63, 255 };
        case 5: return SDL_Color{ 90, 67, 56, 255 };
        default: return SDL_Color{ 255, 255, 255, 255 };
    }
}

float turretWorldAngleDeg(const Tank& tank) {
    return tank.facingRight ? tank.turretAngleDeg : (180.0f - tank.turretAngleDeg);
}

Projectile spawnProjectile(const Tank& tank) {
    Projectile proj;
    proj.kind = tank.selected;
    proj.owner = tank.id;

    float speed = tank.launchSpeed;
    switch (proj.kind) {
        case ProjectileKind::Mortar:
            proj.damage = DAMAGE_MORTAR;
            proj.radius = RADIUS_MORTAR;
            break;
        case ProjectileKind::Cluster:
            proj.damage = DAMAGE_CLUSTER;
            proj.radius = RADIUS_CLUSTER;
            speed *= 0.95f;
            break;
        case ProjectileKind::Napalm:
            proj.damage = DAMAGE_NAPALM_DIRECT;
            proj.radius = RADIUS_NAPALM;
            speed *= 1.3f;
            break;
        case ProjectileKind::ClusterShard:
            proj.damage = DAMAGE_CLUSTER_SHARD;
            proj.radius = RADIUS_CLUSTER_SHARD;
            speed *= 0.9f;
            proj.spawnedChildren = true;
            break;
    }

    float angleDeg = turretWorldAngleDeg(tank);
    float angleRad = angleDeg * DEG2RAD;
    float pivotX = tank.rect.x + tank.rect.w * 0.5f;
    float pivotY = tank.rect.y + TURRET_PIVOT_WORLD_OFFSET_Y;

    proj.position.x = pivotX + std::cos(angleRad) * MUZZLE_LENGTH;
    proj.position.y = pivotY - std::sin(angleRad) * MUZZLE_LENGTH;
    proj.velocity = SDL_FPoint{
        std::cos(angleRad) * speed,
        -std::sin(angleRad) * speed
    };

    return proj;
}

void updateTank(Tank& tank, const Uint8* keys, float dt, std::vector<Projectile>& projectiles) {
    if (tank.reloadTimer > 0.0f) {
        tank.reloadTimer -= dt;
        if (tank.reloadTimer < 0.0f) tank.reloadTimer = 0.0f;
    }

    if (keys[tank.aimUp]) tank.turretAngleDeg += TURRET_ROT_SPEED * dt;
    if (keys[tank.aimDown]) tank.turretAngleDeg -= TURRET_ROT_SPEED * dt;

    tank.turretAngleDeg = std::clamp(tank.turretAngleDeg, 0.0f, MAX_TURRET_SWING);

    if (keys[tank.powerUp])   tank.launchSpeed += POWER_ADJUST_RATE * dt;
    if (keys[tank.powerDown]) tank.launchSpeed -= POWER_ADJUST_RATE * dt;
    tank.launchSpeed = std::clamp(tank.launchSpeed, MIN_LAUNCH_SPEED, MAX_LAUNCH_SPEED);

    if (keys[tank.nextAmmo]) {
        if (!tank.ammoSwitchHeld) {
            tank.selected = nextAmmoType(tank.selected);
            tank.ammoSwitchHeld = true;
        }
    } else {
        tank.ammoSwitchHeld = false;
    }

    if (keys[tank.fire] && tank.reloadTimer <= 0.0f) {
        projectiles.push_back(spawnProjectile(tank));
        tank.reloadTimer = RELOAD_TIME;
    }
}

bool circleIntersectsRect(const SDL_FPoint& center, float radius, const SDL_FRect& rect) {
    float closestX = std::clamp(center.x, rect.x, rect.x + rect.w);
    float closestY = std::clamp(center.y, rect.y, rect.y + rect.h);
    float dx = center.x - closestX;
    float dy = center.y - closestY;
    return (dx * dx + dy * dy) <= radius * radius;
}

SDL_FRect tankHitbox(const Tank& tank) {
    SDL_FRect hit = tank.rect;
    float extraWidth = HULL_DRAW_WIDTH * 0.45f;
    hit.x -= extraWidth * 0.5f;
    hit.w += extraWidth;
    float extraTop = TURRET_DRAW_HEIGHT * 0.85f;
    hit.y -= extraTop;
    hit.h += extraTop;
    return hit;
}

void deformTerrain(std::vector<int>& terrain, float centerX, float radius, float depth);

float sceneryMaxHealth(SceneryKind kind) {
    (void)kind;
    return 120.0f;
}

void erodeTerrainLayers(GameState& state, float centerX, float radius, float depth);

void destroySceneryObject(GameState& state, SceneryObject& object, const SDL_FPoint& impact) {
    if (!object.alive) return;
    object.alive = false;
    float radius = 26.0f;
    float depth = 14.0f;
    erodeTerrainLayers(state, object.rect.x + object.rect.w * 0.5f, radius, depth);
    state.explosions.push_back({ impact, 0.5f, 0.5f, radius + 6.0f, false });
}

void damageSceneryObject(GameState& state, SceneryObject& object, float amount, const SDL_FPoint& impact) {
    if (!object.alive) return;
    object.health -= amount;
    float scarDepth = std::max(2.0f, amount * 0.15f);
    erodeTerrainLayers(state, impact.x, std::max(object.rect.w * 0.25f, 10.0f), scarDepth);
    if (object.health <= 0.0f) {
        destroySceneryObject(state, object, impact);
    }
}

void erodeTerrainLayers(GameState& state, float centerX, float radius, float depth) {
    deformTerrain(state.terrainHeights, centerX, radius, depth);
    deformTerrain(state.terrainSubstrate, centerX, radius * 0.7f, depth * 0.35f);
    int start = std::max(0, static_cast<int>(std::floor(centerX - radius - 2.0f)));
    int end = std::min(LOGICAL_WIDTH - 1, static_cast<int>(std::ceil(centerX + radius + 2.0f)));
    for (int x = start; x <= end; ++x) {
        if (x >= 0 && x < static_cast<int>(state.terrainHeights.size()) && x < static_cast<int>(state.terrainSubstrate.size())) {
            state.terrainHeights[x] = std::min(state.terrainHeights[x], state.terrainSubstrate[x] - 2);
        }
    }
}

void carveCircularCrater(GameState& state, float centerX, float radius, float depth) {
    if (radius <= 0.0f || depth <= 0.0f) return;
    int start = std::max(0, static_cast<int>(std::floor(centerX - radius - 2.0f)));
    int end = std::min(LOGICAL_WIDTH - 1, static_cast<int>(std::ceil(centerX + radius + 2.0f)));
    float radiusSq = radius * radius;
    for (int x = start; x <= end; ++x) {
        float dx = static_cast<float>(x) - centerX;
        float distSq = dx * dx;
        if (distSq > radiusSq) continue;
        float normalized = distSq / radiusSq;
        float drop = depth * std::sqrt(std::max(0.0f, 1.0f - normalized));
        if (x < static_cast<int>(state.terrainHeights.size())) {
            state.terrainHeights[x] = std::min(LOGICAL_HEIGHT - 8, state.terrainHeights[x] + static_cast<int>(std::round(drop)));
        }
        if (x < static_cast<int>(state.terrainSubstrate.size())) {
            state.terrainSubstrate[x] = std::min(LOGICAL_HEIGHT - 6, state.terrainSubstrate[x] + static_cast<int>(std::round(drop * 0.35f)));
            state.terrainSubstrate[x] = std::max(state.terrainSubstrate[x], state.terrainHeights[x] + 8);
        }
    }
}

float clampPosition(float value, float halfWidth) {
    return std::clamp(value, halfWidth + 4.0f, static_cast<float>(LOGICAL_WIDTH) - halfWidth - 4.0f);
}

void addSceneryObject(GameState& state, SceneryKind kind, float centerX) {
    float width = randomFloat(20.0f, 28.0f);
    float height = randomFloat(78.0f, 108.0f);

    float halfWidth = width * 0.5f;
    float clampedCenter = clampPosition(centerX, halfWidth);
    float left = clampedCenter - halfWidth;
    float groundLeft = terrainHeightAt(state.terrainHeights, left);
    float groundRight = terrainHeightAt(state.terrainHeights, left + width);
    float support = std::min(groundLeft, groundRight);
    float top = support - height;

    SceneryObject object;
    object.rect = SDL_FRect{ left, top, width, height };
    object.kind = kind;
    object.maxHealth = sceneryMaxHealth(kind);
    object.health = object.maxHealth;
    object.alive = true;
    state.scenery.push_back(object);
}

void generateSceneryObjects(GameState& state) {
    state.scenery.clear();
    constexpr float MIN_DISTANCE_BETWEEN_TOWERS = 110.0f;
    constexpr float TANK_CLEAR_ZONE = 110.0f;
    std::vector<float> selected;
    std::array<float, 2> tankCenters{
        56.0f + TANK_COLLISION_WIDTH * 0.5f,
        static_cast<float>(LOGICAL_WIDTH) - 72.0f + TANK_COLLISION_WIDTH * 0.5f
    };

    auto isValid = [&](float candidate) {
        for (float center : tankCenters) {
            if (std::abs(candidate - center) < TANK_CLEAR_ZONE) return false;
        }
        for (float existing : selected) {
            if (std::abs(candidate - existing) < MIN_DISTANCE_BETWEEN_TOWERS) return false;
        }
        return true;
    };

    const int desiredTowers = 3;
    for (int i = 0; i < desiredTowers; ++i) {
        bool placed = false;
        for (int attempt = 0; attempt < 20 && !placed; ++attempt) {
            float candidate = randomFloat(80.0f, static_cast<float>(LOGICAL_WIDTH) - 80.0f);
            if (!isValid(candidate)) continue;
            selected.push_back(candidate);
            placed = true;
        }
    }

    for (float center : selected) {
        addSceneryObject(state, SceneryKind::Tower, center);
    }
}

void deformTerrain(std::vector<int>& terrain, float centerX, float radius, float depth) {
    if (terrain.empty()) return;
    int start = std::max(0, static_cast<int>(std::floor(centerX - radius - 2.0f)));
    int end = std::min(LOGICAL_WIDTH - 1, static_cast<int>(std::ceil(centerX + radius + 2.0f)));
    for (int x = start; x <= end; ++x) {
        float dx = static_cast<float>(x) - centerX;
        float dist = std::abs(dx);
        if (dist > radius) continue;
        float t = dist / radius;
        float falloff = (1.0f - t * t);
        float delta = depth * falloff;
        terrain[x] = std::min(LOGICAL_HEIGHT - 8, terrain[x] + static_cast<int>(std::round(delta)));
    }
    for (int& h : terrain) {
        h = std::clamp(h, LOGICAL_HEIGHT - 140, LOGICAL_HEIGHT - 20);
    }
}

void applyGravityToTank(Tank& tank, const std::vector<int>& terrain, float dt) {
    constexpr float GRAVITY_ACC = 260.0f;
    float leftSample = terrainHeightAt(terrain, tank.rect.x + tank.rect.w * 0.25f);
    float rightSample = terrainHeightAt(terrain, tank.rect.x + tank.rect.w * 0.75f);
    float support = std::min(leftSample, rightSample);
    float bottom = tank.rect.y + tank.rect.h;

    float gap = support - bottom;
    if (gap > 0.5f) {
        tank.verticalVelocity += GRAVITY_ACC * dt;
        tank.rect.y += tank.verticalVelocity * dt;
    } else if (gap < -0.5f) {
        tank.rect.y = support - tank.rect.h - 0.5f;
        tank.verticalVelocity = 0.0f;
    } else {
        tank.rect.y = support - tank.rect.h;
        tank.verticalVelocity = 0.0f;
    }

    float newBottom = tank.rect.y + tank.rect.h;
    if (newBottom >= support - 0.2f && tank.verticalVelocity > 0.0f && gap <= 0.5f) {
        tank.rect.y = support - tank.rect.h;
        tank.verticalVelocity = 0.0f;
    }

    if (tank.rect.y + tank.rect.h > LOGICAL_HEIGHT - 2) {
        tank.rect.y = LOGICAL_HEIGHT - 2 - tank.rect.h;
        tank.verticalVelocity = 0.0f;
    }
}

void updateProjectiles(GameState& state, float dt) {
    std::vector<Projectile> spawned;
    for (auto& proj : state.projectiles) {
        if (!proj.alive) continue;

        proj.age += dt;

        if (proj.kind == ProjectileKind::Cluster && !proj.spawnedChildren && proj.age >= CLUSTER_SPLIT_TIME) {
            float speedMag = std::sqrt(proj.velocity.x * proj.velocity.x + proj.velocity.y * proj.velocity.y);
            float baseAngle = std::atan2(proj.velocity.y, proj.velocity.x);
            for (int i = -1; i <= 1; ++i) {
                float spread = CLUSTER_SPREAD * static_cast<float>(i);
                float newAngle = baseAngle + spread;
                float newSpeed = speedMag * randomFloat(0.88f, 1.02f);
                Projectile shard;
                shard.kind = ProjectileKind::ClusterShard;
                shard.owner = proj.owner;
                shard.damage = DAMAGE_CLUSTER_SHARD;
                shard.radius = RADIUS_CLUSTER_SHARD;
                shard.position = proj.position;
                shard.velocity.x = std::cos(newAngle) * newSpeed;
                shard.velocity.y = std::sin(newAngle) * newSpeed;
                shard.spawnedChildren = true;
                spawned.push_back(shard);
            }
            state.explosions.push_back({proj.position, 0.25f, 0.25f, 14.0f, false});
            proj.alive = false;
            continue;
        }

        bool hitScenery = false;
        for (auto& object : state.scenery) {
            if (!object.alive) continue;
            if (circleIntersectsRect(proj.position, proj.radius, object.rect)) {
                float dmg = static_cast<float>(proj.damage);
                if (proj.kind == ProjectileKind::Napalm) {
                    dmg *= 0.7f;
                }
                damageSceneryObject(state, object, dmg, proj.position);
                state.explosions.push_back({proj.position, EXPLOSION_DURATION * 0.8f, EXPLOSION_DURATION * 0.8f, 20.0f, false});
                if (proj.kind == ProjectileKind::Napalm) {
                    float napalmRadius = 32.0f;
                    float napalmDepth = 11.0f;
                    carveCircularCrater(state, proj.position.x, napalmRadius, napalmDepth);
                    NapalmPatch patch;
                    patch.position = proj.position;
                    patch.radius = napalmRadius;
                    patch.currentRadius = 0.0f;
                    patch.timer = NAPALM_BURN_DURATION;
                    state.napalmPatches.push_back(patch);
                }
                proj.alive = false;
                hitScenery = true;
                break;
            }
        }
        if (hitScenery) {
            continue;
        }

        proj.velocity.y += GRAVITY * dt;
        proj.position.x += proj.velocity.x * dt;
        proj.position.y += proj.velocity.y * dt;

        if (proj.position.x + proj.radius < 0.0f || proj.position.x - proj.radius > LOGICAL_WIDTH ||
            proj.position.y - proj.radius > LOGICAL_HEIGHT) {
            proj.alive = false;
            continue;
        }

        float terrainY = terrainHeightAt(state.terrainHeights, proj.position.x);
        if (proj.position.y + proj.radius >= terrainY) {
            switch (proj.kind) {
                case ProjectileKind::Mortar:
                    carveCircularCrater(state, proj.position.x, 24.0f, 14.0f);
                    break;
                case ProjectileKind::Cluster:
                    erodeTerrainLayers(state, proj.position.x, 18.0f, 8.0f);
                    break;
                case ProjectileKind::ClusterShard:
                    erodeTerrainLayers(state, proj.position.x, 12.0f, 6.0f);
                    break;
                case ProjectileKind::Napalm: {
                    float napalmRadius = 34.0f;
                    float napalmDepth = 12.0f;
                    carveCircularCrater(state, proj.position.x, napalmRadius, napalmDepth);
                    NapalmPatch patch;
                    patch.position = proj.position;
                    patch.radius = napalmRadius;
                    patch.currentRadius = 0.0f;
                    patch.timer = NAPALM_BURN_DURATION;
                    state.napalmPatches.push_back(patch);
                    break;
                }
            }
            state.explosions.push_back({proj.position, EXPLOSION_DURATION, EXPLOSION_DURATION, 24.0f, proj.kind == ProjectileKind::Napalm});
            proj.alive = false;
            continue;
        }

        if (!state.matchOver) {
            Tank* targets[2] = { &state.player1, &state.player2 };
            for (Tank* target : targets) {
                if (proj.owner == target->id) continue;
                SDL_FRect hitbox = tankHitbox(*target);
                if (circleIntersectsRect(proj.position, proj.radius, hitbox)) {
                    target->hp -= proj.damage;
                    state.explosions.push_back({proj.position, EXPLOSION_DURATION, EXPLOSION_DURATION, 26.0f, false});
                    switch (proj.kind) {
                        case ProjectileKind::Mortar:
                            carveCircularCrater(state, proj.position.x, 22.0f, 12.0f);
                            break;
                        case ProjectileKind::Cluster:
                        case ProjectileKind::ClusterShard:
                            erodeTerrainLayers(state, proj.position.x, 16.0f, 8.0f);
                            break;
                        case ProjectileKind::Napalm: {
                            float napalmRadius = 32.0f;
                            float napalmDepth = 11.0f;
                            carveCircularCrater(state, proj.position.x, napalmRadius, napalmDepth);
                            NapalmPatch patch;
                            patch.position = proj.position;
                            patch.radius = napalmRadius;
                            patch.currentRadius = 0.0f;
                            patch.timer = NAPALM_BURN_DURATION;
                            state.napalmPatches.push_back(patch);
                            break;
                        }
                    }
                    proj.alive = false;
                    if (target->hp <= 0) {
                        target->exploding = true;
                        target->explosionTimer = TANK_EXPLOSION_DURATION;
                        state.explosions.push_back({
                            SDL_FPoint{ target->rect.x + target->rect.w * 0.5f, target->rect.y + target->rect.h * 0.5f },
                            TANK_EXPLOSION_DURATION,
                            TANK_EXPLOSION_DURATION,
                            48.0f,
                            true
                        });
                        erodeTerrainLayers(state, target->rect.x + target->rect.w * 0.5f, 36.0f, 18.0f);
                        state.matchOver = true;
                        state.winner = (target->id == 1) ? 2 : 1;
                        state.resetTimer = 3.0f;
                    }
                    break;
                }
            }
        }
    }

    state.projectiles.erase(
        std::remove_if(state.projectiles.begin(), state.projectiles.end(),
                       [](const Projectile& p) { return !p.alive; }),
        state.projectiles.end());

    if (!spawned.empty()) {
        state.projectiles.insert(state.projectiles.end(), spawned.begin(), spawned.end());
    }
}

void drawRect(SDL_Renderer* renderer, SDL_FRect rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderFillRectF(renderer, &rect);
}

void drawFilledCircle(SDL_Renderer* renderer, float cx, float cy, float radius, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int minY = static_cast<int>(std::floor(cy - radius));
    int maxY = static_cast<int>(std::ceil(cy + radius));
    for (int y = minY; y <= maxY; ++y) {
        float dy = cy - static_cast<float>(y);
        float spanSq = radius * radius - dy * dy;
        if (spanSq < 0.0f) continue;
        float dx = std::sqrt(spanSq);
        int x0 = static_cast<int>(std::floor(cx - dx));
        int x1 = static_cast<int>(std::ceil(cx + dx));
        SDL_RenderDrawLine(renderer, x0, y, x1, y);
    }
}

using GlyphRows = std::array<uint8_t, GLYPH_HEIGHT>;

constexpr GlyphRows GLYPH_SPACE{ 0,0,0,0,0,0,0 };
constexpr GlyphRows GLYPH_A{ 0b011110, 0b100001, 0b100001, 0b111111, 0b100001, 0b100001, 0b100001 };
constexpr GlyphRows GLYPH_C{ 0b011110, 0b100001, 0b100000, 0b100000, 0b100000, 0b100001, 0b011110 };
constexpr GlyphRows GLYPH_E{ 0b111111, 0b100000, 0b100000, 0b111110, 0b100000, 0b100000, 0b111111 };
constexpr GlyphRows GLYPH_G{ 0b011110, 0b100001, 0b100000, 0b101111, 0b100001, 0b100001, 0b011110 };
constexpr GlyphRows GLYPH_M{ 0b100001, 0b110011, 0b101101, 0b100001, 0b100001, 0b100001, 0b100001 };
constexpr GlyphRows GLYPH_O{ 0b011110, 0b100001, 0b100001, 0b100001, 0b100001, 0b100001, 0b011110 };
constexpr GlyphRows GLYPH_V{ 0b100001, 0b100001, 0b100001, 0b100001, 0b010010, 0b010010, 0b001100 };
constexpr GlyphRows GLYPH_R{ 0b111110, 0b100001, 0b100001, 0b111110, 0b101000, 0b100100, 0b100011 };
constexpr GlyphRows GLYPH_T{ 0b111111, 0b001100, 0b001100, 0b001100, 0b001100, 0b001100, 0b001100 };
constexpr GlyphRows GLYPH_P{ 0b111110, 0b100001, 0b100001, 0b111110, 0b100000, 0b100000, 0b100000 };
constexpr GlyphRows GLYPH_L{ 0b100000, 0b100000, 0b100000, 0b100000, 0b100000, 0b100000, 0b111111 };
constexpr GlyphRows GLYPH_U{ 0b100001, 0b100001, 0b100001, 0b100001, 0b100001, 0b100001, 0b011110 };
constexpr GlyphRows GLYPH_Y{ 0b100001, 0b010010, 0b010010, 0b001100, 0b001100, 0b001100, 0b001100 };
constexpr GlyphRows GLYPH_W{ 0b100001, 0b100001, 0b100001, 0b100101, 0b101101, 0b110011, 0b100001 };
constexpr GlyphRows GLYPH_I{ 0b111111, 0b001100, 0b001100, 0b001100, 0b001100, 0b001100, 0b111111 };
constexpr GlyphRows GLYPH_N{ 0b100001, 0b110001, 0b101001, 0b100101, 0b100011, 0b100001, 0b100001 };
constexpr GlyphRows GLYPH_S{ 0b011111, 0b100000, 0b100000, 0b011110, 0b000001, 0b000001, 0b111110 };
constexpr GlyphRows GLYPH_ONE{ 0b001100, 0b011100, 0b001100, 0b001100, 0b001100, 0b001100, 0b111111 };
constexpr GlyphRows GLYPH_TWO{ 0b011110, 0b100001, 0b000001, 0b000110, 0b001100, 0b011000, 0b111111 };

const GlyphRows* glyphFor(char c) {
    switch (c) {
        case 'A': return &GLYPH_A;
        case 'C': return &GLYPH_C;
        case 'E': return &GLYPH_E;
        case 'G': return &GLYPH_G;
        case 'M': return &GLYPH_M;
        case 'O': return &GLYPH_O;
        case 'V': return &GLYPH_V;
        case 'R': return &GLYPH_R;
        case 'T': return &GLYPH_T;
        case 'P': return &GLYPH_P;
        case 'L': return &GLYPH_L;
        case 'U': return &GLYPH_U;
        case 'Y': return &GLYPH_Y;
        case 'W': return &GLYPH_W;
        case 'I': return &GLYPH_I;
        case 'N': return &GLYPH_N;
        case 'S': return &GLYPH_S;
        case '1': return &GLYPH_ONE;
        case '2': return &GLYPH_TWO;
        case ' ': return &GLYPH_SPACE;
        default:  return nullptr;
    }
}

int drawGlyph(SDL_Renderer* renderer, int x, int y, SDL_Color color, const GlyphRows& glyph, int pixelSize) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect pixel{ x, y, pixelSize, pixelSize };
    for (int row = 0; row < GLYPH_HEIGHT; ++row) {
        pixel.y = y + row * pixelSize;
        uint8_t bits = glyph[row];
        for (int col = 0; col < GLYPH_WIDTH; ++col) {
            pixel.x = x + col * pixelSize;
            bool filled = (bits & (1 << (GLYPH_WIDTH - 1 - col))) != 0;
            if (filled) {
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
    return GLYPH_WIDTH * pixelSize;
}

int drawText(SDL_Renderer* renderer, int x, int y, const std::string& text, SDL_Color color, int pixelSize = DEFAULT_GLYPH_PIXEL) {
    int cursor = x;
    int glyphSpacing = pixelSize + 1;
    int wordSpacing = pixelSize * 2;
    for (char c : text) {
        if (c == ' ') {
            cursor += wordSpacing;
            continue;
        }
        const GlyphRows* glyph = glyphFor(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        if (!glyph) {
            cursor += wordSpacing;
            continue;
        }
        cursor += drawGlyph(renderer, cursor, y, color, *glyph, pixelSize);
        cursor += glyphSpacing;
    }
    return cursor - x;
}

int measureText(const std::string& text, int pixelSize = DEFAULT_GLYPH_PIXEL) {
    int width = 0;
    int glyphSpacing = pixelSize + 1;
    int wordSpacing = pixelSize * 2;
    for (char c : text) {
        if (c == ' ') {
            width += wordSpacing;
            continue;
        }
        const GlyphRows* glyph = glyphFor(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        if (glyph) {
            width += GLYPH_WIDTH * pixelSize + glyphSpacing;
        } else {
            width += wordSpacing;
        }
    }
    if (width > 0) width -= glyphSpacing;
    return width;
}

void drawBackground(SDL_Renderer* renderer) {
    SDL_Color duskSkyTop{ 28, 21, 56, 255 };
    SDL_Color duskSkyMid{ 120, 65, 110, 255 };
    SDL_Color duskSkyBottom{ 230, 154, 104, 255 };

    for (int y = 0; y < LOGICAL_HEIGHT; ++y) {
        float t = y / static_cast<float>(LOGICAL_HEIGHT - 1);
        float band = std::sin(t * 12.0f) * 0.06f;
        float warpedT = std::clamp(t + band, 0.0f, 1.0f);
        SDL_Color c;
        if (warpedT < 0.5f) {
            float u = warpedT / 0.5f;
            c.r = static_cast<Uint8>(duskSkyTop.r + (duskSkyMid.r - duskSkyTop.r) * u);
            c.g = static_cast<Uint8>(duskSkyTop.g + (duskSkyMid.g - duskSkyTop.g) * u);
            c.b = static_cast<Uint8>(duskSkyTop.b + (duskSkyMid.b - duskSkyTop.b) * u);
        } else {
            float u = (warpedT - 0.5f) / 0.5f;
            c.r = static_cast<Uint8>(duskSkyMid.r + (duskSkyBottom.r - duskSkyMid.r) * u);
            c.g = static_cast<Uint8>(duskSkyMid.g + (duskSkyBottom.g - duskSkyMid.g) * u);
            c.b = static_cast<Uint8>(duskSkyMid.b + (duskSkyBottom.b - duskSkyMid.b) * u);
        }

        int wobble = static_cast<int>(std::sin((y * 2.1f) + 0.4f) * 4.0f);
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        SDL_RenderDrawLine(renderer, -8 + wobble, y, LOGICAL_WIDTH + 8 + wobble, y);

        if (y % 7 == 0) {
            SDL_SetRenderDrawColor(renderer, 255, 200, 150, 35);
            SDL_RenderDrawLine(renderer, 0, y, LOGICAL_WIDTH, y + wobble / 2);
        }
    }
}

void drawTerrain(SDL_Renderer* renderer, const std::vector<int>& surface, const std::vector<int>& substrate) {
    SDL_Color bedrock{ 72, 76, 88, 255 };
    SDL_Color base{ 104, 108, 120, 255 };
    SDL_Color highlight{ 224, 226, 232, 255 };
    SDL_Color midTone{ 150, 154, 164, 255 };
    SDL_Color rimLight{ 242, 244, 248, 255 };
    SDL_Color striation{ 94, 98, 112, 180 };

    for (int x = 0; x < LOGICAL_WIDTH; ++x) {
        int top = surface[x];
        int sub = substrate.empty() ? std::min(LOGICAL_HEIGHT - 12, top + 14) : std::max(surface[x] + 6, substrate[x]);
        SDL_SetRenderDrawColor(renderer, bedrock.r, bedrock.g, bedrock.b, 255);
        SDL_RenderDrawLine(renderer, x, sub, x, LOGICAL_HEIGHT);

        SDL_SetRenderDrawColor(renderer, base.r, base.g, base.b, 255);
        SDL_RenderDrawLine(renderer, x, top, x, sub);
    }

    SDL_SetRenderDrawColor(renderer, striation.r, striation.g, striation.b, striation.a);
    for (int x = 0; x < LOGICAL_WIDTH; x += 6) {
        int top = surface[x];
        SDL_RenderDrawLine(renderer, x - 2, top + 3, x + 4, top + 8);
    }

    SDL_SetRenderDrawColor(renderer, midTone.r, midTone.g, midTone.b, 150);
    for (int x = 0; x < LOGICAL_WIDTH; x += 5) {
        int top = surface[x];
        SDL_RenderDrawLine(renderer, x, top + 2, x + 1, top + 6);
    }

    SDL_SetRenderDrawColor(renderer, highlight.r, highlight.g, highlight.b, 210);
    for (int x = 0; x < LOGICAL_WIDTH; ++x) {
        int top = surface[x];
        SDL_RenderDrawPoint(renderer, x, top);
        if (x % 7 == 0) {
            SDL_RenderDrawPoint(renderer, x, top - 1);
        }
    }

    SDL_SetRenderDrawColor(renderer, rimLight.r, rimLight.g, rimLight.b, 160);
    for (int x = 1; x < LOGICAL_WIDTH - 1; ++x) {
        int current = surface[x];
        int prev = surface[x - 1];
        int next = surface[x + 1];
        if (current <= prev && current <= next) {
            SDL_RenderDrawPoint(renderer, x, current - 1);
        }
    }
}

void drawScenery(SDL_Renderer* renderer, const std::vector<SceneryObject>& objects) {
    for (const auto& obj : objects) {
        if (!obj.alive) continue;
        float healthRatio = obj.maxHealth > 0.0f ? std::clamp(obj.health / obj.maxHealth, 0.0f, 1.0f) : 1.0f;
        SDL_FRect rect = obj.rect;
        SDL_Color base = SDL_Color{ static_cast<Uint8>(124 + 32 * healthRatio), static_cast<Uint8>(128 + 26 * healthRatio), static_cast<Uint8>(134 + 24 * healthRatio), 255 };
        SDL_SetRenderDrawColor(renderer, base.r, base.g, base.b, 255);
        SDL_RenderFillRectF(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, 92, 92, 104, 255);
        SDL_RenderDrawLine(renderer, static_cast<int>(rect.x + rect.w * 0.2f), static_cast<int>(rect.y + rect.h * 0.2f), static_cast<int>(rect.x + rect.w * 0.8f), static_cast<int>(rect.y + rect.h * 0.95f));
        SDL_RenderDrawLine(renderer, static_cast<int>(rect.x + rect.w * 0.8f), static_cast<int>(rect.y + rect.h * 0.2f), static_cast<int>(rect.x + rect.w * 0.2f), static_cast<int>(rect.y + rect.h * 0.95f));

        SDL_FRect platform{ rect.x - rect.w * 0.1f, rect.y - rect.h * 0.08f, rect.w * 1.2f, rect.h * 0.18f };
        SDL_SetRenderDrawColor(renderer, 84, 68, 60, 255);
        SDL_RenderFillRectF(renderer, &platform);

        SDL_FRect hut{ rect.x + rect.w * 0.08f, rect.y - rect.h * 0.24f, rect.w * 0.84f, rect.h * 0.25f };
        SDL_SetRenderDrawColor(renderer, 98, 88, 80, 255);
        SDL_RenderFillRectF(renderer, &hut);
        SDL_SetRenderDrawColor(renderer, 150, 152, 160, 255);
        SDL_Rect window{ static_cast<int>(hut.x + hut.w * 0.2f), static_cast<int>(hut.y + hut.h * 0.35f), static_cast<int>(hut.w * 0.2f), static_cast<int>(hut.h * 0.4f) };
        SDL_RenderFillRect(renderer, &window);
        window.x += static_cast<int>(hut.w * 0.4f);
        SDL_RenderFillRect(renderer, &window);
    }
}

void drawTank(SDL_Renderer* renderer, const Tank& tank, const Assets& assets, bool isPlayerOne) {
    if (tank.exploding) {
        float fade = std::clamp(tank.explosionTimer / TANK_EXPLOSION_DURATION, 0.0f, 1.0f);
        SDL_Color smoke{ 60, 60, 70, static_cast<Uint8>(fade * 160.0f) };
        drawFilledCircle(renderer,
                         tank.rect.x + tank.rect.w * 0.5f,
                         tank.rect.y + tank.rect.h * 0.5f,
                         12.0f + (1.0f - fade) * 20.0f,
                         smoke);
        return;
    }
    float wobble = std::sin(SDL_GetTicks() * 0.0035f + (isPlayerOne ? 0.35f : 2.2f)) * 1.2f;

    SDL_SetTextureColorMod(assets.hull,
        isPlayerOne ? 172 : 140,
        isPlayerOne ? 172 : 140,
        isPlayerOne ? 176 : 150);
    SDL_SetTextureColorMod(assets.turret,
        isPlayerOne ? 200 : 168,
        isPlayerOne ? 200 : 168,
        isPlayerOne ? 205 : 176);

    SDL_FRect hullDest{
        tank.rect.x - HULL_OFFSET_X + wobble * 0.3f,
        tank.rect.y - HULL_OFFSET_Y + wobble * 0.2f,
        HULL_DRAW_WIDTH,
        HULL_DRAW_HEIGHT
    };
    SDL_Rect hullDst{
        static_cast<int>(std::lround(hullDest.x)),
        static_cast<int>(std::lround(hullDest.y)),
        static_cast<int>(std::lround(hullDest.w)),
        static_cast<int>(std::lround(hullDest.h))
    };
    SDL_RenderCopy(renderer, assets.hull, nullptr, &hullDst);

    float pivotWorldX = tank.rect.x + tank.rect.w * 0.5f;
    float pivotWorldY = tank.rect.y + TURRET_PIVOT_WORLD_OFFSET_Y;

    SDL_Rect turretDst{
        static_cast<int>(std::lround(pivotWorldX - TURRET_PIVOT_X + wobble * 0.4f)),
        static_cast<int>(std::lround(pivotWorldY - TURRET_PIVOT_Y + wobble * 0.3f)),
        static_cast<int>(std::lround(TURRET_DRAW_WIDTH)),
        static_cast<int>(std::lround(TURRET_DRAW_HEIGHT))
    };
    SDL_Point pivot{
        static_cast<int>(std::lround(TURRET_PIVOT_X)),
        static_cast<int>(std::lround(TURRET_PIVOT_Y))
    };

    double renderAngle = -static_cast<double>(turretWorldAngleDeg(tank));
    SDL_RenderCopyEx(renderer, assets.turret, nullptr, &turretDst, renderAngle, &pivot, SDL_FLIP_NONE);
}

void drawProjectiles(SDL_Renderer* renderer, const std::vector<Projectile>& projectiles) {
    for (const auto& proj : projectiles) {
        SDL_Color glow{};
        SDL_Color core{};
        float glowExtra = 1.6f;
        switch (proj.kind) {
            case ProjectileKind::Mortar:
                glow = SDL_Color{ 248, 236, 210, 160 };
                core = SDL_Color{ 255, 252, 240, 255 };
                break;
            case ProjectileKind::Cluster:
                glow = SDL_Color{ 255, 118, 118, 170 };
                core = SDL_Color{ 255, 178, 178, 255 };
                break;
            case ProjectileKind::ClusterShard:
                glow = SDL_Color{ 255, 90, 90, 170 };
                core = SDL_Color{ 255, 158, 158, 255 };
                glowExtra = 1.2f;
                break;
            case ProjectileKind::Napalm:
                glow = SDL_Color{ 255, 152, 64, 210 };
                core = SDL_Color{ 255, 228, 136, 255 };
                glowExtra = 2.4f;
                break;
        }
        drawFilledCircle(renderer, proj.position.x, proj.position.y, proj.radius + glowExtra, glow);
        drawFilledCircle(renderer, proj.position.x, proj.position.y, proj.radius, core);
        if (proj.kind == ProjectileKind::Napalm) {
            SDL_Color ember{ 255, 108, 32, 160 };
            drawFilledCircle(renderer, proj.position.x, proj.position.y + proj.radius * 0.35f, proj.radius * 0.65f, ember);
        }
    }
}

void drawExplosions(SDL_Renderer* renderer, const std::vector<Explosion>& explosions) {
    for (const auto& explosion : explosions) {
        float lifeT = std::clamp(explosion.timer / explosion.duration, 0.0f, 1.0f);
        float pct = 1.0f - lifeT;
        float radius = (explosion.isTankExplosion ? 12.0f : 6.0f) + pct * explosion.maxRadius;
        Uint8 alpha = static_cast<Uint8>(lifeT * (explosion.isTankExplosion ? 255.0f : 200.0f));
        SDL_Color outer = explosion.isTankExplosion
            ? SDL_Color{ 255, 120, 80, static_cast<Uint8>(alpha * 0.6f) }
            : SDL_Color{ 255, 150, 70, static_cast<Uint8>(alpha * 0.7f) };
        SDL_Color inner = explosion.isTankExplosion
            ? SDL_Color{ 255, 240, 200, alpha }
            : SDL_Color{ 255, 235, 180, alpha };
        drawFilledCircle(renderer, explosion.position.x, explosion.position.y, radius, outer);
        drawFilledCircle(renderer, explosion.position.x, explosion.position.y, radius * (explosion.isTankExplosion ? 0.7f : 0.6f), inner);
    }
}

void updateExplosions(std::vector<Explosion>& explosions, float dt) {
    for (auto& explosion : explosions) {
        explosion.timer -= dt;
    }
    explosions.erase(
        std::remove_if(explosions.begin(), explosions.end(),
                       [](const Explosion& e) { return e.timer <= 0.0f; }),
        explosions.end());
}

void drawNapalmPatches(SDL_Renderer* renderer, const std::vector<NapalmPatch>& patches) {
    for (const auto& patch : patches) {
        float lifeT = std::clamp(patch.timer / NAPALM_BURN_DURATION, 0.0f, 1.0f);
        float radius = std::max(patch.currentRadius, patch.radius * 0.25f);
        SDL_Color outer{ 255, 120, 48, static_cast<Uint8>(lifeT * 120.0f) };
        SDL_Color inner{ 255, 190, 96, static_cast<Uint8>(lifeT * 200.0f) };
        drawFilledCircle(renderer, patch.position.x, patch.position.y, radius, outer);
        drawFilledCircle(renderer, patch.position.x, patch.position.y, radius * 0.6f, inner);
    }
}

void updateNapalmPatches(GameState& state, float dt) {
    for (auto& patch : state.napalmPatches) {
        if (patch.timer <= 0.0f) continue;
        float growth = (patch.radius / std::max(0.2f, NAPALM_BURN_DURATION)) * dt * 1.4f;
        patch.currentRadius = std::min(patch.radius, patch.currentRadius + growth);
        patch.timer -= dt;
    }

    state.napalmPatches.erase(
        std::remove_if(state.napalmPatches.begin(), state.napalmPatches.end(),
                       [](const NapalmPatch& p) { return p.timer <= 0.0f; }),
        state.napalmPatches.end());
}

void drawUI(SDL_Renderer* renderer, const GameState& state) {
    SDL_SetRenderDrawColor(renderer, palette(4).r, palette(4).g, palette(4).b, 255);
    SDL_RenderDrawLine(renderer, 12, 24, LOGICAL_WIDTH - 12, 24);

    SDL_FRect p1Hp{ 20.0f, 28.0f, (state.player1.hp / static_cast<float>(TANK_HP)) * 96.0f, 6.0f };
    SDL_FRect p2Hp{ LOGICAL_WIDTH - 116.0f, 28.0f, (state.player2.hp / static_cast<float>(TANK_HP)) * 96.0f, 6.0f };
    drawRect(renderer, p1Hp, palette(1));
    drawRect(renderer, p2Hp, palette(3));

    SDL_Color labelColor{ 230, 218, 190, 255 };
    const std::string p1Label = "PLAYER 1";
    const std::string p2Label = "PLAYER 2";
    constexpr int NAMEPLATE_PIXEL = 1;
    constexpr int AMMO_PIXEL = NAMEPLATE_PIXEL;
    int labelHeight = GLYPH_HEIGHT * NAMEPLATE_PIXEL;
    int p1LabelW = measureText(p1Label, NAMEPLATE_PIXEL);
    int p2LabelW = measureText(p2Label, NAMEPLATE_PIXEL);
    int p1LabelX = static_cast<int>(std::lround(p1Hp.x + (p1Hp.w - p1LabelW) * 0.5f));
    int p2LabelX = static_cast<int>(std::lround(p2Hp.x + (p2Hp.w - p2LabelW) * 0.5f));
    int p1LabelY = std::max(0, static_cast<int>(std::lround(p1Hp.y)) - labelHeight - 2);
    int p2LabelY = std::max(0, static_cast<int>(std::lround(p2Hp.y)) - labelHeight - 2);
    drawText(renderer, p1LabelX, p1LabelY, p1Label, labelColor, NAMEPLATE_PIXEL);
    drawText(renderer, p2LabelX, p2LabelY, p2Label, labelColor, NAMEPLATE_PIXEL);

    const float barWidth = 96.0f;
    const float barHeight = 4.0f;
    const float minSpeed = MIN_LAUNCH_SPEED;
    const float denom = MAX_LAUNCH_SPEED - MIN_LAUNCH_SPEED;

    auto drawPowerBar = [&](const Tank& tank, float x, float y) {
        float pct = (tank.launchSpeed - minSpeed) / denom;
        pct = std::clamp(pct, 0.0f, 1.0f);
        SDL_FRect bg{ x, y, barWidth, barHeight };
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 180);
        SDL_RenderFillRectF(renderer, &bg);
        SDL_FRect fill{ x, y, barWidth * pct, barHeight };
        SDL_SetRenderDrawColor(renderer, 255, 214, 120, 255);
        SDL_RenderFillRectF(renderer, &fill);
        SDL_Rect outline{ static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), static_cast<int>(std::lround(barWidth)), static_cast<int>(std::lround(barHeight)) };
        SDL_SetRenderDrawColor(renderer, 80, 60, 40, 255);
        SDL_RenderDrawRect(renderer, &outline);
    };

    float powerBarY = 28.0f + 10.0f;
    drawPowerBar(state.player1, 20.0f, powerBarY);
    drawPowerBar(state.player2, LOGICAL_WIDTH - 116.0f, powerBarY);

    std::string p1Ammo = ammoDisplayName(state.player1.selected);
    std::string p2Ammo = ammoDisplayName(state.player2.selected);
    int p1AmmoW = measureText(p1Ammo, AMMO_PIXEL);
    int p2AmmoW = measureText(p2Ammo, AMMO_PIXEL);
    int ammoY = static_cast<int>(std::lround(powerBarY + barHeight + 6.0f));
    int p1AmmoX = static_cast<int>(std::lround(20.0f + (barWidth - p1AmmoW) * 0.5f));
    int p2AmmoX = static_cast<int>(std::lround(LOGICAL_WIDTH - 116.0f + (barWidth - p2AmmoW) * 0.5f));
    drawText(renderer, p1AmmoX, ammoY, p1Ammo, labelColor, AMMO_PIXEL);
    drawText(renderer, p2AmmoX, ammoY, p2Ammo, labelColor, AMMO_PIXEL);
}

void drawBanner(SDL_Renderer* renderer, int winner) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect banner{ 64, LOGICAL_HEIGHT / 2 - 36, LOGICAL_WIDTH - 128, 72 };
    SDL_RenderFillRect(renderer, &banner);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &banner);

    SDL_Color textColor{ 255, 236, 180, 255 };
    const std::string title = "GAME OVER";
    const std::string subtitle = (winner == 1) ? "PLAYER 1 WINS" : "PLAYER 2 WINS";

    int titleWidth = measureText(title);
    int subtitleWidth = measureText(subtitle);
    int titleX = banner.x + (banner.w - titleWidth) / 2;
    int subtitleX = banner.x + (banner.w - subtitleWidth) / 2;
    int titleY = banner.y + 16;
    int subtitleY = banner.y + 40;

    drawText(renderer, titleX, titleY, title, textColor);
    drawText(renderer, subtitleX, subtitleY, subtitle, textColor);
}

void positionTankOnTerrain(Tank& tank, const std::vector<int>& terrain) {
    float centerX = tank.rect.x + tank.rect.w * 0.5f;
    float surfaceY = terrainHeightAt(terrain, centerX);
    tank.rect.y = surfaceY - tank.rect.h;
    tank.verticalVelocity = 0.0f;
}

void resetMatch(GameState& state) {
    generateTerrain(state.terrainHeights, state.terrainSubstrate);
    generateSceneryObjects(state);
    state.projectiles.clear();
    state.explosions.clear();
    state.napalmPatches.clear();
    state.matchOver = false;
    state.winner = 0;
    state.resetTimer = 2.0f;

    state.player1.rect = makeTankRect(56.0f, 0.0f);
    state.player2.rect = makeTankRect(LOGICAL_WIDTH - 72.0f, 0.0f);

    positionTankOnTerrain(state.player1, state.terrainHeights);
    positionTankOnTerrain(state.player2, state.terrainHeights);
    state.player1.verticalVelocity = 0.0f;
    state.player2.verticalVelocity = 0.0f;

    state.player1.turretAngleDeg = 45.0f;
    state.player2.turretAngleDeg = 45.0f;

    state.player1.reloadTimer = 0.0f;
    state.player2.reloadTimer = 0.0f;

    state.player1.launchSpeed = DEFAULT_LAUNCH_SPEED;
    state.player2.launchSpeed = DEFAULT_LAUNCH_SPEED;

    state.player1.selected = ProjectileKind::Mortar;
    state.player2.selected = ProjectileKind::Mortar;
    state.player1.ammoSwitchHeld = false;
    state.player2.ammoSwitchHeld = false;

    state.player1.hp = TANK_HP;
    state.player2.hp = TANK_HP;

    state.player1.exploding = false;
    state.player2.exploding = false;
    state.player1.explosionTimer = 0.0f;
    state.player2.explosionTimer = 0.0f;
}

} // namespace

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    int windowScale = DEFAULT_WINDOW_SCALE;
    int windowWidth = LOGICAL_WIDTH * windowScale;
    int windowHeight = LOGICAL_HEIGHT * windowScale;
    bool widthSet = false;
    bool heightSet = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--scale" && i + 1 < argc) {
            windowScale = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--window-width" && i + 1 < argc) {
            windowWidth = std::max(LOGICAL_WIDTH, std::atoi(argv[++i]));
            widthSet = true;
        } else if (arg == "--window-height" && i + 1 < argc) {
            windowHeight = std::max(LOGICAL_HEIGHT, std::atoi(argv[++i]));
            heightSet = true;
        }
    }

    if (!widthSet) {
        windowWidth = LOGICAL_WIDTH * windowScale;
    }
    if (!heightSet) {
        windowHeight = LOGICAL_HEIGHT * windowScale;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Tank Duel",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, LOGICAL_WIDTH, LOGICAL_HEIGHT);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    Assets assets;
    if (!loadAssets(renderer, assets)) {
        SDL_Log("Failed to create tank sprites: %s", SDL_GetError());
        destroyAssets(assets);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    GameState state;

    state.player1.id = 1;
    state.player1.facingRight = true;
    state.player1.aimUp = SDL_SCANCODE_Q;
    state.player1.aimDown = SDL_SCANCODE_A;
    state.player1.powerUp = SDL_SCANCODE_W;
    state.player1.powerDown = SDL_SCANCODE_S;
    state.player1.fire = SDL_SCANCODE_SPACE;
    state.player1.nextAmmo = SDL_SCANCODE_E;

    state.player2.id = 2;
    state.player2.facingRight = false;
    state.player2.aimUp = SDL_SCANCODE_I;
    state.player2.aimDown = SDL_SCANCODE_K;
    state.player2.powerUp = SDL_SCANCODE_O;
    state.player2.powerDown = SDL_SCANCODE_L;
    state.player2.fire = SDL_SCANCODE_RETURN;
    state.player2.nextAmmo = SDL_SCANCODE_P;

    resetMatch(state);

    bool running = true;
    Uint32 lastTicks = SDL_GetTicks();

    while (running) {
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) {
                running = false;
            }
        }

        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTicks) / 1000.0f;
        lastTicks = now;

        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        if (!state.matchOver) {
            updateTank(state.player1, keys, dt, state.projectiles);
            updateTank(state.player2, keys, dt, state.projectiles);
            updateProjectiles(state, dt);
        } else {
            state.resetTimer -= dt;
            if (state.resetTimer <= 0.0f) {
                resetMatch(state);
            }
        }

        updateExplosions(state.explosions, dt);
        updateNapalmPatches(state, dt);
        applyGravityToTank(state.player1, state.terrainHeights, dt);
        applyGravityToTank(state.player2, state.terrainHeights, dt);
        if (state.player1.exploding) {
            state.player1.explosionTimer -= dt;
            if (state.player1.explosionTimer <= 0.0f) {
                state.player1.exploding = false;
            }
        }
        if (state.player2.exploding) {
            state.player2.explosionTimer -= dt;
            if (state.player2.explosionTimer <= 0.0f) {
                state.player2.exploding = false;
            }
        }

        drawBackground(renderer);
        drawTerrain(renderer, state.terrainHeights, state.terrainSubstrate);
        drawScenery(renderer, state.scenery);
        drawNapalmPatches(renderer, state.napalmPatches);
        drawProjectiles(renderer, state.projectiles);
        drawExplosions(renderer, state.explosions);
        drawTank(renderer, state.player1, assets, true);
        drawTank(renderer, state.player2, assets, false);
        drawUI(renderer, state);

        if (state.matchOver) {
            drawBanner(renderer, state.winner);
        }

        SDL_RenderPresent(renderer);
    }

    destroyAssets(assets);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
