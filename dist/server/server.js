"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const http_1 = require("http");
const socket_io_1 = require("socket.io");
const cors_1 = __importDefault(require("cors"));
const path_1 = __importDefault(require("path"));
const GameRoom_1 = require("./game/GameRoom");
const app = (0, express_1.default)();
const server = (0, http_1.createServer)(app);
const io = new socket_io_1.Server(server, {
    cors: {
        origin: process.env.NODE_ENV === 'production' ? false : ['http://localhost:5173'],
        methods: ['GET', 'POST']
    }
});
app.use((0, cors_1.default)());
app.use(express_1.default.static(path_1.default.join(__dirname, '../public')));
// Game state
const rooms = new Map();
const playerRooms = new Map();
// Generate room ID
function generateRoomId() {
    return Math.random().toString(36).substring(2, 8).toUpperCase();
}
// Find available room or create new one
function findOrCreateRoom() {
    // Find room with only one player
    for (const [roomId, room] of rooms) {
        if (room.getPlayerCount() === 1) {
            return roomId;
        }
    }
    // Create new room
    const roomId = generateRoomId();
    rooms.set(roomId, new GameRoom_1.GameRoom(roomId, io));
    return roomId;
}
io.on('connection', (socket) => {
    console.log(`Player connected: ${socket.id}`);
    socket.on('join_game', (playerName) => {
        const roomId = findOrCreateRoom();
        const room = rooms.get(roomId);
        const player = {
            id: socket.id,
            name: playerName || `Player ${socket.id.substring(0, 4)}`,
            roomId
        };
        socket.join(roomId);
        playerRooms.set(socket.id, roomId);
        const success = room.addPlayer(player);
        if (success) {
            socket.emit('joined_room', { roomId, playerId: socket.id });
            // If room is full, start the game
            if (room.getPlayerCount() === 2) {
                room.startGame();
            }
        }
        else {
            socket.emit('room_full');
        }
    });
    socket.on('player_action', (action) => {
        const roomId = playerRooms.get(socket.id);
        if (roomId) {
            const room = rooms.get(roomId);
            room?.handlePlayerAction(socket.id, action);
        }
    });
    socket.on('disconnect', () => {
        console.log(`Player disconnected: ${socket.id}`);
        const roomId = playerRooms.get(socket.id);
        if (roomId) {
            const room = rooms.get(roomId);
            room?.removePlayer(socket.id);
            // Clean up empty rooms
            if (room?.getPlayerCount() === 0) {
                rooms.delete(roomId);
            }
            playerRooms.delete(socket.id);
        }
    });
});
const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
});
