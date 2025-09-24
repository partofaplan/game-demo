"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.GAME_CONSTANTS = exports.ProjectileKind = void 0;
var ProjectileKind;
(function (ProjectileKind) {
    ProjectileKind["Mortar"] = "mortar";
    ProjectileKind["Cluster"] = "cluster";
    ProjectileKind["ClusterShard"] = "cluster_shard";
    ProjectileKind["Napalm"] = "napalm";
    ProjectileKind["Dirtgun"] = "dirtgun";
})(ProjectileKind || (exports.ProjectileKind = ProjectileKind = {}));
// Game constants
exports.GAME_CONSTANTS = {
    LOGICAL_WIDTH: 1280,
    LOGICAL_HEIGHT: 768,
    DEFAULT_LAUNCH_SPEED: 160.0,
    MIN_LAUNCH_SPEED: 90.0,
    MAX_LAUNCH_SPEED: 260.0,
    POWER_ADJUST_RATE: 110.0,
    DAMAGE_MORTAR: 24,
    DAMAGE_CLUSTER: 16,
    DAMAGE_CLUSTER_SHARD: 12,
    DAMAGE_NAPALM_DIRECT: 18,
    DAMAGE_DIRTGUN: 0,
    TANK_HP: 100,
    RELOAD_TIME: 0.45,
    GRAVITY: 120.0,
    TURRET_ROT_SPEED: 120.0,
    MAX_TURRET_SWING: 90.0,
    DEG2RAD: Math.PI / 180,
    TERRAIN_BASELINE: 768 - 140,
    TANK_COLLISION_WIDTH: 18.0,
    TANK_COLLISION_HEIGHT: 10.0,
    RADIUS_MORTAR: 3.2,
    RADIUS_CLUSTER: 3.0,
    RADIUS_CLUSTER_SHARD: 2.2,
    RADIUS_NAPALM: 3.8,
    RADIUS_DIRTGUN: 2.5,
    CLUSTER_SPLIT_TIME: 0.45,
    CLUSTER_SPREAD: 0.22,
    NAPALM_BURN_DURATION: 1.2,
    NAPALM_EROSION_RATE: 32.0,
    EXPLOSION_DURATION: 0.45,
    TANK_EXPLOSION_DURATION: 1.2,
    TURN_TIME_LIMIT: 30.0
};
