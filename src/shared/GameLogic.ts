import {
  Tank,
  Projectile,
  Position,
  ProjectileKind,
  GameState,
  Explosion,
  NapalmPatch,
  GAME_CONSTANTS as C
} from './types'

export class GameLogic {
  static createTank(id: string, x: number): Tank {
    return {
      id,
      position: { x, y: 0 },
      hp: C.TANK_HP,
      maxHp: C.TANK_HP,
      turretAngle: 0,
      power: C.DEFAULT_LAUNCH_SPEED,
      currentAmmo: ProjectileKind.Mortar,
      isAlive: true,
      shieldActive: false,
      shieldCooldown: 0
    }
  }

  static createProjectile(
    kind: ProjectileKind,
    position: Position,
    velocity: Position,
    ownerId: string
  ): Projectile {
    const config = this.getProjectileConfig(kind)

    return {
      id: Math.random().toString(36),
      kind,
      position: { ...position },
      velocity: { ...velocity },
      radius: config.radius,
      damage: config.damage,
      ownerId,
      splitTime: kind === ProjectileKind.Cluster ? C.CLUSTER_SPLIT_TIME : undefined,
      isActive: true
    }
  }

  static getProjectileConfig(kind: ProjectileKind) {
    switch (kind) {
      case ProjectileKind.Mortar:
        return { radius: C.RADIUS_MORTAR, damage: C.DAMAGE_MORTAR }
      case ProjectileKind.Cluster:
        return { radius: C.RADIUS_CLUSTER, damage: C.DAMAGE_CLUSTER }
      case ProjectileKind.ClusterShard:
        return { radius: C.RADIUS_CLUSTER_SHARD, damage: C.DAMAGE_CLUSTER_SHARD }
      case ProjectileKind.Napalm:
        return { radius: C.RADIUS_NAPALM, damage: C.DAMAGE_NAPALM_DIRECT }
      case ProjectileKind.Dirtgun:
        return { radius: C.RADIUS_DIRTGUN, damage: C.DAMAGE_DIRTGUN }
    }
  }

  static generateTerrain(): number[] {
    const terrain = new Array(C.LOGICAL_WIDTH).fill(0)

    // Generate hilly terrain
    for (let x = 0; x < C.LOGICAL_WIDTH; x++) {
      const normalized = x / C.LOGICAL_WIDTH
      let height = C.TERRAIN_BASELINE

      // Multiple sine waves for varied terrain
      height += Math.sin(normalized * Math.PI * 2) * 30
      height += Math.sin(normalized * Math.PI * 4) * 15
      height += Math.sin(normalized * Math.PI * 8) * 8

      // Add some randomness
      height += (Math.random() - 0.5) * 10

      terrain[x] = Math.floor(Math.max(height, C.TERRAIN_BASELINE - 50))
    }

    return terrain
  }

  static updateProjectiles(gameState: GameState, deltaTime: number): void {
    const newProjectiles: Projectile[] = []

    for (const projectile of gameState.projectiles) {
      if (!projectile.isActive) continue

      // Update position
      projectile.position.x += projectile.velocity.x * deltaTime
      projectile.position.y += projectile.velocity.y * deltaTime

      // Apply gravity
      projectile.velocity.y += C.GRAVITY * deltaTime

      // Handle cluster bomb splitting
      if (projectile.kind === ProjectileKind.Cluster && projectile.splitTime !== undefined) {
        projectile.splitTime -= deltaTime
        if (projectile.splitTime <= 0) {
          // Create cluster shards
          for (let i = 0; i < 5; i++) {
            const angle = (i / 5) * Math.PI * 2
            const speed = 50 + Math.random() * 30
            const shardVelocity = {
              x: Math.cos(angle) * speed + projectile.velocity.x * 0.3,
              y: Math.sin(angle) * speed + projectile.velocity.y * 0.3
            }

            newProjectiles.push(this.createProjectile(
              ProjectileKind.ClusterShard,
              { ...projectile.position },
              shardVelocity,
              projectile.ownerId
            ))
          }

          projectile.isActive = false
          continue
        }
      }

      // Check terrain collision
      const terrainHeight = this.getTerrainHeight(gameState.terrain, projectile.position.x)
      if (projectile.position.y >= terrainHeight) {
        this.handleProjectileImpact(gameState, projectile)
        projectile.isActive = false
        continue
      }

      // Check tank collision
      for (const tank of gameState.tanks.values()) {
        if (tank.id === projectile.ownerId || !tank.isAlive) continue

        if (this.checkTankCollision(tank, projectile)) {
          this.handleTankHit(tank, projectile, gameState)
          projectile.isActive = false
          break
        }
      }

      // Remove projectiles that go off screen
      if (projectile.position.x < -50 || projectile.position.x > C.LOGICAL_WIDTH + 50 ||
          projectile.position.y > C.LOGICAL_HEIGHT + 50) {
        projectile.isActive = false
      }
    }

    // Add new projectiles and filter out inactive ones
    gameState.projectiles = [...gameState.projectiles.filter(p => p.isActive), ...newProjectiles]
  }

  static getTerrainHeight(terrain: number[], x: number): number {
    const index = Math.floor(x)
    if (index < 0 || index >= terrain.length) {
      return C.LOGICAL_HEIGHT
    }
    return terrain[index]
  }

  static checkTankCollision(tank: Tank, projectile: Projectile): boolean {
    const dx = tank.position.x - projectile.position.x
    const dy = tank.position.y - projectile.position.y
    const distance = Math.sqrt(dx * dx + dy * dy)

    return distance < (C.TANK_COLLISION_WIDTH / 2 + projectile.radius)
  }

  static handleProjectileImpact(gameState: GameState, projectile: Projectile): void {
    const explosion: Explosion = {
      id: Math.random().toString(36),
      position: { ...projectile.position },
      radius: projectile.radius * 8, // Explosion radius is larger than projectile
      duration: C.EXPLOSION_DURATION,
      elapsed: 0,
      kind: 'normal'
    }

    gameState.explosions.push(explosion)

    // Terrain modification
    this.modifyTerrain(gameState.terrain, projectile)

    // Create napalm patch for napalm projectiles
    if (projectile.kind === ProjectileKind.Napalm) {
      const napalmPatch: NapalmPatch = {
        id: Math.random().toString(36),
        position: { ...projectile.position },
        radius: projectile.radius * 6,
        duration: C.NAPALM_BURN_DURATION,
        elapsed: 0
      }
      gameState.napalmPatches.push(napalmPatch)
    }

    // Damage nearby tanks
    for (const tank of gameState.tanks.values()) {
      if (!tank.isAlive) continue

      const distance = this.getDistance(tank.position, projectile.position)
      if (distance < explosion.radius) {
        const damageRatio = 1 - (distance / explosion.radius)
        const damage = Math.floor(projectile.damage * damageRatio)

        if (!tank.shieldActive && damage > 0) {
          tank.hp = Math.max(0, tank.hp - damage)
          if (tank.hp <= 0) {
            tank.isAlive = false
            this.createTankExplosion(gameState, tank)
          }
        }
      }
    }
  }

  static handleTankHit(tank: Tank, projectile: Projectile, gameState: GameState): void {
    if (!tank.shieldActive) {
      tank.hp = Math.max(0, tank.hp - projectile.damage)
      if (tank.hp <= 0) {
        tank.isAlive = false
        this.createTankExplosion(gameState, tank)
      }
    }
  }

  static createTankExplosion(gameState: GameState, tank: Tank): void {
    const explosion: Explosion = {
      id: Math.random().toString(36),
      position: { ...tank.position },
      radius: 40,
      duration: C.TANK_EXPLOSION_DURATION,
      elapsed: 0,
      kind: 'tank'
    }
    gameState.explosions.push(explosion)
  }

  static modifyTerrain(terrain: number[], projectile: Projectile): void {
    const centerX = Math.floor(projectile.position.x)
    const centerY = projectile.position.y
    const radius = projectile.radius * 4 // Terrain modification radius

    for (let x = centerX - radius; x <= centerX + radius; x++) {
      if (x < 0 || x >= terrain.length) continue

      const distance = Math.abs(x - centerX)
      if (distance <= radius) {
        const falloff = 1 - (distance / radius)

        if (projectile.kind === ProjectileKind.Dirtgun) {
          // Dirtgun builds terrain
          terrain[x] = Math.min(terrain[x] - falloff * 20, centerY)
        } else {
          // Other projectiles destroy terrain
          terrain[x] = Math.max(terrain[x] + falloff * 20, C.TERRAIN_BASELINE + 20)
        }
      }
    }
  }

  static updateExplosions(gameState: GameState, deltaTime: number): void {
    gameState.explosions = gameState.explosions.filter(explosion => {
      explosion.elapsed += deltaTime
      return explosion.elapsed < explosion.duration
    })
  }

  static updateNapalmPatches(gameState: GameState, deltaTime: number): void {
    gameState.napalmPatches = gameState.napalmPatches.filter(patch => {
      patch.elapsed += deltaTime

      // Damage tanks in napalm
      for (const tank of gameState.tanks.values()) {
        if (!tank.isAlive || tank.shieldActive) continue

        const distance = this.getDistance(tank.position, patch.position)
        if (distance < patch.radius) {
          tank.hp = Math.max(0, tank.hp - C.NAPALM_EROSION_RATE * deltaTime)
          if (tank.hp <= 0) {
            tank.isAlive = false
            this.createTankExplosion(gameState, tank)
          }
        }
      }

      return patch.elapsed < patch.duration
    })
  }

  static updateTanks(gameState: GameState, deltaTime: number): void {
    for (const tank of gameState.tanks.values()) {
      if (tank.shieldCooldown > 0) {
        tank.shieldCooldown -= deltaTime
        if (tank.shieldCooldown <= 0) {
          tank.shieldActive = false
        }
      }

      // Update tank position based on terrain
      if (tank.isAlive) {
        tank.position.y = this.getTerrainHeight(gameState.terrain, tank.position.x) - C.TANK_COLLISION_HEIGHT
      }
    }
  }

  static getDistance(pos1: Position, pos2: Position): number {
    const dx = pos1.x - pos2.x
    const dy = pos1.y - pos2.y
    return Math.sqrt(dx * dx + dy * dy)
  }

  static nextAmmoType(current: ProjectileKind): ProjectileKind {
    switch (current) {
      case ProjectileKind.Mortar: return ProjectileKind.Cluster
      case ProjectileKind.Cluster: return ProjectileKind.Napalm
      case ProjectileKind.Napalm: return ProjectileKind.Dirtgun
      case ProjectileKind.Dirtgun: return ProjectileKind.Mortar
      default: return ProjectileKind.Mortar
    }
  }

  static fireTank(tank: Tank): Projectile | null {
    if (!tank.isAlive) return null

    const angle = tank.turretAngle * C.DEG2RAD
    const velocity = {
      x: Math.cos(angle) * tank.power,
      y: Math.sin(angle) * tank.power
    }

    const muzzleDistance = 15 // Distance from tank center to muzzle
    const startPosition = {
      x: tank.position.x + Math.cos(angle) * muzzleDistance,
      y: tank.position.y + Math.sin(angle) * muzzleDistance
    }

    return this.createProjectile(tank.currentAmmo, startPosition, velocity, tank.id)
  }
}