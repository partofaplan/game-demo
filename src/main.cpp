// src/main.cpp
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <string>
#include <vector>

namespace {
constexpr int LOGICAL_WIDTH  = 448;
constexpr int LOGICAL_HEIGHT = 336;
constexpr int WINDOW_SCALE   = 3;

constexpr float DEFAULT_LAUNCH_SPEED   = 160.0f;
constexpr float MIN_LAUNCH_SPEED       = 90.0f;
constexpr float MAX_LAUNCH_SPEED       = 260.0f;
constexpr float POWER_ADJUST_RATE      = 110.0f;
constexpr float ROCKET_SPEED_FACTOR    = 0.75f;
constexpr int PROJECTILE_DAMAGE_SHELL  = 20;
constexpr int PROJECTILE_DAMAGE_ROCKET = 40;
constexpr int TANK_HP = 100;
constexpr float RELOAD_TIME = 0.45f;
constexpr float GRAVITY = 120.0f;
constexpr float TURRET_ROT_SPEED = 120.0f;
constexpr float MAX_TURRET_SWING = 90.0f;
constexpr float DEG2RAD = 0.0174532925f;
constexpr float PI = 3.14159265f;

constexpr float TERRAIN_BASELINE = LOGICAL_HEIGHT - 70.0f;

constexpr float TANK_COLLISION_WIDTH = 12.5f;
constexpr float TANK_COLLISION_HEIGHT = 6.5f;

constexpr float TANK_SCALE = 0.35f;

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

constexpr float SHELL_RADIUS = 3.0f;
constexpr float ROCKET_RADIUS = 4.0f;
constexpr float EXPLOSION_DURATION = 0.45f;
constexpr float TANK_EXPLOSION_DURATION = 1.2f;

constexpr int GLYPH_WIDTH = 6;
constexpr int GLYPH_HEIGHT = 7;
constexpr int DEFAULT_GLYPH_PIXEL = 3;

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

enum class ProjectileKind { Shell, Rocket };

struct Projectile {
    SDL_FPoint position{};
    SDL_FPoint velocity{};
    float radius{SHELL_RADIUS};
    ProjectileKind kind{};
    int damage{};
    int owner{};
    bool alive{true};
};

struct Explosion {
    SDL_FPoint position{};
    float timer{EXPLOSION_DURATION};
    float duration{EXPLOSION_DURATION};
    float maxRadius{22.0f};
    bool isTankExplosion{false};
};

struct Tank {
    SDL_FRect rect{};
    float turretAngleDeg{45.0f};
    float reloadTimer{0.0f};
    float launchSpeed{DEFAULT_LAUNCH_SPEED};
    ProjectileKind selected{ProjectileKind::Shell};
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
};

struct GameState {
    Tank player1{};
    Tank player2{};
    std::vector<Projectile> projectiles{};
    std::vector<Explosion> explosions{};
    std::vector<int> terrainHeights{};
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

void generateTerrain(std::vector<int>& heights) {
    heights.resize(LOGICAL_WIDTH);
    for (int x = 0; x < LOGICAL_WIDTH; ++x) {
        float fx = static_cast<float>(x);
        float base = TERRAIN_BASELINE;
        base += std::sin(fx * 0.035f) * 14.0f;
        base += std::sin(fx * 0.11f + 1.2f) * 7.0f;
        base += std::sin(fx * 0.22f + 2.4f) * 4.0f;
        heights[x] = static_cast<int>(std::round(base));
    }

    struct Feature { int center; float radius; float magnitude; };
    const Feature craters[] = {
        { 90,  26.0f, 18.0f },
        { 210, 32.0f, 22.0f },
        { 330, 20.0f, 14.0f }
    };
    for (const auto& crater : craters) {
        for (int x = std::max(0, crater.center - static_cast<int>(crater.radius));
             x < std::min(LOGICAL_WIDTH, crater.center + static_cast<int>(crater.radius)); ++x) {
            float dx = static_cast<float>(x - crater.center);
            float t = std::clamp(std::abs(dx) / crater.radius, 0.0f, 1.0f);
            float depth = (std::cos(t * PI) * 0.5f + 0.5f) * crater.magnitude;
            heights[x] += static_cast<int>(std::round(depth));
        }
    }

    const Feature berms[] = {
        { 140, 18.0f, -16.0f },
        { 255, 22.0f, -20.0f },
        { 395, 16.0f, -12.0f }
    };
    for (const auto& berm : berms) {
        for (int x = std::max(0, berm.center - static_cast<int>(berm.radius));
             x < std::min(LOGICAL_WIDTH, berm.center + static_cast<int>(berm.radius)); ++x) {
            float dx = static_cast<float>(x - berm.center);
            float t = std::clamp(std::abs(dx) / berm.radius, 0.0f, 1.0f);
            float lift = (std::cos(t * PI) * 0.5f + 0.5f) * berm.magnitude;
            heights[x] += static_cast<int>(std::round(lift));
        }
    }

    for (int& h : heights) {
        h = std::clamp(h, LOGICAL_HEIGHT - 110, LOGICAL_HEIGHT - 28);
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
    proj.damage = (proj.kind == ProjectileKind::Shell) ? PROJECTILE_DAMAGE_SHELL : PROJECTILE_DAMAGE_ROCKET;
    proj.radius = (proj.kind == ProjectileKind::Shell) ? SHELL_RADIUS : ROCKET_RADIUS;

    float baseSpeed = tank.launchSpeed;
    float speed = (proj.kind == ProjectileKind::Rocket) ? baseSpeed * ROCKET_SPEED_FACTOR : baseSpeed;

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
        tank.selected = (tank.selected == ProjectileKind::Shell) ? ProjectileKind::Rocket : ProjectileKind::Shell;
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

void updateProjectiles(GameState& state, float dt) {
    for (auto& proj : state.projectiles) {
        if (!proj.alive) continue;

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
            state.explosions.push_back({proj.position, EXPLOSION_DURATION, EXPLOSION_DURATION, 22.0f, false});
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
constexpr GlyphRows GLYPH_E{ 0b111111, 0b100000, 0b100000, 0b111110, 0b100000, 0b100000, 0b111111 };
constexpr GlyphRows GLYPH_G{ 0b011110, 0b100001, 0b100000, 0b101111, 0b100001, 0b100001, 0b011110 };
constexpr GlyphRows GLYPH_M{ 0b100001, 0b110011, 0b101101, 0b100001, 0b100001, 0b100001, 0b100001 };
constexpr GlyphRows GLYPH_O{ 0b011110, 0b100001, 0b100001, 0b100001, 0b100001, 0b100001, 0b011110 };
constexpr GlyphRows GLYPH_V{ 0b100001, 0b100001, 0b100001, 0b100001, 0b010010, 0b010010, 0b001100 };
constexpr GlyphRows GLYPH_R{ 0b111110, 0b100001, 0b100001, 0b111110, 0b101000, 0b100100, 0b100011 };
constexpr GlyphRows GLYPH_P{ 0b111110, 0b100001, 0b100001, 0b111110, 0b100000, 0b100000, 0b100000 };
constexpr GlyphRows GLYPH_L{ 0b100000, 0b100000, 0b100000, 0b100000, 0b100000, 0b100000, 0b111111 };
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
        case 'E': return &GLYPH_E;
        case 'G': return &GLYPH_G;
        case 'M': return &GLYPH_M;
        case 'O': return &GLYPH_O;
        case 'V': return &GLYPH_V;
        case 'R': return &GLYPH_R;
        case 'P': return &GLYPH_P;
        case 'L': return &GLYPH_L;
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

void drawTerrain(SDL_Renderer* renderer, const std::vector<int>& heights) {
    SDL_Color base{ 96, 68, 52, 255 };
    SDL_Color highlight{ 214, 188, 148, 255 };
    SDL_Color shadow{ 42, 32, 28, 255 };
    SDL_Color midTone{ 136, 104, 80, 255 };
    SDL_Color rimLight{ 242, 208, 172, 255 };

    for (int x = 0; x < LOGICAL_WIDTH; ++x) {
        int top = heights[x];
        SDL_SetRenderDrawColor(renderer, base.r, base.g, base.b, 255);
        SDL_RenderDrawLine(renderer, x, top, x, LOGICAL_HEIGHT);
    }

    SDL_SetRenderDrawColor(renderer, midTone.r, midTone.g, midTone.b, 140);
    for (int x = 0; x < LOGICAL_WIDTH; x += 4) {
        int top = heights[x];
        int scribble = static_cast<int>(std::sin(x * 0.18f) * 3.5f);
        SDL_RenderDrawLine(renderer, x - 2, top + scribble, x + 6, top + scribble + 3);
    }

    SDL_SetRenderDrawColor(renderer, 128, 88, 60, 90);
    for (int x = 0; x < LOGICAL_WIDTH; x += 3) {
        int top = heights[x];
        SDL_RenderDrawLine(renderer, x, top + 2, x + 1, top + 6);
    }

    SDL_SetRenderDrawColor(renderer, shadow.r, shadow.g, shadow.b, 160);
    for (int x = 0; x < LOGICAL_WIDTH; ++x) {
        if (x == LOGICAL_WIDTH - 1) continue;
        int current = heights[x];
        int next = heights[x + 1];
        if (next > current) {
            SDL_RenderDrawLine(renderer, x + 1, current - 1, x + 1, next + 2);
        }
    }

    SDL_SetRenderDrawColor(renderer, highlight.r, highlight.g, highlight.b, 210);
    for (int x = 0; x < LOGICAL_WIDTH; ++x) {
        int top = heights[x];
        SDL_RenderDrawPoint(renderer, x, top);
        if (x % 5 == 0) {
            SDL_RenderDrawPoint(renderer, x, top - 1);
        }
    }

    SDL_SetRenderDrawColor(renderer, rimLight.r, rimLight.g, rimLight.b, 150);
    for (int x = 1; x < LOGICAL_WIDTH - 1; ++x) {
        int current = heights[x];
        int prev = heights[x - 1];
        int next = heights[x + 1];
        if (current <= prev && current <= next) {
            SDL_RenderDrawPoint(renderer, x, current - 1);
        }
    }

    SDL_SetRenderDrawColor(renderer, 58, 44, 36, 80);
    for (int x = 0; x < LOGICAL_WIDTH; x += 12) {
        int baseY = heights[x] + 4;
        SDL_RenderDrawLine(renderer, x - 3, baseY, x + 3, baseY + 6);
        SDL_RenderDrawLine(renderer, x + 3, baseY + 6, x + 6, baseY + 10);
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
        isPlayerOne ? 120 : 180,
        isPlayerOne ? 150 : 155,
        isPlayerOne ? 110 : 120);
    SDL_SetTextureColorMod(assets.turret,
        isPlayerOne ? 130 : 188,
        isPlayerOne ? 170 : 168,
        isPlayerOne ? 120 : 138);

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
        SDL_Color glow = (proj.kind == ProjectileKind::Shell)
            ? SDL_Color{ 255, 224, 170, 140 }
            : SDL_Color{ 255, 180, 160, 150 };
        SDL_Color core = (proj.kind == ProjectileKind::Shell)
            ? SDL_Color{ 255, 248, 220, 255 }
            : SDL_Color{ 255, 196, 120, 255 };
        drawFilledCircle(renderer, proj.position.x, proj.position.y, proj.radius + 1.5f, glow);
        drawFilledCircle(renderer, proj.position.x, proj.position.y, proj.radius, core);
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

    drawPowerBar(state.player1, 20.0f, 28.0f + 10.0f);
    drawPowerBar(state.player2, LOGICAL_WIDTH - 116.0f, 28.0f + 10.0f);
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
}

void resetMatch(GameState& state) {
    state.projectiles.clear();
    state.explosions.clear();
    state.matchOver = false;
    state.winner = 0;
    state.resetTimer = 2.0f;

    state.player1.rect = makeTankRect(60.0f, 0.0f);
    state.player2.rect = makeTankRect(LOGICAL_WIDTH - 95.0f, 0.0f);

    positionTankOnTerrain(state.player1, state.terrainHeights);
    positionTankOnTerrain(state.player2, state.terrainHeights);

    state.player1.turretAngleDeg = 45.0f;
    state.player2.turretAngleDeg = 45.0f;

    state.player1.reloadTimer = 0.0f;
    state.player2.reloadTimer = 0.0f;

    state.player1.launchSpeed = DEFAULT_LAUNCH_SPEED;
    state.player2.launchSpeed = DEFAULT_LAUNCH_SPEED;

    state.player1.selected = ProjectileKind::Shell;
    state.player2.selected = ProjectileKind::Shell;

    state.player1.hp = TANK_HP;
    state.player2.hp = TANK_HP;

    state.player1.exploding = false;
    state.player2.exploding = false;
    state.player1.explosionTimer = 0.0f;
    state.player2.explosionTimer = 0.0f;
}

} // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Tank Duel",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        LOGICAL_WIDTH * WINDOW_SCALE, LOGICAL_HEIGHT * WINDOW_SCALE,
        SDL_WINDOW_SHOWN);
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
    generateTerrain(state.terrainHeights);

    state.player1.id = 1;
    state.player1.facingRight = true;
    state.player1.aimUp = SDL_SCANCODE_W;
    state.player1.aimDown = SDL_SCANCODE_S;
    state.player1.powerUp = SDL_SCANCODE_E;
    state.player1.powerDown = SDL_SCANCODE_Q;
    state.player1.fire = SDL_SCANCODE_F;
    state.player1.nextAmmo = SDL_SCANCODE_G;

    state.player2.id = 2;
    state.player2.facingRight = false;
    state.player2.aimUp = SDL_SCANCODE_UP;
    state.player2.aimDown = SDL_SCANCODE_DOWN;
    state.player2.powerUp = SDL_SCANCODE_PAGEUP;
    state.player2.powerDown = SDL_SCANCODE_PAGEDOWN;
    state.player2.fire = SDL_SCANCODE_RCTRL;
    state.player2.nextAmmo = SDL_SCANCODE_RSHIFT;

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
        drawTerrain(renderer, state.terrainHeights);
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
