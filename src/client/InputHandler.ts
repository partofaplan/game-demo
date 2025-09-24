import { GAME_CONSTANTS as C } from '@shared/types'

export class InputHandler {
  private keys = new Set<string>()
  private actionCallback: ((action: any) => void) | null = null

  constructor() {
    this.setupEventListeners()
  }

  onAction(callback: (action: any) => void): void {
    this.actionCallback = callback
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
        this.actionCallback({
          type: 'aim',
          data: { angle: this.getCurrentAngle() - C.TURRET_ROT_SPEED * 0.1 }
        })
        break

      case 'KeyA':
        // Aim down
        this.actionCallback({
          type: 'aim',
          data: { angle: this.getCurrentAngle() + C.TURRET_ROT_SPEED * 0.1 }
        })
        break

      case 'KeyW':
        // Increase power
        this.actionCallback({
          type: 'power',
          data: { power: this.getCurrentPower() + C.POWER_ADJUST_RATE * 0.1 }
        })
        break

      case 'KeyS':
        // Decrease power
        this.actionCallback({
          type: 'power',
          data: { power: this.getCurrentPower() - C.POWER_ADJUST_RATE * 0.1 }
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

  private getCurrentAngle(): number {
    // This should be tracked from game state in a real implementation
    return 0
  }

  private getCurrentPower(): number {
    // This should be tracked from game state in a real implementation
    return C.DEFAULT_LAUNCH_SPEED
  }
}