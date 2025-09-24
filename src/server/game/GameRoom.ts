import { Server as SocketServer } from 'socket.io'
import { Player, Tank, GameState, PlayerAction, GAME_CONSTANTS as C, ProjectileKind } from '../../shared/types'
import { GameLogic } from '../../shared/GameLogic'

export class GameRoom {
  private gameState: GameState
  private players: Map<string, Player> = new Map()
  private lastUpdate = Date.now()
  private gameLoop: NodeJS.Timeout | null = null

  constructor(
    private roomId: string,
    private io: SocketServer
  ) {
    this.gameState = {
      tanks: new Map(),
      projectiles: [],
      explosions: [],
      napalmPatches: [],
      terrain: GameLogic.generateTerrain(),
      currentTurn: null,
      turnTimeLeft: C.TURN_TIME_LIMIT,
      gamePhase: 'waiting',
      winner: null
    }
  }

  addPlayer(player: Player): boolean {
    if (this.players.size >= 2) {
      return false
    }

    this.players.set(player.id, player)

    // Create tank for player
    const tankX = this.players.size === 1 ? 100 : C.LOGICAL_WIDTH - 100
    const tank = GameLogic.createTank(player.id, tankX)

    // Position tank on terrain
    tank.position.y = GameLogic.getTerrainHeight(this.gameState.terrain, tankX) - C.TANK_COLLISION_HEIGHT

    this.gameState.tanks.set(player.id, tank)

    this.broadcastToRoom('player_joined', {
      player,
      gameState: this.serializeGameState()
    })

    return true
  }

  removePlayer(playerId: string): void {
    this.players.delete(playerId)
    this.gameState.tanks.delete(playerId)

    if (this.gameState.gamePhase === 'playing' && this.gameState.tanks.size === 1) {
      this.endGame()
    }

    this.broadcastToRoom('player_left', { playerId })
  }

  getPlayerCount(): number {
    return this.players.size
  }

  startGame(): void {
    if (this.players.size !== 2) return

    this.gameState.gamePhase = 'playing'
    this.gameState.currentTurn = Array.from(this.players.keys())[0]
    this.gameState.turnTimeLeft = C.TURN_TIME_LIMIT

    this.broadcastToRoom('game_started', {
      gameState: this.serializeGameState()
    })

    this.startGameLoop()
  }

  private startGameLoop(): void {
    this.gameLoop = setInterval(() => {
      this.update()
    }, 1000 / 60) // 60 FPS
  }

  private update(): void {
    if (this.gameState.gamePhase !== 'playing') return

    const now = Date.now()
    const deltaTime = (now - this.lastUpdate) / 1000
    this.lastUpdate = now

    // Update game objects
    GameLogic.updateProjectiles(this.gameState, deltaTime)
    GameLogic.updateExplosions(this.gameState, deltaTime)
    GameLogic.updateNapalmPatches(this.gameState, deltaTime)
    GameLogic.updateTanks(this.gameState, deltaTime)

    // Update turn timer
    this.gameState.turnTimeLeft -= deltaTime
    if (this.gameState.turnTimeLeft <= 0) {
      this.nextTurn()
    }

    // Check win condition
    const aliveTanks = Array.from(this.gameState.tanks.values()).filter(tank => tank.isAlive)
    if (aliveTanks.length <= 1) {
      this.endGame()
      return
    }

    // Broadcast updated game state
    this.broadcastToRoom('game_state', this.serializeGameState())
  }

  handlePlayerAction(playerId: string, action: PlayerAction): void {
    if (this.gameState.gamePhase !== 'playing') return
    if (this.gameState.currentTurn !== playerId) return

    const tank = this.gameState.tanks.get(playerId)
    if (!tank || !tank.isAlive) return

    switch (action.type) {
      case 'aim':
        tank.turretAngle = Math.max(-C.MAX_TURRET_SWING,
          Math.min(C.MAX_TURRET_SWING, action.data.angle))
        break

      case 'power':
        tank.power = Math.max(C.MIN_LAUNCH_SPEED,
          Math.min(C.MAX_LAUNCH_SPEED, action.data.power))
        break

      case 'fire':
        const projectile = GameLogic.fireTank(tank)
        if (projectile) {
          this.gameState.projectiles.push(projectile)
          this.nextTurn()
        }
        break

      case 'change_ammo':
        tank.currentAmmo = GameLogic.nextAmmoType(tank.currentAmmo)
        break

      case 'shield':
        if (tank.shieldCooldown <= 0) {
          tank.shieldActive = true
          tank.shieldCooldown = 5.0 // 5 second shield duration
        }
        break
    }
  }

  private nextTurn(): void {
    const playerIds = Array.from(this.players.keys())
    const currentIndex = playerIds.indexOf(this.gameState.currentTurn!)
    const nextIndex = (currentIndex + 1) % playerIds.length

    this.gameState.currentTurn = playerIds[nextIndex]
    this.gameState.turnTimeLeft = C.TURN_TIME_LIMIT

    this.broadcastToRoom('turn_changed', {
      currentTurn: this.gameState.currentTurn
    })
  }

  private endGame(): void {
    if (this.gameLoop) {
      clearInterval(this.gameLoop)
      this.gameLoop = null
    }

    const aliveTanks = Array.from(this.gameState.tanks.values()).filter(tank => tank.isAlive)
    this.gameState.winner = aliveTanks.length === 1 ? aliveTanks[0].id : null
    this.gameState.gamePhase = 'ended'

    this.broadcastToRoom('game_ended', {
      winner: this.gameState.winner,
      gameState: this.serializeGameState()
    })
  }

  private broadcastToRoom(event: string, data: any): void {
    this.io.to(this.roomId).emit(event, data)
  }

  private serializeGameState() {
    return {
      ...this.gameState,
      tanks: Object.fromEntries(this.gameState.tanks)
    }
  }
}