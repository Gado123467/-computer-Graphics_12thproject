#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <memory>
#include <queue>
#include <map>
#include <string>
#include <thread>
#include <mutex>
#include <enet/enet.h>

// Constants and enums
namespace GameConstants {
    constexpr float FIELD_RADIUS = 20.0f;
    constexpr float GOAL_WIDTH = 3.0f;
    constexpr float GOAL_HEIGHT = 2.0f;
    constexpr float GOAL_DEPTH = 0.5f;
    constexpr float GOAL_OFFSET = 1.0f;
    constexpr float PI = 3.14159f;
    constexpr int NUM_CLOUDS = 5;
    constexpr int PORT = 1234;
    constexpr int MAX_PLAYERS = 4;
}

enum class PowerUpType {
    SPEED_BOOST,
    SHIELD,
    BALL_MAGNET,
    GOAL_MULTIPLIER
};

enum class AIState {
    CHASE_BALL,
    DEFEND,
    SUPPORT_ATTACK,
    RETURN_TO_POSITION,
    AVOID_OBSTACLE
};

// Network message types
enum class MessageType {
    PLAYER_POSITION,
    BALL_POSITION,
    POWERUP_SPAWN,
    POWERUP_COLLECTED,
    GOAL_SCORED,
    PLAYER_JOIN,
    PLAYER_LEAVE
};

// Network message structure
struct NetworkMessage {
    MessageType type;
    int playerId;
    float x, y, z;
    float rotation;
    int data;
};

// PowerUp class
class PowerUp : public GameObject {
private:
    PowerUpType type;
    float duration;
    bool active;
    float respawnTime;

public:
    PowerUp(PowerUpType t, float x, float z)
        : GameObject(x, 1.0f, z), type(t), duration(10.0f), active(true), respawnTime(30.0f) {}

    void update(float deltaTime) override {
        if (!active) {
            respawnTime -= deltaTime;
            if (respawnTime <= 0) {
                active = true;
                respawnTime = 30.0f;
                // Randomize position
                float angle = (float)(rand() % 360) * GameConstants::PI / 180.0f;
                x = cos(angle) * (GameConstants::FIELD_RADIUS * 0.7f);
                z = sin(angle) * (GameConstants::FIELD_RADIUS * 0.7f);
            }
        }
    }

    void render() override {
        if (!active) return;

        glPushMatrix();
        glTranslatef(x, y + sin(glutGet(GLUT_ELAPSED_TIME) / 500.0f), z);
        glRotatef(glutGet(GLUT_ELAPSED_TIME) / 20.0f, 0, 1, 0);

        switch (type) {
            case PowerUpType::SPEED_BOOST:
                glColor3f(1.0f, 0.5f, 0.0f);
                break;
            case PowerUpType::SHIELD:
                glColor3f(0.0f, 1.0f, 1.0f);
                break;
            case PowerUpType::BALL_MAGNET:
                glColor3f(1.0f, 0.0f, 1.0f);
                break;
            case PowerUpType::GOAL_MULTIPLIER:
                glColor3f(1.0f, 1.0f, 0.0f);
                break;
        }

        glutSolidOctahedron();
        glPopMatrix();
    }

    bool isActive() const { return active; }
    PowerUpType getType() const { return type; }
    void collect() { active = false; }
};

// Enhanced AI with state machine and path finding
class AIController {
private:
    Car& controlledCar;
    AIState currentState;
    std::vector<Vec2> pathNodes;
    float decisionTimer;

    struct Vec2 { float x, z; };
    std::vector<Vec2> generatePath(Vec2 start, Vec2 end) {
        std::vector<Vec2> path;
        // A* pathfinding implementation here
        return path;
    }

    void updateState(const Ball& ball, const std::vector<Car*>& players) {
        float ballDist = distance(controlledCar.getPosition(), ball.getPosition());
        float goalDist = distance(controlledCar.getPosition(), Vec2{GameConstants::FIELD_RADIUS, 0});

        // State machine logic
        switch (currentState) {
            case AIState::CHASE_BALL:
                if (ballDist > 15.0f) {
                    currentState = AIState::RETURN_TO_POSITION;
                } else if (isTeamInDanger(players)) {
                    currentState = AIState::DEFEND;
                }
                break;

            case AIState::DEFEND:
                if (!isTeamInDanger(players) && ballDist < 10.0f) {
                    currentState = AIState::CHASE_BALL;
                }
                break;

            // Add more state transitions
        }
    }

    bool isTeamInDanger(const std::vector<Car*>& players) {
        // Analyze game situation
        return false;
    }

public:
    AIController(Car& car) : controlledCar(car), currentState(AIState::CHASE_BALL), decisionTimer(0) {}

    void update(float deltaTime, const Ball& ball, const std::vector<Car*>& players) {
        decisionTimer += deltaTime;
        if (decisionTimer >= 0.5f) {
            updateState(ball, players);
            decisionTimer = 0;
        }

        // Execute current state behavior
        switch (currentState) {
            case AIState::CHASE_BALL:
                controlledCar.moveTowards(ball.getPosition(), deltaTime);
                break;
            case AIState::DEFEND:
                defendGoal(ball);
                break;
            // Implement other state behaviors
        }
    }
};

// Network manager class
class NetworkManager {
private:
    ENetHost* host;
    ENetPeer* peer;
    std::queue<NetworkMessage> messageQueue;
    std::mutex queueMutex;
    bool isServer;
    std::map<int, ENetPeer*> clients;

    void handleMessage(const NetworkMessage& msg) {
        switch (msg.type) {
            case MessageType::PLAYER_POSITION:
                updatePlayerPosition(msg);
                break;
            case MessageType::POWERUP_COLLECTED:
                handlePowerUpCollection(msg);
                break;
            // Handle other message types
        }
    }

public:
    NetworkManager(bool server) : isServer(server) {
        enet_initialize();

        if (server) {
            ENetAddress address;
            address.host = ENET_HOST_ANY;
            address.port = GameConstants::PORT;
            host = enet_host_create(&address, GameConstants::MAX_PLAYERS, 2, 0, 0);
        } else {
            host = enet_host_create(NULL, 1, 2, 0, 0);
        }
    }

    void update() {
        ENetEvent event;
        while (enet_host_service(host, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    handleConnect(event);
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    handleReceive(event);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    handleDisconnect(event);
                    break;
            }
        }
    }

    void sendMessage(const NetworkMessage& msg) {
        ENetPacket* packet = enet_packet_create(&msg, sizeof(NetworkMessage),
                                              ENET_PACKET_FLAG_RELIABLE);
        if (isServer) {
            enet_host_broadcast(host, 0, packet);
        } else {
            enet_peer_send(peer, 0, packet);
        }
    }

    ~NetworkManager() {
        if (peer) enet_peer_disconnect(peer, 0);
        enet_host_destroy(host);
        enet_deinitialize();
    }
};

// Enhanced Game World with scoring and power-ups
class GameWorld {
private:
    std::vector<std::unique_ptr<GameObject>> gameObjects;
    std::vector<std::unique_ptr<PowerUp>> powerUps;
    std::unique_ptr<NetworkManager> network;
    std::map<int, int> scores;
    unsigned int fieldDisplayList;
    Ball ball;

    void spawnPowerUp() {
        float angle = (float)(rand() % 360) * GameConstants::PI / 180.0f;
        float radius = (float)(rand() % 70 + 30) / 100.0f * GameConstants::FIELD_RADIUS;
        PowerUpType type = static_cast<PowerUpType>(rand() % 4);
        powerUps.push_back(std::make_unique<PowerUp>(type,
            cos(angle) * radius, sin(angle) * radius));
    }

    void checkCollisions() {
        for (auto& car : getPlayers()) {
            // Check power-up collisions
            for (auto& powerUp : powerUps) {
                if (powerUp->isActive() && distance(car->getPosition(), powerUp->getPosition()) < 1.5f) {
                    applyPowerUp(*car, powerUp->getType());
                    powerUp->collect();
                    // Send network message
                    NetworkMessage msg{MessageType::POWERUP_COLLECTED};
                    network->sendMessage(msg);
                }
            }

            // Check ball collision
            if (distance(car->getPosition(), ball.getPosition()) < 2.0f) {
                handleBallCollision(*car);
            }
        }
    }

    void applyPowerUp(Car& car, PowerUpType type) {
        switch (type) {
            case PowerUpType::SPEED_BOOST:
                car.applySpeedBoost(5.0f);
                break;
            case PowerUpType::SHIELD:
                car.activateShield(10.0f);
                break;
            // Handle other power-up types
        }
    }

public:
    GameWorld(bool isServer) : network(std::make_unique<NetworkManager>(isServer)) {
        initDisplayLists();

        // Create initial power-ups
        for (int i = 0; i < 3; ++i) {
            spawnPowerUp();
        }

        // Create players and AI
        gameObjects.push_back(std::make_unique<Car>(false)); // Player
        for (int i = 0; i < 2; ++i) {
            auto aiCar = std::make_unique<Car>(true);
            aiCar->setAIController(std::make_unique<AIController>(*aiCar));
            gameObjects.push_back(std::move(aiCar));
        }
    }

    void update(float deltaTime) {
        network->update();

        for (auto& obj : gameObjects) {
            obj->update(deltaTime);
        }

        for (auto& powerUp : powerUps) {
            powerUp->update(deltaTime);
        }

        checkCollisions();
        checkScoring();

        // Spawn new power-ups occasionally
        if (rand() % 300 == 0) {
            spawnPowerUp();
        }
    }

    void render() {
        glCallList(fieldDisplayList);

        for (auto& obj : gameObjects) {
            obj->render();
        }

        for (auto& powerUp : powerUps) {
            powerUp->render();
        }

        renderScoreboard();
    }
};

// Main game loop remains largely the same, but now includes network initialization

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Enhanced 3D Multiplayer Game");

    // Initialize OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Create game world - ask if server or client
    bool isServer = (argc > 1 && strcmp(argv[1], "-server") == 0);
    gameWorld = std::make_unique<GameWorld>(isServer);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, update, 0);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}
