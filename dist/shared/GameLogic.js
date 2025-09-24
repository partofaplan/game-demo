"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.GameLogic = void 0;
const types_1 = require("./types");
class GameLogic {
    static createTank(id, x, isPlayer2 = false) {
        return {
            id,
            position: { x, y: 0 },
            hp: types_1.GAME_CONSTANTS.TANK_HP,
            maxHp: types_1.GAME_CONSTANTS.TANK_HP,
            turretAngle: isPlayer2 ? 180 : 0, // Player 2 faces left (180°), Player 1 faces right (0°)
            power: types_1.GAME_CONSTANTS.DEFAULT_LAUNCH_SPEED,
            currentAmmo: types_1.ProjectileKind.Mortar,
            isAlive: true,
            shieldActive: false,
            shieldCooldown: 0,
            team: isPlayer2 ? 'green' : 'red'
        };
    }
    static createProjectile(kind, position, velocity, ownerId) {
        const config = this.getProjectileConfig(kind);
        return {
            id: Math.random().toString(36),
            kind,
            position: { ...position },
            velocity: { ...velocity },
            radius: config.radius,
            damage: config.damage,
            ownerId,
            splitTime: kind === types_1.ProjectileKind.Cluster ? types_1.GAME_CONSTANTS.CLUSTER_SPLIT_TIME : undefined,
            isActive: true
        };
    }
    static getProjectileConfig(kind) {
        switch (kind) {
            case types_1.ProjectileKind.Mortar:
                return { radius: types_1.GAME_CONSTANTS.RADIUS_MORTAR, damage: types_1.GAME_CONSTANTS.DAMAGE_MORTAR };
            case types_1.ProjectileKind.Cluster:
                return { radius: types_1.GAME_CONSTANTS.RADIUS_CLUSTER, damage: types_1.GAME_CONSTANTS.DAMAGE_CLUSTER };
            case types_1.ProjectileKind.ClusterShard:
                return { radius: types_1.GAME_CONSTANTS.RADIUS_CLUSTER_SHARD, damage: types_1.GAME_CONSTANTS.DAMAGE_CLUSTER_SHARD };
            case types_1.ProjectileKind.Napalm:
                return { radius: types_1.GAME_CONSTANTS.RADIUS_NAPALM, damage: types_1.GAME_CONSTANTS.DAMAGE_NAPALM_DIRECT };
            case types_1.ProjectileKind.Dirtgun:
                return { radius: types_1.GAME_CONSTANTS.RADIUS_DIRTGUN, damage: types_1.GAME_CONSTANTS.DAMAGE_DIRTGUN };
        }
    }
    static generateTerrain() {
        const terrain = new Array(types_1.GAME_CONSTANTS.LOGICAL_WIDTH).fill(0);
        // Generate hilly terrain
        for (let x = 0; x < types_1.GAME_CONSTANTS.LOGICAL_WIDTH; x++) {
            const normalized = x / types_1.GAME_CONSTANTS.LOGICAL_WIDTH;
            let height = types_1.GAME_CONSTANTS.TERRAIN_BASELINE;
            // Multiple sine waves for varied terrain
            height += Math.sin(normalized * Math.PI * 2) * 30;
            height += Math.sin(normalized * Math.PI * 4) * 15;
            height += Math.sin(normalized * Math.PI * 8) * 8;
            // Add some randomness
            height += (Math.random() - 0.5) * 10;
            terrain[x] = Math.floor(Math.max(height, types_1.GAME_CONSTANTS.TERRAIN_BASELINE - 50));
        }
        return terrain;
    }
    static updateProjectiles(gameState, deltaTime) {
        const newProjectiles = [];
        for (const projectile of gameState.projectiles) {
            if (!projectile.isActive)
                continue;
            // Update position
            projectile.position.x += projectile.velocity.x * deltaTime;
            projectile.position.y += projectile.velocity.y * deltaTime;
            // Apply gravity
            projectile.velocity.y += types_1.GAME_CONSTANTS.GRAVITY * deltaTime;
            // Handle cluster bomb splitting
            if (projectile.kind === types_1.ProjectileKind.Cluster && projectile.splitTime !== undefined) {
                projectile.splitTime -= deltaTime;
                if (projectile.splitTime <= 0) {
                    // Create cluster shards
                    for (let i = 0; i < 5; i++) {
                        const angle = (i / 5) * Math.PI * 2;
                        const speed = 50 + Math.random() * 30;
                        const shardVelocity = {
                            x: Math.cos(angle) * speed + projectile.velocity.x * 0.3,
                            y: Math.sin(angle) * speed + projectile.velocity.y * 0.3
                        };
                        newProjectiles.push(this.createProjectile(types_1.ProjectileKind.ClusterShard, { ...projectile.position }, shardVelocity, projectile.ownerId));
                    }
                    projectile.isActive = false;
                    continue;
                }
            }
            // Check terrain collision
            const terrainHeight = this.getTerrainHeight(gameState.terrain, projectile.position.x);
            if (projectile.position.y >= terrainHeight) {
                this.handleProjectileImpact(gameState, projectile);
                projectile.isActive = false;
                continue;
            }
            // Check tank collision
            for (const tank of gameState.tanks.values()) {
                if (tank.id === projectile.ownerId || !tank.isAlive)
                    continue;
                if (this.checkTankCollision(tank, projectile)) {
                    this.handleTankHit(tank, projectile, gameState);
                    projectile.isActive = false;
                    break;
                }
            }
            // Remove projectiles that go off screen
            if (projectile.position.x < -50 || projectile.position.x > types_1.GAME_CONSTANTS.LOGICAL_WIDTH + 50 ||
                projectile.position.y > types_1.GAME_CONSTANTS.LOGICAL_HEIGHT + 50) {
                projectile.isActive = false;
            }
        }
        // Add new projectiles and filter out inactive ones
        gameState.projectiles = [...gameState.projectiles.filter(p => p.isActive), ...newProjectiles];
    }
    static getTerrainHeight(terrain, x) {
        const index = Math.floor(x);
        if (index < 0 || index >= terrain.length) {
            return types_1.GAME_CONSTANTS.LOGICAL_HEIGHT;
        }
        return terrain[index];
    }
    static checkTankCollision(tank, projectile) {
        const dx = tank.position.x - projectile.position.x;
        const dy = tank.position.y - projectile.position.y;
        const distance = Math.sqrt(dx * dx + dy * dy);
        return distance < (types_1.GAME_CONSTANTS.TANK_COLLISION_WIDTH / 2 + projectile.radius);
    }
    static handleProjectileImpact(gameState, projectile) {
        const explosion = {
            id: Math.random().toString(36),
            position: { ...projectile.position },
            radius: projectile.radius * 8, // Explosion radius is larger than projectile
            duration: types_1.GAME_CONSTANTS.EXPLOSION_DURATION,
            elapsed: 0,
            kind: 'normal'
        };
        gameState.explosions.push(explosion);
        // Terrain modification
        this.modifyTerrain(gameState.terrain, projectile);
        // Create napalm patch for napalm projectiles
        if (projectile.kind === types_1.ProjectileKind.Napalm) {
            const napalmPatch = {
                id: Math.random().toString(36),
                position: { ...projectile.position },
                radius: projectile.radius * 6,
                duration: types_1.GAME_CONSTANTS.NAPALM_BURN_DURATION,
                elapsed: 0
            };
            gameState.napalmPatches.push(napalmPatch);
        }
        // Damage nearby tanks
        for (const tank of gameState.tanks.values()) {
            if (!tank.isAlive)
                continue;
            const distance = this.getDistance(tank.position, projectile.position);
            if (distance < explosion.radius) {
                const damageRatio = 1 - (distance / explosion.radius);
                const damage = Math.floor(projectile.damage * damageRatio);
                if (!tank.shieldActive && damage > 0) {
                    tank.hp = Math.max(0, tank.hp - damage);
                    if (tank.hp <= 0) {
                        tank.isAlive = false;
                        this.createTankExplosion(gameState, tank);
                    }
                }
            }
        }
    }
    static handleTankHit(tank, projectile, gameState) {
        if (!tank.shieldActive) {
            tank.hp = Math.max(0, tank.hp - projectile.damage);
            if (tank.hp <= 0) {
                tank.isAlive = false;
                this.createTankExplosion(gameState, tank);
            }
        }
    }
    static createTankExplosion(gameState, tank) {
        const explosion = {
            id: Math.random().toString(36),
            position: { ...tank.position },
            radius: 40,
            duration: types_1.GAME_CONSTANTS.TANK_EXPLOSION_DURATION,
            elapsed: 0,
            kind: 'tank'
        };
        gameState.explosions.push(explosion);
    }
    static modifyTerrain(terrain, projectile) {
        const centerX = Math.floor(projectile.position.x);
        const centerY = projectile.position.y;
        const radius = projectile.radius * 4; // Terrain modification radius
        for (let x = centerX - radius; x <= centerX + radius; x++) {
            if (x < 0 || x >= terrain.length)
                continue;
            const distance = Math.abs(x - centerX);
            if (distance <= radius) {
                const falloff = 1 - (distance / radius);
                if (projectile.kind === types_1.ProjectileKind.Dirtgun) {
                    // Dirtgun builds terrain
                    terrain[x] = Math.min(terrain[x] - falloff * 20, centerY);
                }
                else {
                    // Other projectiles destroy terrain
                    terrain[x] = Math.max(terrain[x] + falloff * 20, types_1.GAME_CONSTANTS.TERRAIN_BASELINE + 20);
                }
            }
        }
    }
    static updateExplosions(gameState, deltaTime) {
        gameState.explosions = gameState.explosions.filter(explosion => {
            explosion.elapsed += deltaTime;
            return explosion.elapsed < explosion.duration;
        });
    }
    static updateNapalmPatches(gameState, deltaTime) {
        gameState.napalmPatches = gameState.napalmPatches.filter(patch => {
            patch.elapsed += deltaTime;
            // Damage tanks in napalm
            for (const tank of gameState.tanks.values()) {
                if (!tank.isAlive || tank.shieldActive)
                    continue;
                const distance = this.getDistance(tank.position, patch.position);
                if (distance < patch.radius) {
                    tank.hp = Math.max(0, tank.hp - types_1.GAME_CONSTANTS.NAPALM_EROSION_RATE * deltaTime);
                    if (tank.hp <= 0) {
                        tank.isAlive = false;
                        this.createTankExplosion(gameState, tank);
                    }
                }
            }
            return patch.elapsed < patch.duration;
        });
    }
    static updateTanks(gameState, deltaTime) {
        for (const tank of gameState.tanks.values()) {
            if (tank.shieldCooldown > 0) {
                tank.shieldCooldown -= deltaTime;
                if (tank.shieldCooldown <= 0) {
                    tank.shieldActive = false;
                }
            }
            // Update tank position based on terrain
            if (tank.isAlive) {
                tank.position.y = this.getTerrainHeight(gameState.terrain, tank.position.x) - types_1.GAME_CONSTANTS.TANK_COLLISION_HEIGHT;
            }
        }
    }
    static getDistance(pos1, pos2) {
        const dx = pos1.x - pos2.x;
        const dy = pos1.y - pos2.y;
        return Math.sqrt(dx * dx + dy * dy);
    }
    static nextAmmoType(current) {
        switch (current) {
            case types_1.ProjectileKind.Mortar: return types_1.ProjectileKind.Cluster;
            case types_1.ProjectileKind.Cluster: return types_1.ProjectileKind.Napalm;
            case types_1.ProjectileKind.Napalm: return types_1.ProjectileKind.Dirtgun;
            case types_1.ProjectileKind.Dirtgun: return types_1.ProjectileKind.Mortar;
            default: return types_1.ProjectileKind.Mortar;
        }
    }
    static fireTank(tank) {
        if (!tank.isAlive)
            return null;
        const angle = tank.turretAngle * types_1.GAME_CONSTANTS.DEG2RAD;
        const velocity = {
            x: Math.cos(angle) * tank.power,
            y: Math.sin(angle) * tank.power
        };
        const muzzleDistance = 30; // Distance from tank center to muzzle (doubled for larger tank)
        const startPosition = {
            x: tank.position.x + Math.cos(angle) * muzzleDistance,
            y: tank.position.y + Math.sin(angle) * muzzleDistance
        };
        return this.createProjectile(tank.currentAmmo, startPosition, velocity, tank.id);
    }
}
exports.GameLogic = GameLogic;
