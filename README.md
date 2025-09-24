# Tank Duel Web Game

A real-time multiplayer web version of Tank Duel built for Google Cloud Run.

## 🎮 Game Features

- **Real-time Multiplayer**: Two players battle in real-time using WebSockets
- **HTML5 Canvas Graphics**: Pixel-perfect 2D graphics with smooth animations
- **Turn-based Combat**: Strategic gameplay with 30-second turns
- **Multiple Weapons**: Mortar, Cluster bombs, Napalm, and Dirt Gun
- **Destructible Terrain**: Dynamic battlefields that change with each explosion
- **Shield System**: Activate protective shields for defense

## 🚀 Quick Start

### Local Development

1. **Install dependencies**:
   ```bash
   npm install
   ```

2. **Start development server**:
   ```bash
   npm run dev
   ```

3. **Open in browser**:
   - Client: http://localhost:5173
   - Server: http://localhost:3000

### Production Build

```bash
npm run build
npm start
```

## ☁️ Deploy to Google Cloud Run

### Prerequisites

- Google Cloud SDK installed and configured
- A Google Cloud Project with billing enabled

### Deployment Steps

1. **Update the project ID** in `deploy.sh`:
   ```bash
   PROJECT_ID="your-project-id"
   ```

2. **Run the deployment script**:
   ```bash
   ./deploy.sh
   ```

3. **Or deploy manually**:
   ```bash
   # Enable APIs
   gcloud services enable cloudbuild.googleapis.com run.googleapis.com

   # Build and deploy
   gcloud builds submit --config cloudbuild.yaml

   # Get service URL
   gcloud run services describe tank-duel --region=us-central1 --format="value(status.url)"
   ```

### Manual Docker Deployment

```bash
# Build image
docker build -t tank-duel .

# Run locally
docker run -p 3000:3000 tank-duel

# Push to Google Container Registry
docker tag tank-duel gcr.io/YOUR_PROJECT_ID/tank-duel
docker push gcr.io/YOUR_PROJECT_ID/tank-duel

# Deploy to Cloud Run
gcloud run deploy tank-duel \
  --image gcr.io/YOUR_PROJECT_ID/tank-duel \
  --region us-central1 \
  --platform managed \
  --allow-unauthenticated
```

## 🎯 How to Play

### Controls

- **Q/A**: Aim turret up/down
- **W/S**: Adjust power
- **SPACE**: Fire weapon
- **E**: Change ammo type
- **R**: Activate shield

### Game Flow

1. Enter your name and click "Join Game"
2. Wait for another player to join
3. Take turns firing at your opponent
4. Use terrain and strategy to your advantage
5. Last tank standing wins!

### Weapons

- **Mortar**: Standard explosive round
- **Cluster**: Splits into multiple smaller explosives
- **Napalm**: Creates burning patches that damage over time
- **Dirt Gun**: Builds terrain instead of destroying it

## 🏗️ Architecture

### Tech Stack

- **Frontend**: TypeScript, HTML5 Canvas, Vite
- **Backend**: Node.js, Express, Socket.IO
- **Deployment**: Docker, Google Cloud Run
- **Real-time Communication**: WebSockets via Socket.IO

### Project Structure

```
src/
├── client/           # Frontend code
│   ├── main.ts       # Main client entry point
│   ├── GameRenderer.ts   # Canvas rendering
│   ├── InputHandler.ts   # Keyboard controls
│   └── index.html    # Game UI
├── server/           # Backend code
│   ├── server.ts     # Express + Socket.IO server
│   └── game/
│       └── GameRoom.ts   # Game state management
└── shared/           # Shared code
    ├── types.ts      # TypeScript interfaces
    └── GameLogic.ts  # Core game mechanics
```

### Game Logic

The game uses authoritative server architecture:

- **Server**: Maintains canonical game state, processes all game logic
- **Client**: Renders game state, handles input, sends actions to server
- **Synchronization**: Real-time updates via WebSockets

## 🔧 Configuration

### Environment Variables

- `PORT`: Server port (default: 3000)
- `NODE_ENV`: Environment mode (development/production)

### Cloud Run Settings

- **Memory**: 512Mi
- **CPU**: 1 vCPU
- **Concurrency**: 80 requests per instance
- **Max Instances**: 10

## 🐛 Troubleshooting

### Common Issues

1. **Build fails**: Ensure all dependencies are installed
2. **WebSocket connection fails**: Check CORS settings and port configuration
3. **Game doesn't load**: Verify client build completed successfully
4. **Cloud Run timeout**: Increase timeout in `cloudbuild.yaml`

### Development Tips

- Use browser dev tools to debug client-side issues
- Check server logs with `gcloud run logs tail tank-duel`
- Monitor Cloud Run metrics in Google Cloud Console

## 📝 API Reference

### Socket.IO Events

#### Client → Server
- `join_game(playerName)`: Join a game room
- `player_action(action)`: Send player input

#### Server → Client
- `joined_room(data)`: Confirmation of joining room
- `game_started(data)`: Game has begun
- `game_state(data)`: Updated game state
- `turn_changed(data)`: Turn has switched
- `game_ended(data)`: Game finished

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test locally
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

**Ready to battle!** 🚀🎯