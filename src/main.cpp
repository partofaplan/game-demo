// src/main.cpp
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr int LOGICAL_WIDTH  = 320;
constexpr int LOGICAL_HEIGHT = 240;
constexpr int WINDOW_SCALE   = 3;

constexpr float PROJECTILE_SPEED_SHELL  = 160.0f;
constexpr float PROJECTILE_SPEED_ROCKET = 110.0f;
constexpr int PROJECTILE_DAMAGE_SHELL   = 20;
constexpr int PROJECTILE_DAMAGE_ROCKET  = 40;
constexpr int TANK_HP = 100;
constexpr float RELOAD_TIME = 0.45f;
constexpr float GRAVITY = 120.0f;
constexpr float TURRET_ROT_SPEED = 120.0f;
constexpr float MAX_TURRET_SWING = 90.0f;
constexpr float DEG2RAD = 0.0174532925f;
constexpr float GROUND_Y = LOGICAL_HEIGHT - 30.0f;

constexpr float TANK_COLLISION_WIDTH = 36.0f;
constexpr float TANK_COLLISION_HEIGHT = 18.0f;
constexpr float TURRET_PIVOT_WORLD_OFFSET_Y = -2.0f;
constexpr float MUZZLE_LENGTH = 32.0f;

constexpr float HULL_SPRITE_WIDTH = 72.0f;
constexpr float HULL_SPRITE_HEIGHT = 28.0f;
constexpr float HULL_OFFSET_X = (HULL_SPRITE_WIDTH - TANK_COLLISION_WIDTH) * 0.5f;
constexpr float HULL_OFFSET_Y = 10.0f;

constexpr float TURRET_SPRITE_WIDTH = 64.0f;
constexpr float TURRET_SPRITE_HEIGHT = 24.0f;
constexpr float TURRET_PIVOT_X = 18.0f;
constexpr float TURRET_PIVOT_Y = 16.0f;

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
    const int w = static_cast<int>(HULL_SPRITE_WIDTH);
    const int h = static_cast<int>(HULL_SPRITE_HEIGHT);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        return nullptr;
    }

    SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

    const SDL_Color trackDark{ 46, 39, 34, 255 };
    const SDL_Color trackLight{ 83, 72, 63, 255 };
    const SDL_Color hullShadow{ 96, 82, 64, 255 };
    const SDL_Color hullBase{ 122, 104, 80, 255 };
    const SDL_Color hullHighlight{ 176, 158, 126, 255 };
    const SDL_Color detail{ 204, 188, 152, 255 };
    const SDL_Color vents{ 64, 70, 62, 255 };

    fillSurfaceRect(surface, 4, 21, 64, 6, trackDark);
    fillSurfaceRect(surface, 4, 21, 64, 2, trackLight);
    fillSurfaceRect(surface, 8, 24, 6, 3, trackLight);
    fillSurfaceRect(surface, 18, 24, 6, 3, trackLight);
    fillSurfaceRect(surface, 28, 24, 6, 3, trackLight);
    fillSurfaceRect(surface, 38, 24, 6, 3, trackLight);
    fillSurfaceRect(surface, 48, 24, 6, 3, trackLight);
    fillSurfaceRect(surface, 58, 24, 6, 3, trackLight);

    fillSurfaceRect(surface, 6, 18, 60, 4, hullShadow);
    fillSurfaceRect(surface, 8, 16, 56, 7, hullBase);
    fillSurfaceRect(surface, 8, 16, 56, 2, hullHighlight);
    fillSurfaceRect(surface, 14, 20, 48, 2, hullShadow);

    fillSurfaceRect(surface, 12, 12, 44, 5, hullBase);
    fillSurfaceRect(surface, 12, 12, 44, 2, hullHighlight);
    fillSurfaceRect(surface, 16, 9, 32, 3, hullBase);
    fillSurfaceRect(surface, 16, 9, 32, 1, hullHighlight);

    fillSurfaceRect(surface, 54, 12, 12, 5, hullShadow);
    fillSurfaceRect(surface, 56, 16, 10, 4, hullShadow);
    fillSurfaceRect(surface, 8, 16, 6, 6, hullShadow);

    fillSurfaceRect(surface, 18, 13, 26, 2, vents);
    fillSurfaceRect(surface, 16, 18, 8, 1, detail);
    fillSurfaceRect(surface, 44, 18, 10, 1, detail);
    fillSurfaceRect(surface, 22, 20, 20, 2, detail);

    fillSurfaceRect(surface, 14, 14, 8, 2, detail);
    fillSurfaceRect(surface, 40, 14, 10, 2, detail);
    fillSurfaceRect(surface, 24, 10, 2, 2, detail);
    fillSurfaceRect(surface, 34, 10, 2, 2, detail);

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    SDL_FreeSurface(surface);
    return tex;
}

SDL_Texture* createTankTurretTexture(SDL_Renderer* renderer) {
    const int w = static_cast<int>(TURRET_SPRITE_WIDTH);
    const int h = static_cast<int>(TURRET_SPRITE_HEIGHT);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        return nullptr;
    }

    SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

    const SDL_Color hullShadow{ 96, 82, 64, 255 };
    const SDL_Color hullBase{ 122, 104, 80, 255 };
    const SDL_Color hullHighlight{ 176, 158, 126, 255 };
    const SDL_Color detail{ 204, 188, 152, 255 };
    const SDL_Color muzzle{ 52, 48, 44, 255 };

    fillSurfaceRect(surface, 2, 10, 30, 8, hullBase);
    fillSurfaceRect(surface, 2, 10, 30, 2, hullHighlight);
    fillSurfaceRect(surface, 4, 8, 22, 4, hullBase);
    fillSurfaceRect(surface, 4, 8, 22, 1, hullHighlight);

    fillSurfaceRect(surface, 10, 6, 10, 3, hullBase);
    fillSurfaceRect(surface, 10, 6, 10, 1, hullHighlight);
    fillSurfaceRect(surface, 8, 4, 8, 2, hullBase);
    fillSurfaceRect(surface, 8, 4, 8, 1, hullHighlight);

    fillSurfaceRect(surface, 18, 12, 12, 4, hullShadow);
    fillSurfaceRect(surface, 18, 12, 12, 1, hullHighlight);

    fillSurfaceRect(surface, 26, 12, 30, 4, hullBase);
    fillSurfaceRect(surface, 26, 12, 30, 1, hullHighlight);
    fillSurfaceRect(surface, 56, 11, 6, 6, muzzle);
    fillSurfaceRect(surface, 30, 14, 24, 2, hullShadow);

    fillSurfaceRect(surface, 6, 12, 6, 2, detail);
    fillSurfaceRect(surface, 16, 9, 4, 2, detail);
    fillSurfaceRect(surface, 22, 9, 4, 2, detail);

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    SDL_FreeSurface(surface);
    return tex;
}

bool loadAssets(SDL_Renderer* renderer, Assets& assets) {
    assets.hull = createTankHullTexture(renderer);
    if (!assets.hull) {
        return false;
    }
    assets.turret = createTankTurretTexture(renderer);
    if (!assets.turret) {
        destroyAssets(assets);
        return false;
    }
    return true;
}

enum class ProjectileKind { Shell, Rocket };

struct Projectile {
    SDL_FRect rect{};
    SDL_FPoint velocity{};
    ProjectileKind kind{};
    int damage{};
    int owner{};
    bool alive{true};
};

struct Tank {
    SDL_FRect rect{};
    float turretAngleDeg{45.0f};
    float reloadTimer{0.0f};
    ProjectileKind selected{ProjectileKind::Shell};
    int hp{TANK_HP};
    SDL_Scancode aimUp{};
    SDL_Scancode aimDown{};
    SDL_Scancode fire{};
    SDL_Scancode nextAmmo{};
    int id{};
    bool facingRight{true};
};

struct GameState {
    Tank player1{};
    Tank player2{};
    std::vector<Projectile> projectiles{};
    bool matchOver{false};
    int winner{0};
    float resetTimer{2.0f};
};

SDL_FRect makeTankRect(float x, float y) {
    return SDL_FRect{ x, y, TANK_COLLISION_WIDTH, TANK_COLLISION_HEIGHT };
}

SDL_Color palette(int index) {
    switch (index) {
        case 0: return SDL_Color{ 12, 12, 26, 255 };
        case 1: return SDL_Color{ 83, 135, 59, 255 };
        case 2: return SDL_Color{ 196, 217, 161, 255 };
        case 3: return SDL_Color{ 217, 87, 99, 255 };
        case 4: return SDL_Color{ 44, 54, 63, 255 };
        case 5: return SDL_Color{ 90, 67, 56, 255 };
        default: return SDL_Color{ 255, 255, 255, 255 };
    }
}

void resetMatch(GameState& state) {
    state.projectiles.clear();
    state.matchOver = false;
    state.winner = 0;
    state.resetTimer = 2.0f;

    state.player1.rect = makeTankRect(24.0f, GROUND_Y - TANK_COLLISION_HEIGHT);
    state.player2.rect = makeTankRect(LOGICAL_WIDTH - 60.0f, GROUND_Y - TANK_COLLISION_HEIGHT);

    state.player1.turretAngleDeg = 45.0f;
    state.player2.turretAngleDeg = 45.0f;

    state.player1.reloadTimer = 0.0f;
    state.player2.reloadTimer = 0.0f;

    state.player1.selected = ProjectileKind::Shell;
    state.player2.selected = ProjectileKind::Shell;

    state.player1.hp = TANK_HP;
    state.player2.hp = TANK_HP;
}

float turretWorldAngleDeg(const Tank& tank) {
    return tank.facingRight ? tank.turretAngleDeg : (180.0f - tank.turretAngleDeg);
}

Projectile spawnProjectile(const Tank& tank) {
    Projectile proj;
    proj.kind = tank.selected;
    proj.owner = tank.id;
    proj.damage = (proj.kind == ProjectileKind::Shell) ? PROJECTILE_DAMAGE_SHELL : PROJECTILE_DAMAGE_ROCKET;
    float speed = (proj.kind == ProjectileKind::Shell) ? PROJECTILE_SPEED_SHELL : PROJECTILE_SPEED_ROCKET;

    constexpr float shotSize = 4.0f;
    float angleDeg = turretWorldAngleDeg(tank);
    float angleRad = angleDeg * DEG2RAD;
    float pivotX = tank.rect.x + tank.rect.w * 0.5f;
    float pivotY = tank.rect.y + TURRET_PIVOT_WORLD_OFFSET_Y;

    proj.rect.w = shotSize;
    proj.rect.h = shotSize;
    proj.rect.x = pivotX - shotSize * 0.5f + std::cos(angleRad) * MUZZLE_LENGTH;
    proj.rect.y = pivotY - shotSize * 0.5f - std::sin(angleRad) * MUZZLE_LENGTH;
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

    if (keys[tank.aimUp]) {
        tank.turretAngleDeg += TURRET_ROT_SPEED * dt;
    }
    if (keys[tank.aimDown]) {
        tank.turretAngleDeg -= TURRET_ROT_SPEED * dt;
    }

    if (tank.turretAngleDeg < 0.0f) tank.turretAngleDeg = 0.0f;
    if (tank.turretAngleDeg > MAX_TURRET_SWING) tank.turretAngleDeg = MAX_TURRET_SWING;

    if (keys[tank.nextAmmo]) {
        tank.selected = (tank.selected == ProjectileKind::Shell) ? ProjectileKind::Rocket : ProjectileKind::Shell;
    }

    if (keys[tank.fire] && tank.reloadTimer <= 0.0f) {
        projectiles.push_back(spawnProjectile(tank));
        tank.reloadTimer = RELOAD_TIME;
    }
}

bool intersects(const SDL_FRect& a, const SDL_FRect& b) {
    return !(a.x + a.w <= b.x || b.x + b.w <= a.x ||
             a.y + a.h <= b.y || b.y + b.h <= a.y);
}

void updateProjectiles(GameState& state, float dt) {
    for (auto& proj : state.projectiles) {
        if (!proj.alive) continue;

        proj.velocity.y += GRAVITY * dt;
        proj.rect.x += proj.velocity.x * dt;
        proj.rect.y += proj.velocity.y * dt;

        if (proj.rect.x + proj.rect.w < 0.0f || proj.rect.x > LOGICAL_WIDTH ||
            proj.rect.y > LOGICAL_HEIGHT) {
            proj.alive = false;
            continue;
        }

        if (proj.rect.y + proj.rect.h >= GROUND_Y) {
            proj.alive = false;
            continue;
        }

        if (!state.matchOver && proj.owner != 1 && intersects(proj.rect, state.player1.rect)) {
            state.player1.hp -= proj.damage;
            proj.alive = false;
            if (state.player1.hp <= 0) {
                state.matchOver = true;
                state.winner = 2;
                state.resetTimer = 3.0f;
            }
        } else if (!state.matchOver && proj.owner != 2 && intersects(proj.rect, state.player2.rect)) {
            state.player2.hp -= proj.damage;
            proj.alive = false;
            if (state.player2.hp <= 0) {
                state.matchOver = true;
                state.winner = 1;
                state.resetTimer = 3.0f;
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

void drawTank(SDL_Renderer* renderer, const Tank& tank, const Assets& assets, bool isPlayerOne) {
    const Uint8 hullR = isPlayerOne ? 120 : 180;
    const Uint8 hullG = isPlayerOne ? 150 : 155;
    const Uint8 hullB = isPlayerOne ? 110 : 120;
    const Uint8 turretR = isPlayerOne ? 130 : 188;
    const Uint8 turretG = isPlayerOne ? 170 : 168;
    const Uint8 turretB = isPlayerOne ? 120 : 138;

    SDL_SetTextureColorMod(assets.hull, hullR, hullG, hullB);
    SDL_SetTextureColorMod(assets.turret, turretR, turretG, turretB);

    SDL_FRect hullDest{
        tank.rect.x - HULL_OFFSET_X,
        tank.rect.y - HULL_OFFSET_Y,
        HULL_SPRITE_WIDTH,
        HULL_SPRITE_HEIGHT
    };

    SDL_Rect hullDestInt{
        static_cast<int>(std::lround(hullDest.x)),
        static_cast<int>(std::lround(hullDest.y)),
        static_cast<int>(std::lround(hullDest.w)),
        static_cast<int>(std::lround(hullDest.h))
    };

    SDL_RenderCopy(renderer, assets.hull, nullptr, &hullDestInt);

    float pivotWorldX = tank.rect.x + tank.rect.w * 0.5f;
    float pivotWorldY = tank.rect.y + TURRET_PIVOT_WORLD_OFFSET_Y;

    SDL_Rect turretDest{
        static_cast<int>(std::lround(pivotWorldX - TURRET_PIVOT_X)),
        static_cast<int>(std::lround(pivotWorldY - TURRET_PIVOT_Y)),
        static_cast<int>(std::lround(TURRET_SPRITE_WIDTH)),
        static_cast<int>(std::lround(TURRET_SPRITE_HEIGHT))
    };

    SDL_Point pivot{
        static_cast<int>(std::lround(TURRET_PIVOT_X)),
        static_cast<int>(std::lround(TURRET_PIVOT_Y))
    };

    double renderAngle = -static_cast<double>(turretWorldAngleDeg(tank));
    SDL_RenderCopyEx(renderer, assets.turret, nullptr, &turretDest, renderAngle, &pivot, SDL_FLIP_NONE);
}

void drawProjectiles(SDL_Renderer* renderer, const std::vector<Projectile>& projectiles) {
    for (const auto& proj : projectiles) {
        SDL_Color color = (proj.kind == ProjectileKind::Shell) ? SDL_Color{ 244, 226, 161, 255 }
                                                               : SDL_Color{ 255, 149, 128, 255 };
        drawRect(renderer, proj.rect, color);
    }
}

void drawUI(SDL_Renderer* renderer, const GameState& state) {
    SDL_SetRenderDrawColor(renderer, palette(4).r, palette(4).g, palette(4).b, 255);
    SDL_RenderDrawLine(renderer, 8, 16, LOGICAL_WIDTH - 8, 16);

    SDL_FRect p1Hp{ 16.0f, 8.0f, (state.player1.hp / static_cast<float>(TANK_HP)) * 96.0f, 6.0f };
    SDL_FRect p2Hp{ LOGICAL_WIDTH - 112.0f, 8.0f, (state.player2.hp / static_cast<float>(TANK_HP)) * 96.0f, 6.0f };
    drawRect(renderer, p1Hp, palette(1));
    drawRect(renderer, p2Hp, palette(3));
}

void drawBanner(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect banner{ 40, LOGICAL_HEIGHT / 2 - 20, LOGICAL_WIDTH - 80, 40 };
    SDL_RenderFillRect(renderer, &banner);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &banner);
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
    state.player1.aimUp = SDL_SCANCODE_W;
    state.player1.aimDown = SDL_SCANCODE_S;
    state.player1.fire = SDL_SCANCODE_F;
    state.player1.nextAmmo = SDL_SCANCODE_G;

    state.player2.id = 2;
    state.player2.facingRight = false;
    state.player2.aimUp = SDL_SCANCODE_UP;
    state.player2.aimDown = SDL_SCANCODE_DOWN;
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

        SDL_SetRenderDrawColor(renderer, palette(0).r, palette(0).g, palette(0).b, 255);
        SDL_RenderClear(renderer);

        SDL_FRect ground{ 0.0f, GROUND_Y, static_cast<float>(LOGICAL_WIDTH), static_cast<float>(LOGICAL_HEIGHT) - GROUND_Y };
        drawRect(renderer, ground, palette(5));

        SDL_SetRenderDrawColor(renderer, palette(4).r, palette(4).g, palette(4).b, 255);
        SDL_RenderDrawLine(renderer, 0, static_cast<int>(GROUND_Y), LOGICAL_WIDTH, static_cast<int>(GROUND_Y));

        drawProjectiles(renderer, state.projectiles);
        drawTank(renderer, state.player1, assets, true);
        drawTank(renderer, state.player2, assets, false);
        drawUI(renderer, state);

        if (state.matchOver) {
            drawBanner(renderer);
        }

        SDL_RenderPresent(renderer);
    }

    destroyAssets(assets);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

