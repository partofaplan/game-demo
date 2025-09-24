export interface Player {
  id: string
  name: string
  roomId: string
}

export interface Position {
  x: number
  y: number
}

export interface Tank {
  id: string
  position: Position
  hp: number
  maxHp: number
  turretAngle: number
  power: number
  currentAmmo: ProjectileKind
  isAlive: boolean
  shieldActive: boolean
  shieldCooldown: number
}

export enum ProjectileKind {
  Mortar = 'mortar',
  Cluster = 'cluster',
  ClusterShard = 'cluster_shard',
  Napalm = 'napalm',
  Dirtgun = 'dirtgun'
}

export interface Projectile {
  id: string
  kind: ProjectileKind
  position: Position
  velocity: Position
  radius: number
  damage: number
  ownerId: string
  splitTime?: number
  isActive: boolean
}

export interface Explosion {
  id: string
  position: Position
  radius: number
  duration: number
  elapsed: number
  kind: 'normal' | 'tank' | 'napalm'
}

export interface NapalmPatch {
  id: string
  position: Position
  radius: number
  duration: number
  elapsed: number
}

export interface GameState {
  tanks: Map<string, Tank>
  projectiles: Projectile[]
  explosions: Explosion[]
  napalmPatches: NapalmPatch[]
  terrain: number[]
  currentTurn: string | null
  turnTimeLeft: number
  gamePhase: 'waiting' | 'playing' | 'ended'
  winner: string | null
}

export interface PlayerAction {
  type: 'aim' | 'power' | 'fire' | 'change_ammo' | 'shield'
  data?: any
}

export interface GameMessage {
  type: 'game_state' | 'player_joined' | 'player_left' | 'game_started' | 'game_ended' | 'turn_changed'
  data: any
}

// Game constants
export const GAME_CONSTANTS = {
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

  TANK_COLLISION_WIDTH: 9.0,
  TANK_COLLISION_HEIGHT: 5.0,

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
}