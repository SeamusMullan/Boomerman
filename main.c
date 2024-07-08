#include "raylib.h"
#include <stdlib.h>
#include <stdbool.h>

#define try bool __HadError=false;
#define catch(x) ExitJmp:if(__HadError)
#define throw(x) {__HadError=true;goto ExitJmp;}

#define GRID_SIZE 32
#define PLAYER_SPEED 0.1f
#define MAX_BOMBS 10
#define EXPLOSION_FRAMES 5
#define EXPLOSION_DURATION 0.5f
#define MAP_WIDTH 29
#define MAP_HEIGHT 19
#define EXPLOSION_RANGE 3

typedef enum {
    TILE_EMPTY,
    TILE_WALL,
    TILE_DESTRUCTIBLE
} TileType;

typedef enum {
    BOMB_ACTIVE,
    BOMB_EXPLODING,
    BOMB_INACTIVE
} BombState;

typedef struct {
    Vector2 position;
    float timer;
    BombState state;
    int explosionFrame;
    float explosionTimer;
    int explosionLength[4];  // Up, Right, Down, Left
} Bomb;

typedef struct {
    Bomb bombs[MAX_BOMBS];
    int count;
} BombList;

typedef enum {
    GAME_MAIN_MENU,
    GAME_INSTRUCTIONS,
    GAME_PLAYING,
    GAME_RESPAWN
} GameState;

TileType map[MAP_HEIGHT][MAP_WIDTH];
GameState currentState = GAME_MAIN_MENU;
int selectedMenuItem = 0;
const char* mainMenuItems[] = {"Start Game", "Instructions", "Quit"};
const int mainMenuItemsCount = 3;
const char* respawnMenuItems[] = {"Respawn", "Quit to Main Menu"};
const int respawnMenuItemsCount = 2;

void InitMap() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (x == 0 || x == MAP_WIDTH - 1 || y == 0 || y == MAP_HEIGHT - 1)
                map[y][x] = TILE_WALL;
            else if (x % 2 == 0 && y % 2 == 0)
                map[y][x] = TILE_WALL;
            else if (rand() % 3 == 0)
                map[y][x] = TILE_DESTRUCTIBLE;
            else
                map[y][x] = TILE_EMPTY;
        }
    }
    // Ensure player starting position is clear
    map[1][1] = TILE_EMPTY;
    map[1][2] = TILE_EMPTY;
    map[2][1] = TILE_EMPTY;
}

void DrawMap() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            Rectangle rect = {x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE};
            switch (map[y][x]) {
                case TILE_WALL:
                    DrawRectangleRec(rect, GRAY);
                    break;
                case TILE_DESTRUCTIBLE:
                    DrawRectangleRec(rect, BROWN);
                    break;
                default:
                    break;
            }
        }
    }
}

void InitBombList(BombList* list) {
    list->count = 0;
    for (int i = 0; i < MAX_BOMBS; i++) {
        list->bombs[i].state = BOMB_INACTIVE;
    }
}

void AddBomb(BombList* list, Vector2 position) {
    if (list->count < MAX_BOMBS) {
        for (int i = 0; i < MAX_BOMBS; i++) {
            if (list->bombs[i].state == BOMB_INACTIVE) {
                list->bombs[i].position = position;
                list->bombs[i].timer = 3.0f;
                list->bombs[i].state = BOMB_ACTIVE;
                list->bombs[i].explosionFrame = 0;
                list->bombs[i].explosionTimer = 0;
                for (int j = 0; j < 4; j++) {
                    list->bombs[i].explosionLength[j] = 0;
                }
                list->count++;
                break;
            }
        }
    }
}

void CalculateExplosion(Bomb* bomb) {
    int bombX = bomb->position.x / GRID_SIZE;
    int bombY = bomb->position.y / GRID_SIZE;
    int dx[] = {0, 1, 0, -1};  // Up, Right, Down, Left
    int dy[] = {-1, 0, 1, 0};

    for (int dir = 0; dir < 4; dir++) {
        for (int length = 1; length <= EXPLOSION_RANGE; length++) {
            int newX = bombX + dx[dir] * length;
            int newY = bombY + dy[dir] * length;

            if (newX < 0 || newX >= MAP_WIDTH || newY < 0 || newY >= MAP_HEIGHT) {
                break;
            }

            if (map[newY][newX] == TILE_WALL) {
                break;
            }

            bomb->explosionLength[dir] = length;

            if (map[newY][newX] == TILE_DESTRUCTIBLE) {
                map[newY][newX] = TILE_EMPTY;
                break;
            }
        }
    }
}

void UpdateBombs(BombList* list, float deltaTime) {
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (list->bombs[i].state == BOMB_ACTIVE) {
            list->bombs[i].timer -= deltaTime;
            if (list->bombs[i].timer <= 0) {
                list->bombs[i].state = BOMB_EXPLODING;
                list->bombs[i].explosionTimer = EXPLOSION_DURATION;
                CalculateExplosion(&list->bombs[i]);
            }
        } else if (list->bombs[i].state == BOMB_EXPLODING) {
            list->bombs[i].explosionTimer -= deltaTime;
            list->bombs[i].explosionFrame = (int)((EXPLOSION_DURATION - list->bombs[i].explosionTimer) / (EXPLOSION_DURATION / EXPLOSION_FRAMES));
            if (list->bombs[i].explosionTimer <= 0) {
                list->bombs[i].state = BOMB_INACTIVE;
                list->count--;
            }
        }
    }
}

void DrawBombs(const BombList* list) {
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (list->bombs[i].state == BOMB_ACTIVE) {
            DrawCircle((int)list->bombs[i].position.x + GRID_SIZE / 2,
                       (int)list->bombs[i].position.y + GRID_SIZE / 2,
                       GRID_SIZE / 2, BLACK);
        } else if (list->bombs[i].state == BOMB_EXPLODING) {
            Color explosionColor;
            switch(list->bombs[i].explosionFrame) {
                case 0: explosionColor = RED; break;
                case 1: explosionColor = ORANGE; break;
                case 2: explosionColor = YELLOW; break;
                case 3: explosionColor = LIGHTGRAY; break;
                default: explosionColor = WHITE; break;
            }

            int bombX = list->bombs[i].position.x / GRID_SIZE;
            int bombY = list->bombs[i].position.y / GRID_SIZE;
            int dx[] = {0, 1, 0, -1};  // Up, Right, Down, Left
            int dy[] = {-1, 0, 1, 0};

            // Draw center of explosion
            DrawRectangle(bombX * GRID_SIZE, bombY * GRID_SIZE, GRID_SIZE, GRID_SIZE, explosionColor);

            // Draw explosion in four directions
            for (int dir = 0; dir < 4; dir++) {
                for (int length = 1; length <= list->bombs[i].explosionLength[dir]; length++) {
                    int newX = bombX + dx[dir] * length;
                    int newY = bombY + dy[dir] * length;
                    DrawRectangle(newX * GRID_SIZE, newY * GRID_SIZE, GRID_SIZE, GRID_SIZE, explosionColor);
                }
            }
        }
    }
}

bool CheckPlayerCollisionWithExplosion(const Vector2 playerPosition, const BombList* list) {
    int playerTileX = playerPosition.x / GRID_SIZE;
    int playerTileY = playerPosition.y / GRID_SIZE;

    for (int i = 0; i < MAX_BOMBS; i++) {
        if (list->bombs[i].state == BOMB_EXPLODING) {
            int bombX = list->bombs[i].position.x / GRID_SIZE;
            int bombY = list->bombs[i].position.y / GRID_SIZE;
            int dx[] = {0, 1, 0, -1};  // Up, Right, Down, Left
            int dy[] = {-1, 0, 1, 0};

            // Check center of explosion
            if (playerTileX == bombX && playerTileY == bombY) {
                return true;
            }

            // Check explosion in four directions
            for (int dir = 0; dir < 4; dir++) {
                for (int length = 1; length <= list->bombs[i].explosionLength[dir]; length++) {
                    int newX = bombX + dx[dir] * length;
                    int newY = bombY + dy[dir] * length;
                    if (playerTileX == newX && playerTileY == newY) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void DrawMenu(int screenWidth, int screenHeight, const char* title, const char** items, int itemCount, int selectedItem) {
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.7f));
    DrawText(title, screenWidth/2 - MeasureText(title, 40)/2, screenHeight/2 - 150, 40, RED);

    for (int i = 0; i < itemCount; i++) {
        Color itemColor = (i == selectedItem) ? RED : WHITE;
        DrawText(items[i], screenWidth/2 - MeasureText(items[i], 30)/2, screenHeight/2 + i * 50, 30, itemColor);
    }

    DrawText("Use Arrow Keys or WASD to navigate, Enter to select", screenWidth/2 - MeasureText("Use Arrow Keys or WASD to navigate, Enter to select", 20)/2, screenHeight - 40, 20, WHITE);
}

void DrawInstructions(int screenWidth, int screenHeight) {
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.7f));
    DrawText("Instructions", screenWidth/2 - MeasureText("Instructions", 40)/2, 50, 40, RED);

    const char* instructions[] = {
        "- Use WASD or Arrow Keys to move",
        "- Press E to place a bomb",
        "- Destroy walls and avoid explosions",
        "- Last as long as you can!"
    };

    for (int i = 0; i < 4; i++) {
        DrawText(instructions[i], screenWidth/2 - MeasureText(instructions[i], 20)/2, 150 + i * 40, 20, WHITE);
    }

    DrawText("Press Enter to return to Main Menu", screenWidth/2 - MeasureText("Press Enter to return to Main Menu", 20)/2, screenHeight - 40, 20, WHITE);
}

int main(void) {
    bool errored = false;
    const int screenWidth = GRID_SIZE * MAP_WIDTH;
    const int screenHeight = GRID_SIZE * MAP_HEIGHT;
    InitWindow(screenWidth, screenHeight, "Boomerman");

    Vector2 playerPosition = { GRID_SIZE, GRID_SIZE };
    float moveTimer = 0.0f;
    BombList bombList;
    InitBombList(&bombList);
    float bombPlaceTimer = 0.0f;

    try
    {
        InitMap();
    }
    catch(...) {
        errored = true;
    }


    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        switch (currentState) {
            case GAME_MAIN_MENU:
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                    selectedMenuItem = (selectedMenuItem + 1) % mainMenuItemsCount;
                }
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                    selectedMenuItem = (selectedMenuItem - 1 + mainMenuItemsCount) % mainMenuItemsCount;
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    switch (selectedMenuItem) {
                        case 0:  // Start Game
                            currentState = GAME_PLAYING;
                            playerPosition = (Vector2){ GRID_SIZE, GRID_SIZE };
                            InitBombList(&bombList);
                            InitMap();
                            break;
                        case 1:  // Instructions
                            currentState = GAME_INSTRUCTIONS;
                            break;
                        case 2:  // Quit
                            return 0;
                        default:
                            break;
                    }
                }
                break;

            case GAME_INSTRUCTIONS:
                if (IsKeyPressed(KEY_ENTER)) {
                    currentState = GAME_MAIN_MENU;
                    selectedMenuItem = 0;
                }
                break;

            case GAME_PLAYING:
                moveTimer += deltaTime;
                bombPlaceTimer += deltaTime;

                if (moveTimer >= PLAYER_SPEED) {
                    Vector2 newPosition = playerPosition;
                    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) newPosition.x += GRID_SIZE;
                    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) newPosition.x -= GRID_SIZE;
                    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) newPosition.y -= GRID_SIZE;
                    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) newPosition.y += GRID_SIZE;

                    int newTileX = newPosition.x / GRID_SIZE;
                    int newTileY = newPosition.y / GRID_SIZE;
                    if (map[newTileY][newTileX] == TILE_EMPTY) {
                        playerPosition = newPosition;
                    }

                    moveTimer = 0.0f;
                }

                if (IsKeyDown(KEY_E) && bombPlaceTimer >= 0.5f) {
                    Vector2 bombPos = {
                        (float)(((int)playerPosition.x / GRID_SIZE) * GRID_SIZE),
                        (float)(((int)playerPosition.y / GRID_SIZE) * GRID_SIZE)
                    };
                    AddBomb(&bombList, bombPos);
                    bombPlaceTimer = 0.0f;
                }

                UpdateBombs(&bombList, deltaTime);

                if (CheckPlayerCollisionWithExplosion(playerPosition, &bombList)) {
                    currentState = GAME_RESPAWN;
                    selectedMenuItem = 0;
                }
                break;

            case GAME_RESPAWN:
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                    selectedMenuItem = (selectedMenuItem + 1) % respawnMenuItemsCount;
                }
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                    selectedMenuItem = (selectedMenuItem - 1 + respawnMenuItemsCount) % respawnMenuItemsCount;
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    switch (selectedMenuItem) {
                        case 0:  // Respawn
                            currentState = GAME_PLAYING;
                            playerPosition = (Vector2){ GRID_SIZE, GRID_SIZE };
                            InitBombList(&bombList);
                            InitMap();
                            break;
                        case 1:  // Quit to Main Menu
                            currentState = GAME_MAIN_MENU;
                            selectedMenuItem = 0;
                            break;
                        default:
                            break;
                    }
                }
                break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (currentState) {
            case GAME_MAIN_MENU:
                DrawMenu(screenWidth, screenHeight, "Boomerman", mainMenuItems, mainMenuItemsCount, selectedMenuItem);
                break;

            case GAME_INSTRUCTIONS:
                DrawInstructions(screenWidth, screenHeight);
                break;

            case GAME_PLAYING:
                DrawMap();
                DrawRectangle((int)playerPosition.x, (int)playerPosition.y, GRID_SIZE, GRID_SIZE, MAROON);
                DrawBombs(&bombList);
                DrawText("Move with WASD, place bomb with E", 10, 10, 20, DARKGRAY);
                break;

            case GAME_RESPAWN:
                DrawMap();
                DrawBombs(&bombList);
                DrawMenu(screenWidth, screenHeight, "Game Over", respawnMenuItems, respawnMenuItemsCount, selectedMenuItem);
                break;
        }

        EndDrawing();
    }

    CloseWindow();

    if (errored) {
        return -1;
    }

    return 0;
}