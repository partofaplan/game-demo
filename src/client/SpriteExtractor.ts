interface SpriteData {
  canvas: HTMLCanvasElement
  width: number
  height: number
}

export class SpriteExtractor {
  private sourceImage: HTMLImageElement | null = null

  async loadSourceImage(imagePath: string): Promise<void> {
    return new Promise((resolve, reject) => {
      const img = new Image()
      img.onload = () => {
        this.sourceImage = img
        resolve()
      }
      img.onerror = reject
      img.src = imagePath
    })
  }

  extractSprite(x: number, y: number, width: number, height: number): SpriteData {
    if (!this.sourceImage) {
      throw new Error('Source image not loaded')
    }

    const canvas = document.createElement('canvas')
    const ctx = canvas.getContext('2d')!

    canvas.width = width
    canvas.height = height

    // Enable crisp pixel art rendering
    ctx.imageSmoothingEnabled = false

    // Extract the sprite region
    ctx.drawImage(
      this.sourceImage,
      x, y, width, height,  // Source rectangle
      0, 0, width, height   // Destination rectangle
    )

    return {
      canvas,
      width,
      height
    }
  }

  async extractTankSprites(): Promise<{
    redHull: SpriteData
    redTurret: SpriteData
    greenHull: SpriteData
    greenTurret: SpriteData
  }> {
    if (!this.sourceImage) {
      throw new Error('Source image not loaded')
    }

    // Approximate sprite positions based on the image
    // Red tank (left side)
    const redHull = this.extractSprite(32, 432, 100, 64)
    const redTurret = this.extractSprite(132, 352, 96, 32)

    // Green tank (right side)
    const greenHull = this.extractSprite(752, 432, 100, 64)
    const greenTurret = this.extractSprite(710, 352, 96, 32)

    return {
      redHull,
      redTurret,
      greenHull,
      greenTurret
    }
  }

  // Convert canvas to data URL for easy loading
  spriteToDataURL(sprite: SpriteData): string {
    return sprite.canvas.toDataURL('image/png')
  }
}