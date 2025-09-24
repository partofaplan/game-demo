import { GAME_CONSTANTS as C } from '@shared/types'

export class InputHandler {
  private keys = new Set<string>()
  private actionCallback: ((action: any) => void) | null = null
  private currentAngle = 0
  private currentPower = C.DEFAULT_LAUNCH_SPEED

  constructor() {
    this.setupEventListeners()
  }

  onAction(callback: (action: any) => void): void {
    this.actionCallback = callback
  }

  updateGameState(angle: number, power: number): void {
    this.currentAngle = angle
    this.currentPower = power
  }

  private setupEventListeners(): void {
    document.addEventListener('keydown', (e) => {
      if (this.keys.has(e.code)) return
      this.keys.add(e.code)
      this.handleKeyDown(e.code)
    })

    document.addEventListener('keyup', (e) => {
      this.keys.delete(e.code)
    })

    // Prevent default browser shortcuts
    document.addEventListener('keydown', (e) => {
      if (['Space', 'KeyW', 'KeyS', 'KeyA', 'KeyQ', 'KeyE', 'KeyR'].includes(e.code)) {
        e.preventDefault()
      }
    })
  }

  private handleKeyDown(code: string): void {
    if (!this.actionCallback) return

    switch (code) {
      case 'KeyQ':
        // Aim up
        this.currentAngle = Math.max(-C.MAX_TURRET_SWING, this.currentAngle - 5)
        this.actionCallback({
          type: 'aim',
          data: { angle: this.currentAngle }
        })
        break

      case 'KeyA':
        // Aim down
        this.currentAngle = Math.min(C.MAX_TURRET_SWING, this.currentAngle + 5)
        this.actionCallback({
          type: 'aim',
          data: { angle: this.currentAngle }
        })
        break

      case 'KeyW':
        // Increase power
        this.currentPower = Math.min(C.MAX_LAUNCH_SPEED, this.currentPower + 10)
        this.actionCallback({
          type: 'power',
          data: { power: this.currentPower }
        })
        break

      case 'KeyS':
        // Decrease power
        this.currentPower = Math.max(C.MIN_LAUNCH_SPEED, this.currentPower - 10)
        this.actionCallback({
          type: 'power',
          data: { power: this.currentPower }
        })
        break

      case 'Space':
        // Fire
        this.actionCallback({ type: 'fire' })
        break

      case 'KeyE':
        // Change ammo
        this.actionCallback({ type: 'change_ammo' })
        break

      case 'KeyR':
        // Shield
        this.actionCallback({ type: 'shield' })
        break
    }
  }

}