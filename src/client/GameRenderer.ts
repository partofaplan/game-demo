import { GAME_CONSTANTS as C, ProjectileKind } from '@shared/types'
import greenTankSprite from './assets/green_tank.png'
import redTankSprite from './assets/red_tank.png'

interface TankSprites {
  image: HTMLImageElement
}

export class GameRenderer {
  private canvas: HTMLCanvasElement
  private ctx: CanvasRenderingContext2D
  private sprites: {
    red: TankSprites | null
    green: TankSprites | null
  } = { red: null, green: null }
  private spritesLoaded = false

  constructor(canvasId: string) {
    this.canvas = document.getElementById(canvasId) as HTMLCanvasElement
    this.ctx = this.canvas.getContext('2d')!

    // Set up pixel art rendering
    this.ctx.imageSmoothingEnabled = false

    // Load sprites
    this.loadSprites()
  }

  private async loadSprites(): Promise<void> {
    try {
      const [greenImg, redImg] = await Promise.all([
        this.loadImage(greenTankSprite),
        this.loadImage(redTankSprite)
      ])

      this.sprites.green = { image: greenImg }
      this.sprites.red = { image: redImg }

      this.spritesLoaded = true
      console.log('Tank sprites loaded successfully')
    } catch (error) {
      console.warn('Failed to load tank sprites, falling back to geometric shapes:', error)
      this.spritesLoaded = false
    }
  }

  private loadImage(src: string): Promise<HTMLImageElement> {
    return new Promise((resolve, reject) => {
      const img = new Image()
      img.onload = () => resolve(img)
      img.onerror = reject
      img.src = src
    })
  }

  render(gameState: any): void {
    if (!gameState) return

    this.clear()
    this.drawTerrain(gameState.terrain)
    this.drawTanks(gameState.tanks)
    this.drawProjectiles(gameState.projectiles)
    this.drawExplosions(gameState.explosions)
    this.drawNapalmPatches(gameState.napalmPatches)
  }

  private clear(): void {
    // Sky gradient
    const gradient = this.ctx.createLinearGradient(0, 0, 0, C.LOGICAL_HEIGHT)
    gradient.addColorStop(0, '#87CEEB')
    gradient.addColorStop(1, '#E0F6FF')

    this.ctx.fillStyle = gradient
    this.ctx.fillRect(0, 0, C.LOGICAL_WIDTH, C.LOGICAL_HEIGHT)
  }

  private drawTerrain(terrain: number[]): void {
    if (!terrain) return

    this.ctx.fillStyle = '#8B4513'
    this.ctx.beginPath()
    this.ctx.moveTo(0, C.LOGICAL_HEIGHT)

    for (let x = 0; x < terrain.length; x++) {
      this.ctx.lineTo(x, terrain[x])
    }

    this.ctx.lineTo(C.LOGICAL_WIDTH, C.LOGICAL_HEIGHT)
    this.ctx.closePath()
    this.ctx.fill()

    // Draw grass on top
    this.ctx.strokeStyle = '#228B22'
    this.ctx.lineWidth = 2
    this.ctx.beginPath()
    for (let x = 0; x < terrain.length; x++) {
      this.ctx.lineTo(x, terrain[x])
    }
    this.ctx.stroke()
  }

  private drawTanks(tanks: any): void {
    if (!tanks) return

    Object.values(tanks).forEach((tank: any) => {
      if (!tank.isAlive) return

      this.ctx.save()
      this.ctx.translate(tank.position.x, tank.position.y)

      if (this.spritesLoaded) {
        this.drawTankWithSprites(tank)
      } else {
        this.drawTankWithShapes(tank)
      }

      // Draw HP bar
      this.drawHealthBar(tank)

      this.ctx.restore()
    })
  }

  private drawTankWithSprites(tank: any): void {
    // Determine sprite based on tank position: left tank gets green, right tank gets red
    const isLeftTank = tank.position.x < C.LOGICAL_WIDTH / 2
    const spriteColor = isLeftTank ? 'green' : 'red'
    const tankSprites = this.sprites[spriteColor]

    if (!tankSprites) return

    // Apply shield effect if active
    if (tank.shieldActive) {
      this.ctx.shadowColor = '#00FFFF'
      this.ctx.shadowBlur = 8
    }

    // Draw tank sprite
    const spriteWidth = 60 // Adjust size as needed
    const spriteHeight = 40 // Adjust size as needed

    this.ctx.drawImage(
      tankSprites.image,
      -spriteWidth / 2,
      -spriteHeight + 5, // Adjust positioning to sit on ground
      spriteWidth,
      spriteHeight
    )

    // Reset shadow
    if (tank.shieldActive) {
      this.ctx.shadowBlur = 0
    }
  }

  private drawTankWithShapes(tank: any): void {
    // Fallback to original rectangle rendering
    this.ctx.fillStyle = tank.shieldActive ? '#00FFFF' : '#4A4A4A'
    this.ctx.fillRect(-C.TANK_COLLISION_WIDTH / 2, 0, C.TANK_COLLISION_WIDTH, C.TANK_COLLISION_HEIGHT)

    // Draw turret
    this.ctx.save()
    this.ctx.translate(0, -4)
    this.ctx.rotate(tank.turretAngle * C.DEG2RAD)

    this.ctx.fillStyle = tank.shieldActive ? '#00DDDD' : '#2A2A2A'
    this.ctx.fillRect(0, -2, 30, 4)

    this.ctx.restore()
  }

  private drawHealthBar(tank: any): void {
    const barWidth = C.TANK_COLLISION_WIDTH
    const barHeight = 4
    const hpPercent = tank.hp / tank.maxHp

    this.ctx.fillStyle = '#FF0000'
    this.ctx.fillRect(-barWidth / 2, -16, barWidth, barHeight)

    this.ctx.fillStyle = '#00FF00'
    this.ctx.fillRect(-barWidth / 2, -16, barWidth * hpPercent, barHeight)
  }

  private drawProjectiles(projectiles: any[]): void {
    if (!projectiles) return

    projectiles.forEach((projectile) => {
      if (!projectile.isActive) return

      this.ctx.save()
      this.ctx.translate(projectile.position.x, projectile.position.y)

      const color = this.getProjectileColor(projectile.kind)
      this.ctx.fillStyle = color
      this.ctx.beginPath()
      this.ctx.arc(0, 0, projectile.radius, 0, Math.PI * 2)
      this.ctx.fill()

      // Draw projectile trail
      this.ctx.strokeStyle = color
      this.ctx.lineWidth = 1
      this.ctx.beginPath()
      this.ctx.moveTo(0, 0)
      this.ctx.lineTo(-projectile.velocity.x * 0.05, -projectile.velocity.y * 0.05)
      this.ctx.stroke()

      this.ctx.restore()
    })
  }

  private drawExplosions(explosions: any[]): void {
    if (!explosions) return

    explosions.forEach((explosion) => {
      const progress = explosion.elapsed / explosion.duration
      const alpha = 1 - progress
      const size = explosion.radius * (0.5 + progress * 0.5)

      this.ctx.save()
      this.ctx.globalAlpha = alpha

      const gradient = this.ctx.createRadialGradient(
        explosion.position.x, explosion.position.y, 0,
        explosion.position.x, explosion.position.y, size
      )

      if (explosion.kind === 'tank') {
        gradient.addColorStop(0, '#FFFF00')
        gradient.addColorStop(0.5, '#FF8000')
        gradient.addColorStop(1, '#FF0000')
      } else {
        gradient.addColorStop(0, '#FFFF80')
        gradient.addColorStop(0.7, '#FF8040')
        gradient.addColorStop(1, '#800000')
      }

      this.ctx.fillStyle = gradient
      this.ctx.beginPath()
      this.ctx.arc(explosion.position.x, explosion.position.y, size, 0, Math.PI * 2)
      this.ctx.fill()

      this.ctx.restore()
    })
  }

  private drawNapalmPatches(napalmPatches: any[]): void {
    if (!napalmPatches) return

    napalmPatches.forEach((patch) => {
      const progress = patch.elapsed / patch.duration
      const alpha = 1 - progress

      this.ctx.save()
      this.ctx.globalAlpha = alpha * 0.7

      const gradient = this.ctx.createRadialGradient(
        patch.position.x, patch.position.y, 0,
        patch.position.x, patch.position.y, patch.radius
      )

      gradient.addColorStop(0, '#FF4500')
      gradient.addColorStop(0.5, '#FF6500')
      gradient.addColorStop(1, '#FF8500')

      this.ctx.fillStyle = gradient
      this.ctx.beginPath()
      this.ctx.arc(patch.position.x, patch.position.y, patch.radius, 0, Math.PI * 2)
      this.ctx.fill()

      this.ctx.restore()
    })
  }

  private getProjectileColor(kind: ProjectileKind): string {
    switch (kind) {
      case ProjectileKind.Mortar: return '#808080'
      case ProjectileKind.Cluster: return '#FFA500'
      case ProjectileKind.ClusterShard: return '#FF8000'
      case ProjectileKind.Napalm: return '#FF4500'
      case ProjectileKind.Dirtgun: return '#8B4513'
      default: return '#808080'
    }
  }
}