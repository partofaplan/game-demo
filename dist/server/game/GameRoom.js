"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.GameRoom = void 0;
const types_1 = require("../../shared/types");
const GameLogic_1 = require("../../shared/GameLogic");
class GameRoom {
    constructor(roomId, io) {
        this.roomId = roomId;
        this.io = io;
        this.players = new Map();
        this.lastUpdate = Date.now();
        this.gameLoop = null;
        this.gameState = {
            tanks: new Map(),
            projectiles: [],
            explosions: [],
            napalmPatches: [],
            terrain: GameLogic_1.GameLogic.generateTerrain(),
            currentTurn: null,
            turnTimeLeft: types_1.GAME_CONSTANTS.TURN_TIME_LIMIT,
            gamePhase: 'waiting',
            winner: null
        };
    }
    addPlayer(player) {
        if (this.players.size >= 2) {
            return false;
        }
        this.players.set(player.id, player);
        // Create tank for player
        const isPlayer2 = this.players.size === 2;
        const tankX = this.players.size === 1 ? 200 : types_1.GAME_CONSTANTS.LOGICAL_WIDTH - 200;
        const tank = GameLogic_1.GameLogic.createTank(player.id, tankX, isPlayer2);
        // Position tank on terrain
        tank.position.y = GameLogic_1.GameLogic.getTerrainHeight(this.gameState.terrain, tankX) - types_1.GAME_CONSTANTS.TANK_COLLISION_HEIGHT;
        this.gameState.tanks.set(player.id, tank);
        this.broadcastToRoom('player_joined', {
            player,
            gameState: this.serializeGameState()
        });
        return true;
    }
    removePlayer(playerId) {
        this.players.delete(playerId);
        this.gameState.tanks.delete(playerId);
        if (this.gameState.gamePhase === 'playing' && this.gameState.tanks.size === 1) {
            this.endGame();
        }
        this.broadcastToRoom('player_left', { playerId });
    }
    getPlayerCount() {
        return this.players.size;
    }
    startGame() {
        if (this.players.size !== 2)
            return;
        this.gameState.gamePhase = 'playing';
        this.gameState.currentTurn = Array.from(this.players.keys())[0];
        this.gameState.turnTimeLeft = types_1.GAME_CONSTANTS.TURN_TIME_LIMIT;
        this.broadcastToRoom('game_started', {
            gameState: this.serializeGameState()
        });
        this.startGameLoop();
    }
    startGameLoop() {
        this.gameLoop = setInterval(() => {
            this.update();
        }, 1000 / 60); // 60 FPS
    }
    update() {
        if (this.gameState.gamePhase !== 'playing')
            return;
        const now = Date.now();
        const deltaTime = (now - this.lastUpdate) / 1000;
        this.lastUpdate = now;
        // Update game objects
        GameLogic_1.GameLogic.updateProjectiles(this.gameState, deltaTime);
        GameLogic_1.GameLogic.updateExplosions(this.gameState, deltaTime);
        GameLogic_1.GameLogic.updateNapalmPatches(this.gameState, deltaTime);
        GameLogic_1.GameLogic.updateTanks(this.gameState, deltaTime);
        // Update turn timer
        this.gameState.turnTimeLeft -= deltaTime;
        if (this.gameState.turnTimeLeft <= 0) {
            this.nextTurn();
        }
        // Check win condition
        const aliveTanks = Array.from(this.gameState.tanks.values()).filter(tank => tank.isAlive);
        if (aliveTanks.length <= 1) {
            this.endGame();
            return;
        }
        // Broadcast updated game state
        this.broadcastToRoom('game_state', this.serializeGameState());
    }
    handlePlayerAction(playerId, action) {
        if (this.gameState.gamePhase !== 'playing')
            return;
        if (this.gameState.currentTurn !== playerId)
            return;
        const tank = this.gameState.tanks.get(playerId);
        if (!tank || !tank.isAlive)
            return;
        switch (action.type) {
            case 'aim':
                tank.turretAngle = Math.max(-types_1.GAME_CONSTANTS.MAX_TURRET_SWING, Math.min(types_1.GAME_CONSTANTS.MAX_TURRET_SWING, action.data.angle));
                break;
            case 'power':
                tank.power = Math.max(types_1.GAME_CONSTANTS.MIN_LAUNCH_SPEED, Math.min(types_1.GAME_CONSTANTS.MAX_LAUNCH_SPEED, action.data.power));
                break;
            case 'fire':
                const projectile = GameLogic_1.GameLogic.fireTank(tank);
                if (projectile) {
                    this.gameState.projectiles.push(projectile);
                    this.nextTurn();
                }
                break;
            case 'change_ammo':
                tank.currentAmmo = GameLogic_1.GameLogic.nextAmmoType(tank.currentAmmo);
                break;
            case 'shield':
                if (tank.shieldCooldown <= 0) {
                    tank.shieldActive = true;
                    tank.shieldCooldown = 5.0; // 5 second shield duration
                }
                break;
        }
    }
    nextTurn() {
        const playerIds = Array.from(this.players.keys());
        const currentIndex = playerIds.indexOf(this.gameState.currentTurn);
        const nextIndex = (currentIndex + 1) % playerIds.length;
        this.gameState.currentTurn = playerIds[nextIndex];
        this.gameState.turnTimeLeft = types_1.GAME_CONSTANTS.TURN_TIME_LIMIT;
        this.broadcastToRoom('turn_changed', {
            currentTurn: this.gameState.currentTurn
        });
    }
    endGame() {
        if (this.gameLoop) {
            clearInterval(this.gameLoop);
            this.gameLoop = null;
        }
        const aliveTanks = Array.from(this.gameState.tanks.values()).filter(tank => tank.isAlive);
        this.gameState.winner = aliveTanks.length === 1 ? aliveTanks[0].id : null;
        this.gameState.gamePhase = 'ended';
        this.broadcastToRoom('game_ended', {
            winner: this.gameState.winner,
            gameState: this.serializeGameState()
        });
    }
    broadcastToRoom(event, data) {
        this.io.to(this.roomId).emit(event, data);
    }
    serializeGameState() {
        return {
            ...this.gameState,
            tanks: Object.fromEntries(this.gameState.tanks)
        };
    }
}
exports.GameRoom = GameRoom;
