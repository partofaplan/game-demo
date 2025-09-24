import express from 'express'
import { createServer } from 'http'
import { Server as SocketServer } from 'socket.io'
import cors from 'cors'
import path from 'path'
import { GameRoom } from './game/GameRoom'
import { Player } from '../shared/types'

const app = express()
const server = createServer(app)
const io = new SocketServer(server, {
  cors: {
    origin: process.env.NODE_ENV === 'production' ? false : ['http://localhost:5173'],
    methods: ['GET', 'POST']
  }
})

app.use(cors())
app.use(express.static(path.join(__dirname, '../public')))

// Game state
const rooms = new Map<string, GameRoom>()
const playerRooms = new Map<string, string>()

// Generate room ID
function generateRoomId(): string {
  return Math.random().toString(36).substring(2, 8).toUpperCase()
}

// Find available room or create new one
function findOrCreateRoom(): string {
  // Find room with only one player
  for (const [roomId, room] of rooms) {
    if (room.getPlayerCount() === 1) {
      return roomId
    }
  }

  // Create new room
  const roomId = generateRoomId()
  rooms.set(roomId, new GameRoom(roomId, io))
  return roomId
}

io.on('connection', (socket) => {
  console.log(`Player connected: ${socket.id}`)

  socket.on('join_game', (playerName: string) => {
    const roomId = findOrCreateRoom()
    const room = rooms.get(roomId)!

    const player: Player = {
      id: socket.id,
      name: playerName || `Player ${socket.id.substring(0, 4)}`,
      roomId
    }

    socket.join(roomId)
    playerRooms.set(socket.id, roomId)

    const success = room.addPlayer(player)
    if (success) {
      socket.emit('joined_room', { roomId, playerId: socket.id })

      // If room is full, start the game
      if (room.getPlayerCount() === 2) {
        room.startGame()
      }
    } else {
      socket.emit('room_full')
    }
  })

  socket.on('player_action', (action) => {
    const roomId = playerRooms.get(socket.id)
    if (roomId) {
      const room = rooms.get(roomId)
      room?.handlePlayerAction(socket.id, action)
    }
  })

  socket.on('disconnect', () => {
    console.log(`Player disconnected: ${socket.id}`)

    const roomId = playerRooms.get(socket.id)
    if (roomId) {
      const room = rooms.get(roomId)
      room?.removePlayer(socket.id)

      // Clean up empty rooms
      if (room?.getPlayerCount() === 0) {
        rooms.delete(roomId)
      }

      playerRooms.delete(socket.id)
    }
  })
})

const PORT = process.env.PORT || 3000
server.listen(PORT, () => {
  console.log(`Server running on port ${PORT}`)
})