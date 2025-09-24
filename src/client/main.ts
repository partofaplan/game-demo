import { io, Socket } from 'socket.io-client'
import { GameRenderer } from './GameRenderer'
import { InputHandler } from './InputHandler'
import { ProjectileKind } from '@shared/types'

class GameClient {
  private socket: Socket
  private renderer: GameRenderer
  private inputHandler: InputHandler
  private gameState: any = null
  private playerId: string | null = null
  private roomId: string | null = null

  constructor() {
    this.socket = io()
    this.renderer = new GameRenderer('gameCanvas')
    this.inputHandler = new InputHandler()

    this.setupSocketEvents()
    this.setupUI()
  }

  private setupSocketEvents(): void {
    this.socket.on('joined_room', (data) => {
      this.playerId = data.playerId
      this.roomId = data.roomId
      document.getElementById('roomId')!.textContent = `Room: ${data.roomId}`
      this.showGameArea()
    })

    this.socket.on('room_full', () => {
      alert('Room is full! Please try again.')
    })

    this.socket.on('game_started', (data) => {
      this.gameState = data.gameState
      this.updateUI()
      this.updateStatus('Game Started!')
    })

    this.socket.on('game_state', (data) => {
      this.gameState = data
      this.renderer.render(this.gameState)
      this.updateUI()
    })

    this.socket.on('turn_changed', (data) => {
      this.updateTurnUI(data.currentTurn)
    })

    this.socket.on('game_ended', (data) => {
      this.gameState = data.gameState
      const isWinner = data.winner === this.playerId
      const winnerName = this.getPlayerName(data.winner)
      this.updateStatus(isWinner ? 'You Win!' : `${winnerName} Wins!`)
    })

    this.socket.on('player_joined', (data) => {
      this.gameState = data.gameState
      this.updatePlayerNames()
    })

    this.socket.on('player_left', () => {
      this.updateStatus('Player left the game')
    })
  }

  private setupUI(): void {
    const joinButton = document.getElementById('joinGame')!
    const playerNameInput = document.getElementById('playerName')! as HTMLInputElement
    const changeAmmoButton = document.getElementById('changeAmmo')!
    const useShieldButton = document.getElementById('useShield')!

    joinButton.addEventListener('click', () => {
      const name = playerNameInput.value.trim() || 'Anonymous'
      this.socket.emit('join_game', name)
    })

    playerNameInput.addEventListener('keypress', (e) => {
      if (e.key === 'Enter') {
        joinButton.click()
      }
    })

    changeAmmoButton.addEventListener('click', () => {
      if (this.isMyTurn()) {
        this.socket.emit('player_action', { type: 'change_ammo' })
      }
    })

    useShieldButton.addEventListener('click', () => {
      if (this.isMyTurn()) {
        this.socket.emit('player_action', { type: 'shield' })
      }
    })

    // Keyboard controls
    this.inputHandler.onAction((action) => {
      if (this.isMyTurn() && this.gameState?.gamePhase === 'playing') {
        this.socket.emit('player_action', action)
      }
    })
  }

  private showGameArea(): void {
    document.getElementById('joinForm')!.classList.add('hidden')
    document.getElementById('gameArea')!.classList.remove('hidden')
  }

  private updateUI(): void {
    if (!this.gameState) return

    this.updatePlayerNames()
    this.updateHealthBars()
    this.updateTurnUI(this.gameState.currentTurn)
    this.updateAmmoDisplay()
    this.updateInputHandler()
  }

  private updatePlayerNames(): void {
    if (!this.gameState?.tanks) return

    const tankIds = Object.keys(this.gameState.tanks)
    const player1Name = document.getElementById('player1Name')!
    const player2Name = document.getElementById('player2Name')!

    if (tankIds.length >= 1) {
      player1Name.textContent = this.getPlayerName(tankIds[0])
    }
    if (tankIds.length >= 2) {
      player2Name.textContent = this.getPlayerName(tankIds[1])
    }
  }

  private updateHealthBars(): void {
    if (!this.gameState?.tanks) return

    const tankIds = Object.keys(this.gameState.tanks)
    const tanks = this.gameState.tanks

    if (tankIds.length >= 1) {
      const tank1 = tanks[tankIds[0]]
      const health1 = document.getElementById('player1Health')! as HTMLElement
      const healthPercent = (tank1.hp / tank1.maxHp) * 100
      health1.style.width = `${healthPercent}%`
      health1.classList.toggle('critical', healthPercent < 30)
    }

    if (tankIds.length >= 2) {
      const tank2 = tanks[tankIds[1]]
      const health2 = document.getElementById('player2Health')! as HTMLElement
      const healthPercent = (tank2.hp / tank2.maxHp) * 100
      health2.style.width = `${healthPercent}%`
      health2.classList.toggle('critical', healthPercent < 30)
    }
  }

  private updateTurnUI(currentTurn: string): void {
    const turnElement = document.getElementById('currentTurn')!
    const timerElement = document.getElementById('turnTimer')!

    if (currentTurn === this.playerId) {
      turnElement.textContent = 'Your Turn'
      turnElement.style.color = '#4CAF50'
    } else {
      const playerName = this.getPlayerName(currentTurn)
      turnElement.textContent = `${playerName}'s Turn`
      turnElement.style.color = '#fff'
    }

    if (this.gameState?.turnTimeLeft) {
      timerElement.textContent = `${Math.ceil(this.gameState.turnTimeLeft)}s`
    }
  }

  private updateAmmoDisplay(): void {
    if (!this.gameState?.tanks || !this.playerId) return

    const tank = this.gameState.tanks[this.playerId]
    if (tank) {
      const ammoElement = document.getElementById('currentAmmo')!
      const powerElement = document.getElementById('currentPower')!
      const angleElement = document.getElementById('currentAngle')!

      ammoElement.textContent = this.getAmmoName(tank.currentAmmo)
      powerElement.textContent = `Power: ${Math.round(tank.power)}`
      angleElement.textContent = `Angle: ${Math.round(tank.turretAngle)}°`
    }
  }

  private updateStatus(message: string): void {
    document.getElementById('status')!.textContent = message
  }

  private isMyTurn(): boolean {
    return this.gameState?.currentTurn === this.playerId
  }

  private getPlayerName(playerId: string): string {
    // In a real implementation, you'd store player names
    return playerId === this.playerId ? 'You' : `Player ${playerId.substring(0, 4)}`
  }

  private updateInputHandler(): void {
    if (!this.gameState?.tanks || !this.playerId) return

    const tank = this.gameState.tanks[this.playerId]
    if (tank) {
      this.inputHandler.updateGameState(tank.turretAngle, tank.power)
    }
  }

  private getAmmoName(ammo: ProjectileKind): string {
    switch (ammo) {
      case ProjectileKind.Mortar: return 'Mortar'
      case ProjectileKind.Cluster: return 'Cluster'
      case ProjectileKind.Napalm: return 'Napalm'
      case ProjectileKind.Dirtgun: return 'Dirt Gun'
      default: return 'Unknown'
    }
  }
}

// Start the game when page loads
new GameClient()