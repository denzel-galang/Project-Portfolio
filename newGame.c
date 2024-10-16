/* 
* Coded by Denzel Galang
* 
* This program is a maze video game that uses ASCII graphics.
* The objective of the game is to reach the opposite corner to proceed to the
* next level while navigating through obstacles and avoiding enemies. Different
* enemy types are introduced as the player progresses through each level. Each
* level is able to be read in through a text file. The text files store the
* layout of each level.
* 
* Dynamic arrays are used to keep track of the number of entities that spawn on 
* each level according to the layout stored in its text file. Linked lists are 
* also used to keep track of and update certain particles that appear on screen.
* Heaps also implement the priority queue used for the A* pathfinding algorithm
* that the enemies use to reach the player. 
*/

#define _CRT_SECURE_NO_WARNINGS
#define _CRTDBG_MAP_ALLOC
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <windows.h>
#include <stdbool.h>

#define GRID_SIZE 23
#define NUM_SQUARES (GRID_SIZE - 3) * (GRID_SIZE - 3)
#define FPS 10
#define FRAME_DELAY 1000 / FPS // in milliseconds

#define AGGRO_RADIUS 6

// defining all the colors to be used
#define RED FOREGROUND_RED
#define GREEN FOREGROUND_GREEN
#define BLUE FOREGROUND_BLUE
#define INTENSITY FOREGROUND_INTENSITY
#define YELLOW RED | GREEN
#define PURPLE RED | BLUE
#define LIGHTER_BLUE GREEN | BLUE
#define WHITE RED | GREEN | BLUE
#define GRAY INTENSITY
#define LIGHT_RED RED | INTENSITY
#define LIGHT_GREEN GREEN | INTENSITY
#define LIGHT_YELLOW RED | GREEN | INTENSITY
#define LIGHT_BLUE BLUE | INTENSITY
#define LIGHT_PURPLE RED | BLUE | INTENSITY
#define CYAN GREEN BLUE | INTENSITY
#define LIGHT_WHITE RED | GREEN | BLUE | INTENSITY

typedef enum {
    ERROR_OPENING_FILE = 1,
    UNKNOWN_ENEMY_TYPE,
    NO_EXIT_EXISTS,
    NO_PLAYER_EXISTS,
    MALLOC_WALL_POSITION_FAILED,
    MALLOC_ENEMY_POSITION_FAILED,
    MALLOC_ENEMY_COUNT_FAILED,
    REALLOC_WALL_POSITION_FAILED,
    REALLOC_ENEMY_COUNT_FAILED,
    MALLOC_PLAYER_LAYER_FAILED,
    MALLOC_WALL_LAYER_FAILED,
    MALLOC_ITEM_LAYER_FAILED,
    MALLOC_ALL_ENEMIES_FAILED,
    MALLOC_ALL_ITEMS_FAILED,
    MALLOC_ITEM_COUNT_FAILED,
    REALLOC_ITEM_COUNT_FAILED
} ErrorCode;

typedef enum {
    BASIC_ENEMY,
    PATROL_ENEMY,
    TELEPORT_ENEMY,
    CHASER_ENEMY,
    TRAPPER_ENEMY,
    BURST_ENEMY,
    MIMIC_ENEMY,
    WALL_BREAKER_ENEMY,
    SHOOTER_ENEMY,
    NUM_ENEMY_TYPES // this variable automatically counts the number of enemy types
} EnemyType;

typedef enum {
    OBJ_ITEM,
    BATTERIES_ITEM,
    REPLLENT_ITEM,
    INVISIBILITY_ITEM,
    NUM_ITEM_TYPES
} ItemType;

typedef enum {
    EXIT_OBJ,
    ITEM_OBJ
 } ObjectiveType;

struct Position {
    int x, y;
};
typedef struct Position Position;

struct Enemy {
    Position roamArr[NUM_SQUARES];
    Position pos;
    Position playerLSP; // LSP = last seen position
    int roamIndex;
    int moveInterval;
    int specialAbility;
    char passiveMarker;
    char aggroMarker;
    bool isRepelled;
    bool isAggro;
};
typedef struct Enemy Enemy;

struct Flashlight {
    int batteryLife;
    bool isActive;
    bool isDisabled;
};
typedef struct Flashlight Flashlight;

struct Player {
    Position pos;
    Flashlight flashlight;
    int facingDirection;
    int lives;
};
typedef struct Player Player;

struct Bullet {
    Position pos;
    int direction;
    struct Bullet* next;
};
typedef struct Bullet Bullet;

struct Particle {
    Position pos;
    char marker;
    int timer; // the duration of the particle in frames
    struct Particle* next;
};
typedef struct Particle Particle;

struct Node {
    Position pos;
    int gCost; // cost from start to current node
    int hCost; // heuristic cost to end node
    int fCost; // gCost + hCost
    struct Node* parent; // pointer to parent node
};
typedef struct Node Node;

struct PriorityQueue { // will be implemented as a heap
    Node** nodes;
    int size;
    int capacity;
};
typedef struct PriorityQueue PriorityQueue;

struct Level {
    Position start;
    Position end;

    Position* walls; // array of wall positions
    int wallCount;

    Position** allEnemies; // 2-D array of all enemies; each index is an array of a specific enemy type
    int* enemyCounts; // array of all enemy counts; each index holds the count of a specific enemy type

    Position** allItems; // 2-D array of all items; each index is an array of a specific item type
    int* itemCounts;

    ObjectiveType objectiveID;

    ErrorCode hasError; // checks for any errors during initialization before starting the game
};
typedef struct Level Level;

struct AllEntities {
    /* the game board is composed of 3 layers, each layer storing all locations of that entity type:
        - the player layer (player and enemies)
        - the wall layer (all non-traversable spaces)
        - and the item layer (all collectibles that the player can use) */

        /* if one layer were to be used to store all 3 types, it would be difficult to display entities that had the same location.
            for instance, if an enemy were to walk over the space of an item, then that item would be hidden from game since the
            enemy movement mechanics work by clearing the old position of the grid. an item marker restoration function would have
            to be called after to ensure that the item can still be seen on the grid immediately after. by keeping each type of entity
            in its own layer, such functions would not be necessary. */

    unsigned char** playerLayer;
    unsigned char** wallLayer;
    unsigned char** itemLayer;
};
typedef struct AllEntities AllEntities;

struct GameBoard {
    AllEntities grid;
    Position** allItems;
    Enemy** allEnemies;
    Player player;
    ErrorCode hasError;
};
typedef struct GameBoard GameBoard;

bool isValid(AllEntities grid, Position entity, char ID);
bool canMove(Position pos, AllEntities grid);
bool doesDetect(Position detector, Position detectee, int detectionRadius);
Particle* addNewParticle(Particle* head, Position pos, char marker, int frameTimer);
void moveAllEnemies(int frameCounter, Level level, AllEntities* grid, Enemy** enemy, Player player, Particle** bombHead, Bullet** head);
void setBomb(Particle** head, Position pos, unsigned char** itemLayer, int spawnChance);
void roamToUnvisited(Enemy* enemy, AllEntities grid);
void updateAllParticles(Particle** bombHead, Particle** explosionHead, unsigned char** itemLayer, unsigned char** wallLayer, int frameCounter);
bool movePlayer(Level level, AllEntities* grid, Player* player, char movement);
bool gameLoop(Level* level, GameBoard* grid);
void initializeRoamArr(Position* roamArr);
void updateBullets(Bullet** head, Particle** bulletParticles, AllEntities* grid);
void displayBombFlicker(Particle* head, unsigned char** itemLayer, int frameCounter);
int findBulletDirection(Position old, Position new);
void freeBombs(Particle* bombHead, Particle* explosionHead);
void updateAllParticles(Particle** bombHead, Particle** explosionHead, unsigned char** itemLayer, unsigned char** wallLayer, int frameCounter);
void updateParticleType(Particle** typeHead, Particle** explosionHead, unsigned char** itemLayer, unsigned char** wallLayer, int frameCounter);
unsigned char** initializeGrid(void);
GameBoard initializeGameBoard(Level level, bool isLevel);
Player initializePlayer(Level level);
Enemy initializeEnemy(unsigned char** playerLayer, Position newPos, char passiveMarker, char aggroMarker, int moveInterval);
bool matchesPosition(Position a, Position b);
bool gameWin(Level level, Position pos);
bool gameLose(Level level, GameBoard game, Particle* explosionHead);
void setCursorPosition(int x, int y);
Bullet* shootBullet(Bullet* head, Position bulletPos, int direction);
void makeRandomMove(AllEntities grid, Position* newPos, Position oldPos);
Enemy** initializeAllEnemies(Level level, unsigned char** playerLayer);
Position** initializeAllItems(Level level, unsigned char** itemLayer);
Level initializeLevel(void);
void shuffleArr(Position* roamArr, int size);
Node* findNode(PriorityQueue* openSet, Position pos);

// all function prototypes for the A* search algorithm implemented for the enemys' pathfinding of the player
PriorityQueue* createPriorityQueue(int capacity);
Node* createNode(Position pos, int gCost, int hCost, Node* parent);
Node* pop(PriorityQueue* pq);
void push(PriorityQueue* pq, Node* node);
void swapNodes(Node** a, Node** b);
void heapifyUp(PriorityQueue* pq, int index);
void heapifyDown(PriorityQueue* pq, int index);
void finalizePath(Node* endNode, Position* path, int* pathLength);
int calculateHCost(Position a, Position b);
bool findPath(AllEntities grid, Position start, Position end, Position* path, int* pathLength);

// all text files that will be used to load the levels
const char* allLevelFiles[] = {
    "gameOver.txt",
    "level1.txt",
    "level2.txt",
    "level3.txt",
    "level4.txt",
    "level5.txt",
    "level6.txt",
    "level7.txt",
    "level8.txt",
    "level9.txt",
    "level10.txt",
    "level11.txt",
    "level12.txt",
    "level13.txt",
    "level14.txt",
    "level15.txt",
    "level16.txt",
    "level17.txt",
    "level18.txt",
    "level19.txt",
    "level21.txt",
    "level22.txt",
    "level23.txt",
    "level24.txt",
    "level25.txt",
    "level26.txt",
    "level27.txt",
    "level28.txt",
    "level29.txt",
    "level30.txt",
    "level31.txt",
    "level32.txt",
    "level33.txt",
    "level34.txt",
    "level35.txt",
    "level36.txt",
    "level37.txt",
    "level38.txt",
    "level39.txt",
    "level40.txt",
};

// defining the menu options when the player clears a level
const char* continueMenu[] = {
    "Continue",
    "Exit to Main Menu"
};

const char* quitMenu[] = {
    "No",
    "Yes"
};

// main menu options
const char* mainMenu[] = {
    "New Game",
    "Level Select",
    "Credits",
    "Quit"
};

const char* gameOverMenu[] = {
    "Retry",
    "Return to Main Menu"
};

const Position postlevelCursor = { 0, GRID_SIZE + 5 };
const Position mainMenuCursor = { 0, 2 };
const Position INVALID_POS = { -1, -1 };

// directional arrays used when making a random move
const int dx[4] = { 0, 0, -1, 1 };
const int dy[4] = { -1, 1, 0, 0 };

// defining the appearance of the enemies on the grid
const char passiveEnemyMarkers[NUM_ENEMY_TYPES] = { 'o', 147, 232, 'O', 234, 145, 'x', 233, 226 };
const char aggroEnemyMarkers[NUM_ENEMY_TYPES] = { 'o', 147, 254, '0', 234, 146, '@', '@', 226 };

// defining all item markers in the game
const char timebombMarkers[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
const char helpfulItemMarkers[] = {'a'};
const char harmfulItemMarkers[];

// defining the movement interval of each enemy type
const int moveIntervals[NUM_ENEMY_TYPES] = { 10, 10, 1, 1, 1, 1, 10, 10, 1 };

int calculateHCost(Position a, Position b) {

    // calculate the Manhattan distance from the finishing point
    return abs(a.x - b.x) + abs(a.y - b.y);
}

PriorityQueue* createPriorityQueue(int capacity) {

    // allocate memory for a priority queue that keeps tracks of nodes to explore, ordered by their fCost
    PriorityQueue* pq = malloc(sizeof(PriorityQueue));
    if (pq != NULL) {
        pq->nodes = malloc(sizeof(Node*) * capacity);
        if (pq->nodes == NULL) {
            fprintf(stderr, "\nMALLOC ERROR: Memory allocation for the priority queue nodes (enemy pathfinding) failed!\n");
            free(pq->nodes);
            free(pq);
            return NULL;
        }
    }
    else {
        fprintf(stderr, "\nMALLOC ERROR: Memory allocation for the priority queue failed!\n");
        free(pq);
        return NULL;
    }

    pq->size = 0;
    pq->capacity = capacity;

    return pq;
}

void swapNodes(Node** a, Node** b) {
    Node* temp = *a;
    *a = *b;
    *b = temp;
}

void heapifyUp(PriorityQueue* pq, int index) {

    // ensure that a newly added node is in the correct position
    while (index > 0) {
        int parentIndex = (index - 1) / 2;

        // swap the nodes if the parent's fCost is more expensive
        if (pq->nodes[index]->fCost < pq->nodes[parentIndex]->fCost) {
            swapNodes(&pq->nodes[index], &pq->nodes[parentIndex]);
            index = parentIndex;
        }
        else break;
    }
}

void heapifyDown(PriorityQueue* pq, int index) {
    int leftChild, rightChild, smallestChild;

    // ensure that the new root is in the correct position after removing the root node
    while (index < pq->size) {
        leftChild = 2 * index + 1;
        rightChild = 2 * index + 2;
        smallestChild = index;

        // determine if the current node has a child, then determine which child (if any) has the smaller fCost
        if (leftChild < pq->size && pq->nodes[leftChild]->fCost < pq->nodes[smallestChild]->fCost) {
            smallestChild = leftChild;
        }

        if (rightChild < pq->size && pq->nodes[rightChild]->fCost < pq->nodes[smallestChild]->fCost) {
            smallestChild = rightChild;
        }

        // swap the nodes with the smaller child, then move down the heap
        if (smallestChild != index) {
            swapNodes(&pq->nodes[index], &pq->nodes[smallestChild]);
            index = smallestChild;
        }
        else break;
    }
}

void push(PriorityQueue* pq, Node* node) {

    // allocate more memory if the current size of the pq exceeds its capacity
    if (pq->size >= pq->capacity) {
        pq->capacity *= 2;
        Node** temp = realloc(pq->nodes, sizeof(Node*) * pq->capacity);
        if (temp == NULL) {
            fprintf(stderr, "\nMALLOC ERROR: Memory reallocation for the priority queue nodes failed!\n");
            return;
        }
        pq->nodes = temp;
    }

    // add the new node to the pq, then heapify up to ensure it is in the correct position
    pq->nodes[pq->size] = node;
    heapifyUp(pq, pq->size);
    pq->size++;
}

Node* pop(PriorityQueue* pq) {
    if (pq->size == 0) {
        return NULL;
    }

    // get the root node to return its value
    Node* root = pq->nodes[0];

    // replace the root node with the last node of the heap
    pq->size--;
    pq->nodes[0] = pq->nodes[pq->size];

    // heapify down to maintain the heap
    heapifyDown(pq, 0);
    return root;
}

Node* createNode(Position newPos, int gCost, int hCost, Node* parent) {
    Node* newNode = malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "\nMALLOC ERROR: Memory allocation to create a new node in the priority queue failed!\n");
        return NULL;
    }

    newNode->pos = newPos;
    newNode->gCost = gCost;
    newNode->hCost = hCost;
    newNode->fCost = gCost + hCost;
    newNode->parent = parent;

    return newNode;
}

void finalizePath(Node* endNode, Position* path, int* pathLength) {
    Node* currentNode = endNode;
    *pathLength = 0;

    // convert the finished path from the pq to the path array by following the end node's parent pointers
    while (currentNode != NULL) {
        path[(*pathLength)++] = currentNode->pos;
        currentNode = currentNode->parent;
    }

    // reverse the path to get it in correct order from start to end
    for (int i = 0; i < *pathLength / 2; i++) {
        Position temp = path[i];
        path[i] = path[*pathLength - i - 1];
        path[*pathLength - i - 1] = temp;
    }
}

bool findPath(AllEntities grid, Position start, Position end, Position* path, int* pathLength) {
    bool pathFound = false;

    // openSet stores the nodes to explore, closedSet stores the nodes already explored
    PriorityQueue* openSet = createPriorityQueue(128);
    bool closedSet[GRID_SIZE][GRID_SIZE] = { false };

    // track all nodes created to free them later
    Node** allNodes = malloc(sizeof(Node*) * (GRID_SIZE * GRID_SIZE));
    int nodeCount = 0;

    // initialize the heap by creating the root node for it
    Node* startNode = createNode(start, 0, calculateHCost(start, end), NULL);
    push(openSet, startNode);
    allNodes[nodeCount++] = startNode;

    // loop thru all nodes to be explored
    while (openSet->size > 0) {

        // evaluate the root node of the pq and mark it as visited on the closedSet
        Node* currentNode = pop(openSet);
        closedSet[currentNode->pos.y][currentNode->pos.x] = true;

        // return true if the destination is reached
        if (matchesPosition(currentNode->pos, end)) {
            finalizePath(currentNode, path, pathLength);
            pathFound = true;
            break;
        }

        // evaluate each neighboring node defined by each cardinal direction
        for (int i = 0; i < 4; i++) {
            Position newPos;
            newPos.x = currentNode->pos.x + dx[i];
            newPos.y = currentNode->pos.y + dy[i];

            // if the neighboring node is valid and not already in the closedSet, then push it to the openSet to be evaluated
            if (!closedSet[newPos.y][newPos.x] && isValid(grid, newPos, 'e')) {
                int newGCost = currentNode->gCost + 1;
                int newHCost = calculateHCost(newPos, end);

                // check if the node is already in the openSet
                Node* existingNode = findNode(openSet, newPos);
                if (existingNode != NULL) {

                    // if the new path is shorter, update the node
                    if (newGCost < existingNode->hCost) {
                        existingNode->gCost = newGCost;
                        existingNode->fCost = newGCost + newHCost;
                        existingNode->parent = currentNode;
                    }
                }
                else { // if the node is not in the openSet, add it                    
                    Node* newNode = createNode(newPos, newGCost, newHCost, currentNode);
                    push(openSet, newNode);
                    allNodes[nodeCount++] = newNode;
                }
            }
        }
    }

    // free all nodes created
    for (int i = 0; i < nodeCount; i++) {
        free(allNodes[i]);
    }
    free(allNodes);

    // free the priority queue
    free(openSet->nodes);
    free(openSet);

    return pathFound; // returns false if the player can't be reached
}

Node* findNode(PriorityQueue* openSet, Position pos) {
    for (int i = 0; i < openSet->size; i++) {
        Node* node = openSet->nodes[i];

        // return the found node if any node matches the given position
        if (matchesPosition(node->pos, pos)) {
            return node;
        }
    }
    return NULL; // return NULL if not found
}

void drawGameState(AllEntities grid, Level level) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // used to change the color of text
    setCursorPosition(0, 2); // reset the cursor to the start of the third line to overwrite the grid

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {

            // order of precedence: beings are printed on top, then items, then the walls
            if (grid.playerLayer[y][x] != ' ') {

                // mark the player with a red color
                if (grid.playerLayer[y][x] == 'X') {
                    SetConsoleTextAttribute(hConsole, LIGHT_GREEN);
                    putchar(grid.playerLayer[y][x]);
                    SetConsoleTextAttribute(hConsole, WHITE);
                }
                else {
                    putchar(grid.playerLayer[y][x]);
                }
            }
            else if (grid.itemLayer[y][x] != ' ') {

                // mark all collectible items with a light blue color
                if (grid.itemLayer[y][x] == '!') {
                    SetConsoleTextAttribute(hConsole, LIGHT_BLUE);
                    putchar(grid.itemLayer[y][x]);
                    SetConsoleTextAttribute(hConsole, WHITE);
                }
                else {
                    putchar(grid.itemLayer[y][x]);
                }
            }
            else if (grid.wallLayer[y][x] != ' ') {

                // mark the exit with a light green color
                if (grid.wallLayer[y][x] == 'E') {

                    // make the exit red for all item objective levels, then make it green when the objective is completed
                    if (level.objectiveID == ITEM_OBJ) {
                        SetConsoleTextAttribute(hConsole, level.itemCounts[OBJ_ITEM] > 0 ? RED : LIGHT_GREEN);
                        putchar(grid.wallLayer[y][x]);
                        SetConsoleTextAttribute(hConsole, WHITE);
                    }
                    else {
                        SetConsoleTextAttribute(hConsole, LIGHT_GREEN);
                        putchar(grid.wallLayer[y][x]);
                        SetConsoleTextAttribute(hConsole, WHITE);
                    }
                }
                else {
                    putchar(grid.wallLayer[y][x]);
                }
            }
            else {
                putchar(' ');
            }
        }
        putchar('\n');
    }

    // print the number of remaining objective items left in item objective levels
    if (level.objectiveID == ITEM_OBJ) {

        // turn the text to light green once all items have been collected
        SetConsoleTextAttribute(hConsole, level.itemCounts[OBJ_ITEM] == 0 ? LIGHT_GREEN : LIGHT_BLUE);
        printf("\t! left: %d  \n", level.itemCounts[OBJ_ITEM]);
        SetConsoleTextAttribute(hConsole, WHITE);
    }
}

bool isValid(AllEntities grid, Position entity, char ID) {

    // bounds checking
    if (entity.x < 1 || entity.x > GRID_SIZE - 2 || entity.y < 1 || entity.y > GRID_SIZE - 2) {
        return false;
    }
    
    // if the wall layer has a wall at that location
    if (grid.wallLayer[entity.y][entity.x] == 178) {
        return false;
    }

    switch (ID) {
    case 'e': // additional checks for enemies

        // if an enemy encounters the player, allow the movement
        if (grid.playerLayer[entity.y][entity.x] == 'X') {
            return true;
        }

        // prevents multiple enemies from occupying the same space
        if (grid.playerLayer[entity.y][entity.x] != ' ') {
            return false;
        }
        break;
    case 'i': // additional checks for items

        // prevents multiple items from occupying the same space
        if (grid.itemLayer[entity.y][entity.x] != ' ') {
            return false;
        }
        break;
    case 'b':

        // the bullet stops traveling once it hits a living entity
        if (grid.playerLayer[entity.y][entity.x] != ' ') {

            if (grid.playerLayer[entity.y][entity.x] == 226) {
                return true;
            }
            else return false;
        }        
        break;
    }
    return true;
}

bool doesDetect(Position detector, Position detectee, int detectionRadius) {
    return abs(detector.x - detectee.x) <= detectionRadius &&
        abs(detector.y - detectee.y) <= detectionRadius;
}

bool hasLineOfSight(Position pos, Position player, unsigned char** wallLayer, int aggroRange) {

    // get the starting (ghost) and ending (player) coordinates
    int xStart = pos.x;
    int yStart = pos.y;
    int xFinish = player.x;
    int yFinish = player.y;

    // calculate differences in coordinates
    int dx = abs(xFinish - xStart);
    int dy = abs(yFinish - yStart);

    // return false if the enemy is too far away to see the player
    if (dx > aggroRange || dy > aggroRange) {
        return false;
    }

    // determine the direction of movement along the x and y axis
    int xStep = (xStart < xFinish) ? 1 : -1;
    int yStep = (yStart < yFinish) ? 1 : -1;

    int err = dx - dy; // error term set to the difference in x coordinates

    // loop until we reach the player's position
    while (true) {

        // check if there is an obstacle at the current position
        if (wallLayer[yStart][xStart] == 178) {
            return false; // line of sight is blocked
        }

        // if the current position matches the player's position, we have line of sight
        if (xStart == xFinish && yStart == yFinish) {
            return true; // reached the player's position
        }

        int e2 = err * 2; // double the error term for comparison

        // adjust the error term and move in the x direction if needed
        if (e2 > -dy) {
            err -= dy;
            xStart += xStep;

            // check for diagonal walls when moving in both x and y directions
            if (wallLayer[yStart][xStart - xStep] == 178 && wallLayer[yStart - yStep][xStart] == 178) {
                return false; // line of sight is blocked by diagonal walls
            }
        }

        // adjust the error term and move in the y direction if needed
        else if (e2 < dx) {
            err += dx;
            yStart += yStep;

            // check for diagonal walls when moving in both x and y directions
            if (wallLayer[yStart][xStart - xStep] == 178 && wallLayer[yStart - yStep][xStart] == 178) {
                return false; // line of sight is blocked by diagonal walls
            }
        }
    }
}

void movePatrolEnemy(AllEntities grid, Enemy* patrol, Position* newPos, Position oldPos) {

    // define an array to store which directions have been tried already
    int triedDirections[4] = { 0 };
    int directionsTried = 0;

    // test the current direction to see if it is valid
    int newDirection = patrol->specialAbility;
    newPos->x = oldPos.x + dx[newDirection];
    newPos->y = oldPos.y + dy[newDirection];
    
    // if the current direction is not valid, then choose another one to move to
    while (!isValid(grid, *newPos, 'e') && directionsTried != 4) {

        // mark the current direction as tried
        triedDirections[newDirection] = 1;
        directionsTried++;

        // choose another direction that hasn't been tested yet
        do {
            newDirection = rand() % 4;
        } while (triedDirections[newDirection] && directionsTried != 4);

        // test the new position with the new direction again
        newPos->x = oldPos.x + dx[newDirection];
        newPos->y = oldPos.y + dy[newDirection];
    }
    patrol->specialAbility = newDirection; // finalize the new direction
}

void moveTeleporterEnemy(AllEntities grid, Position* newPos, Position pos) {
    int toleranceCounter = 0;
    do { // generate a new random location that isn't the same as the player's
        newPos->x = rand() % (GRID_SIZE - 2) + 1;
        newPos->y = rand() % (GRID_SIZE - 2) + 1;

        // allow 100 attempts to teleport, otherwise the enemy doesn't move if the 100th attempt is still invalid
        if (toleranceCounter == 100) break;
        toleranceCounter++;
    } while (!isValid(grid, *newPos, 'e') || matchesPosition(pos, *newPos));
}

void moveAllEnemies(int frameCounter, Level level, AllEntities* grid, Enemy** allEnemies, Player player, Particle** bombHead, Bullet** bulletHead) {
    /* ENEMY MOVEMENT BEHAVIORS:

           BASIC ENEMY: will roam to a random location on the grid. after reaching it, another random location is chosen until all possible locations
                        have been roamed to. then the cycle restarts.
           PATROL ENEMY: will continue to move in a straight line, then will turn to a random direction upon hitting a wall.
           TELEPORTER ENEMY: remains stationary for a set interval until it randomly teleports to a different location.
           CHASER ENEMY: will make random moves until it sees the player within a vision radius, then will chase
                         the player until the player is outside its vision radius. the chaser will still move to the player's last seen position
                         (LSP); upon reaching the LSP, if the enemy still does not directly see the player, it will return to
                         making random moves.
           TRAPPER ENEMY: moves in the same fashion as the basic enemy, but drops bombs upon random intervals that will
                            blow up any surrounding walls after 10 seconds and will kill the player if they are within the blast radius.
           BURST ENEMY: remains stationary then moves to a random location. if it sees the player, then it targets their location next
           SHOOTER ENEMY: will shoot at the player from a distance and can be seen from afar, bullets can damage walls */

    // iterate thru each enemy type
    for (int i = 0; i < NUM_ENEMY_TYPES; i++) {

        // skip all absent enemy types
        if (level.enemyCounts[i] == 0) continue;

        // iterate thru each enemy of that enemy type
        for (int j = 0; j < level.enemyCounts[i]; j++) {

            // only move the current enemy if the current frame is at the end of their move interval
            if (frameCounter % allEnemies[i][j].moveInterval != 0) continue;

            const Position oldPos = allEnemies[i][j].pos;
            Position newPos = oldPos;
            Position path[NUM_SQUARES];
            int pathLength = 0;
            bool LSPflag = false;
            bool isBullet = false;

            switch (i) {
            case BASIC_ENEMY:
            {   // will always roam to a random location on the grid, no aggro state
                if (matchesPosition(allEnemies[i][j].playerLSP, INVALID_POS)) {
                    roamToUnvisited(&allEnemies[i][j], *grid);
                }
                LSPflag = true;
                break;
            }
            case PATROL_ENEMY:
            {   // for this enemy, specialAbility will hold a number between 0-3 inclusive to represent its current direction to travel in
                movePatrolEnemy(*grid, &allEnemies[i][j], &newPos, oldPos);
                break;
            }
            case TELEPORT_ENEMY:
            {   // for this enemy, specialAbility will represent the number of movement intervals passed before the enemy actually teleports
                // specialAbility is used instead of its normally defined movement interval in order to be able to implement its flickering effect
                if (allEnemies[i][j].specialAbility > 0) {
                    allEnemies[i][j].specialAbility--;

                    // make the enemy flicker between its passive and aggro states to indicate that its movement interval is soon (10 frames away)
                    if (allEnemies[i][j].specialAbility <= 10) {

                        /* the layer must be directly updated rather than changing the isAggro status of the enemy.
                           this is because the isValid check at the end of each iteration will override the flickering effect.
                           sure, the aggro status itself will flicker, but it wont be shown on the grid directly. */
                        grid->playerLayer[oldPos.y][oldPos.x] = (frameCounter % 2 == 0) ? passiveEnemyMarkers[TELEPORT_ENEMY] : aggroEnemyMarkers[TELEPORT_ENEMY];
                    }
                    else {
                        grid->playerLayer[oldPos.y][oldPos.x] = allEnemies[i][j].passiveMarker;
                    }
                }
                else { // teleport when the interval is done
                    moveTeleporterEnemy(*grid, &newPos, player.pos);
                    allEnemies[i][j].specialAbility = 35; // reset the interval to 35 frames
                }
                break;
            }
            case CHASER_ENEMY:
            {
                // check every frame if the chaser has line of sight of the player
                if (hasLineOfSight(oldPos, player.pos, grid->wallLayer, AGGRO_RADIUS)) {
                    allEnemies[i][j].playerLSP = player.pos; // update the player's LSP
                    allEnemies[i][j].isAggro = true;
                }

                // if aggro'd, then move every 2 frames towards the player
                if (allEnemies[i][j].isAggro && frameCounter % 2 == 0) {
                    LSPflag = true;
                }
                else { // if not aggro'd, then make a random move every 10 frames
                    if (allEnemies[i][j].specialAbility > 0) {
                        allEnemies[i][j].specialAbility--;
                    }
                    else {
                        LSPflag = true;
                        allEnemies[i][j].specialAbility = 10; // resetting the move timer
                    }
                }
                break;
            }
            case TRAPPER_ENEMY:
            {   // the move interval of this enemy is initially every 7 frames with a 50% chance to set a bomb at its next roaming location.
                // however, the move interval slowly shortens over 2 minutes until it reaches 0, upon which it will move every frame.
                // the probability of setting a bomb will also slowly increase from 50% to 100% over 2 minutes.
                int maxTime = 1200;
                int initialInterval = 10;
                int initialProbability = 50;
                int newMoveInterval = initialInterval - (initialInterval * frameCounter / maxTime);
                int bombSpawnChance = initialProbability + (initialProbability * frameCounter / maxTime);

                if (allEnemies[i][j].specialAbility > 0) {
                    allEnemies[i][j].specialAbility--;
                }
                else {
                    // movement behavior is the same as the braindead enemy
                    if (matchesPosition(allEnemies[i][j].playerLSP, INVALID_POS)) {
                        roamToUnvisited(&allEnemies[i][j], *grid);

                        // chance to set a bomb every time the random location is reached
                        setBomb(bombHead, newPos, grid->itemLayer, bombSpawnChance);
                    }
                    allEnemies[i][j].specialAbility = newMoveInterval;
                    LSPflag = true;
                }
                break;
            }
            case BURST_ENEMY:
            {
                // calculating the move interval to be shorter as more time passes (50 frames initially, then shortens to 0)
                int initialInterval = 50;
                int maxTime = 900;
                int newMoveInterval = initialInterval - (initialInterval * frameCounter / maxTime);

                // after maxTime frames pass, the enemy will pursue the player indefinitely while permanently skipping the passive phase
                if (newMoveInterval < 0) {
                    allEnemies[i][j].playerLSP = player.pos;
                }

                // only have the enemy move when it is aggro'd (therefore, it is only active when aggro'd)
                if (allEnemies[i][j].isAggro) {
                    LSPflag = true;
                    break;
                }

                // travel phase: the enemy beelines straight for the player's exact location.
                //      -- it is important to note that this enemy will always keep moving until the LSP is reached
                if (allEnemies[i][j].specialAbility > 0) {
                    allEnemies[i][j].specialAbility--;

                    // flicker for 10 frames to indicate burst movement is happening soon
                    if (allEnemies[i][j].specialAbility <= 10) {
                        grid->playerLayer[oldPos.y][oldPos.x] = (frameCounter % 2 == 0) ? passiveEnemyMarkers[BURST_ENEMY] : aggroEnemyMarkers[BURST_ENEMY];
                    }
                    else {
                        grid->playerLayer[oldPos.y][oldPos.x] = allEnemies[i][j].passiveMarker;
                    }
                }
                else { // passive phase: sits still for a decreasingly short interval

                    // target the player's exact location
                    allEnemies[i][j].playerLSP = player.pos;

                    // activating the burst enemy once the interval is done
                    allEnemies[i][j].isAggro = true;

                    // resetting the interval to a shorter one
                    allEnemies[i][j].specialAbility = newMoveInterval;
                }
                break;
            }
            case SHOOTER_ENEMY:
                // switch to aggro behavior if the shooter sees the player
                if (hasLineOfSight(oldPos, player.pos, grid->wallLayer, AGGRO_RADIUS * 2)) {
                    allEnemies[i][j].isAggro = true;
                    allEnemies[i][j].playerLSP = player.pos;

                    if (frameCounter % 10 == 0) {
                        LSPflag = true;
                    }

                    if (allEnemies[i][j].specialAbility > 0) {
                        allEnemies[i][j].specialAbility--;
                    }
                    else {
                        allEnemies[i][j].specialAbility = 0;
                        isBullet = true;
                    }
                }
                else { // define the enemy's passive behavior

                    // find another position to roam to if the current one is already reached
                    if (matchesPosition(allEnemies[i][j].playerLSP, INVALID_POS)) {
                        roamToUnvisited(&allEnemies[i][j], *grid);
                        allEnemies[i][j].specialAbility = 0;
                    }

                    // move only every 20 frames
                    if (frameCounter % 20 == 0) {
                        LSPflag = true;
                    }
                }
                break;
            }

            if (LSPflag) {

                // move to the player's LSP if known
                if (!matchesPosition(allEnemies[i][j].playerLSP, INVALID_POS)) {

                    // use the A* pathfinding algorithm to go to the player's LSP
                    if (findPath(*grid, oldPos, allEnemies[i][j].playerLSP, path, &pathLength)) {

                        // either move to the next step of the path or move directly if the destination is reached
                        newPos = (pathLength > 1) ? path[1] : path[0];
                    }
                    else { // roam to another random location if the path is impossible
                        roamToUnvisited(&allEnemies[i][j], *grid);
                    }

                    // set the LSP to an invalid location once reached then de-aggro the enemy
                    if (matchesPosition(allEnemies[i][j].pos, allEnemies[i][j].playerLSP)) {
                        allEnemies[i][j].playerLSP = INVALID_POS;
                        allEnemies[i][j].isAggro = false;
                    }
                }
                else { // if LSP is unknown, make a random move instead                    
                    makeRandomMove(*grid, &newPos, oldPos);
                    allEnemies[i][j].isAggro = false;
                }
            }

            if (isBullet && LSPflag) {
                int a = findBulletDirection(oldPos, newPos);

                *bulletHead = shootBullet(*bulletHead, allEnemies[i][j].pos, a);
            }

            // validate the new position before finalizing the move
            if (isValid(*grid, newPos, 'e')) {

                // update the enemy's position to the new one
                allEnemies[i][j].pos = newPos;

                // clear the enemy's old position from the player grid
                grid->playerLayer[oldPos.y][oldPos.x] = ' ';

                // mark the enemy's new position with its appropriate marker depending on its aggro state
                grid->playerLayer[newPos.y][newPos.x] = allEnemies[i][j].isAggro ? allEnemies[i][j].aggroMarker : allEnemies[i][j].passiveMarker;
            }
        }
    }    
}

void updateBullets(Bullet** head, Particle** explosionHead, AllEntities* grid) {
    Bullet* current = *head;
    Bullet* prev = NULL;

    while (current != NULL) {
        grid->itemLayer[current->pos.y][current->pos.x] = ' '; // clear the bullet's old position

        Position newPos = current->pos;
        newPos.x += dx[current->direction];
        newPos.y += dy[current->direction];

        // if the bullet's new position is invalid, delete the bullet and add an explosion particle
        if (!isValid(*grid, newPos, 'b')) {

            // unlink the bullet from the linked list
            if (prev == NULL) {
                *head = current->next;
            }
            else {
                prev->next = current->next;
            }

            // if the bullet strikes some wall within the grid, clear it
            if (newPos.x >= 1 && newPos.x < GRID_SIZE - 1 && newPos.y >= 1 && newPos.y < GRID_SIZE - 1) {
                grid->wallLayer[newPos.y][newPos.x] = ' ';

                // have an explosion particle replace the wall for 5 frames
                *explosionHead = addNewParticle(*explosionHead, newPos, '#', 5);
            }
            else { // if it strikes an edge wall, don't clear it

                // have the particle show up on the space before the edge wall instead
                *explosionHead = addNewParticle(*explosionHead, current->pos, '#', 5);
            }

            // delete the bullet
            Bullet* temp = current;
            current = current->next;
            free(temp);
        }
        else { // otherwise, continue moving in a straight direction
            current->pos = newPos;
            grid->itemLayer[current->pos.y][current->pos.x] = 254; // mark the bullet on the grid

            prev = current;
            current = current->next;
        }
    }
}

Bullet* shootBullet(Bullet* head, Position bulletPos, int direction) {
    Bullet* newBullet = malloc(sizeof(Bullet));
    if (newBullet == NULL) {
        fprintf(stderr, "Memory allocation for shooting a bullet failed.\n");
        return NULL;
    }

    newBullet->pos = bulletPos;
    newBullet->direction = direction;
    newBullet->next = head;

    return newBullet; // return the new head of the bullet linked list
}

int findBulletDirection(Position old, Position new) {
    int x = new.x - old.x;
    int y = new.y - old.y;

    for (int i = 0; i < 4; i++) {
        if (x == dx[i] && y == dy[i]) {
            return i;
        }
    }
    return 9999;
}

void detonateBomb(Particle** explosionHead, Position pos, int blastRadius, unsigned char** itemLayer, unsigned char** wallLayer) {

    // preparing to clear all surrounding cells by defining the bounds to clear
    int startX = pos.x - blastRadius;
    int startY = pos.y - blastRadius;
    int endX = pos.x + blastRadius;
    int endY = pos.y + blastRadius;

    // clear all surrounding spaces except the walls along the edges of the game board
    for (int x = startX; x <= endX; x++) {
        for (int y = startY; y <= endY; y++) {
            if (x >= 1 && x < GRID_SIZE - 1 && y >= 1 && y < GRID_SIZE - 1) {
                wallLayer[y][x] = ' '; // clear the wall

                // add a new explosion particle that will last for half a second
                *explosionHead = addNewParticle(*explosionHead, (Position) { x, y }, '#', FPS / 2);
            }
        }
    }
}

void displayBombFlicker(Particle* bombHead, unsigned char** itemLayer, int frameCounter) {
    Particle* current = bombHead;

    // traverse the entire list to see which bombs are about to detonate
    while (current != NULL) {

        // see which bombs are on their final second of their countdown
        if (current->timer <= FPS) {

            // have the bomb flicker every other frame
            itemLayer[current->pos.y][current->pos.x] = (frameCounter % 2 == 0) ? 'O' : '0';
        }
        current = current->next;
    }
}

void updateAllParticles(Particle** bombHead, Particle** explosionHead, unsigned char** itemLayer, unsigned char** wallLayer, int frameCounter) {
    
    // update the linked list of bomb particles:
    // the explosionHead has to be passed in so that explosion particles can be added upon any bomb detonations
    updateParticleType(bombHead, explosionHead, itemLayer, wallLayer, frameCounter);

    // as for updating all other particle types, the extra explosionHead argument is just passed in as NULL
    updateParticleType(explosionHead, NULL, itemLayer, wallLayer, frameCounter);

    // have all bombs flicker between 'O' and '0' on their final second before detonating
    displayBombFlicker(*bombHead, itemLayer, frameCounter);
}

void updateParticleType(Particle** typeHead, Particle** explosionHead, unsigned char** itemLayer, unsigned char** wallLayer, int frameCounter) {
    
    /* all particles are defined to be items, therefore they will be stored in the itemLayer */
    
    Particle* prev = NULL;
    Particle* current = *typeHead;

    // determine whether or not the particle type being passed in is the bomb particle
    bool isTypeBomb = (explosionHead != NULL);

    // traverse thru the entire list of the particle
    while (current != NULL) {
        current->timer--;

        // delete the particle node if its timer is up
        if (current->timer <= 0) {
            itemLayer[current->pos.y][current->pos.x] = ' '; // clear it from the grid

            // unlink the node from the list
            if (prev == NULL) {
                *typeHead = current->next;
            }
            else {
                prev->next = current->next;
            }
 
            // if the particle type is a bomb, detonate it to add its explosion particles
            if (isTypeBomb) {
                detonateBomb(explosionHead, current->pos, 1, itemLayer, wallLayer);
            }

            // free the node using a temp pointer
            Particle* temp = current;
            current = current->next;
            free(temp);
        }
        else { // otherwise, the particle is marked on the grid and stays on the screen

            /* bombs are the only particles that have changing markers because of their countdown timer,
               so this function must detect whether or not the list being passed in is the head of the
               bomb list to update each bomb marker accordingly. */

            // if the bomb list being updated, then mark each bomb according to its countdown
            if (isTypeBomb) {
                itemLayer[current->pos.y][current->pos.x] = timebombMarkers[current->timer / FPS];
            }
            else { // for all other particle types, just print the particle's marker
                itemLayer[current->pos.y][current->pos.x] = current->marker;
            }

            prev = current;
            current = current->next;
        }
    }
}

Particle* addNewParticle(Particle* head, Position pos, char marker, int frameTimer) {
    Particle* newParticle = malloc(sizeof(Particle));
    if (newParticle == NULL) {
        fprintf(stderr, "Memory allocation for creating particle type %c failed.\n", marker);
        return NULL;
    }

    newParticle->pos = pos;
    newParticle->marker = marker;
    newParticle->timer = frameTimer;
    newParticle->next = head;

    return newParticle; // return the new head of the linked list
}

void setBomb(Particle** bombHead, Position pos, unsigned char** itemLayer, int spawnChance) {

    // define the probability of a bomb being set (50%) and ensure that the current space on the item layer is empty before setting it
    if (rand() % 100 < spawnChance && itemLayer[pos.y][pos.x] == ' ') {

        // add a new bomb to the linked list of bombs
        *bombHead = addNewParticle(*bombHead, pos, '9', 10 * FPS);
    }
}

void makeRandomMove(AllEntities grid, Position* newPos, Position oldPos) {

    // don't attempt moving if the enemy has no available directions to move to
    if (!canMove(oldPos, grid)) {
        return;
    }

    int toleranceCounter = 0;
    do { // 20 attempts to make a random move in any of the cardinal directions
        int index = rand() % 4;
        newPos->x = oldPos.x + dx[index];
        newPos->y = oldPos.y + dy[index];
        toleranceCounter++;
    } while (!isValid(grid, *newPos, 'e') && toleranceCounter < 20);
}

bool canMove(Position pos, AllEntities grid) {
    for (int i = 0; i < 4; i++) {
        Position newPos;

        // test all 4 adjacent positions according to each cardinal direction
        newPos.x = pos.x + dx[i];
        newPos.y = pos.y + dy[i];

        // return true if there's at least 1 valid direction
        if (isValid(grid, newPos, 'e')) {
            return true;
        }
    }
    return false; // return false if there are no valid directions
}

void roamToUnvisited(Enemy* enemy, AllEntities grid) {
    Position path[NUM_SQUARES];
    int pathLength = 0;
    int shuffleCounter = 0;

    // check each adjacent space to see if the enemy can even move at all
    if (!canMove(enemy->pos, grid)) {
        enemy->playerLSP = INVALID_POS;
        return;
    }

    do { // select a new position to roam to as long as it has a valid path to and is valid itself

        // reset the index to 0 if all positions have been iterated thru, then re-shuffle to prevent the same roam order from occurring
        if (enemy->roamIndex >= NUM_SQUARES) {
            enemy->roamIndex = 0;
            shuffleArr(enemy->roamArr, NUM_SQUARES);
            shuffleCounter++;
        }

        /* any enemy should never have to shuffle the roam array more than once since every position on the grid will eventually be checked. 
            by checking if the roam array has been shuffled more than once, the loop will eventually terminate. */
        if (shuffleCounter > 1) {
            enemy->playerLSP = INVALID_POS;
            break;
        }

        // assign the new position to the enemy's LSP of the player, then increment the roamIndex to identify the next roaming position
        enemy->playerLSP = enemy->roamArr[enemy->roamIndex];
        enemy->roamIndex++;

    } while (!findPath(grid, enemy->pos, enemy->playerLSP, path, &pathLength) || !isValid(grid, enemy->playerLSP, 'e'));
}

bool matchesPosition(Position a, Position b) {
    return a.x == b.x && a.y == b.y;
}

bool movePlayer(Level level, AllEntities* grid, Player* player, char movement) {
    Position oldPos = player->pos;

    switch (movement) {
    case 'W':
    case 'w':
    case 72: // up arrow key
        player->pos.y--;
        player->facingDirection = 0;
        break;
    case 'D':
    case 'd':
    case 77: // right arrow key
        player->pos.x++;
        player->facingDirection = 1;
        break;
    case 'S':
    case 's':
    case 80: // down arrow key
        player->pos.y++;
        player->facingDirection = 2;
        break;
    case 'A':
    case 'a':
    case 75: // left arrow key
        player->pos.x--;
        player->facingDirection = 3;
        break;
    default: // return 0 on unknown inputs
        return false;
    }

    // allow the player to move to the exit
        /* note: this is needed since the exit is out of bounds, meaning that the isValid
                 function would return 0, resulting in the exit being impossible to move to */
    if (matchesPosition(player->pos, level.end)) {

        // invalidate the movement if not all objects have been collected for an item objective level
        if (level.objectiveID == ITEM_OBJ) {
            if (level.itemCounts[OBJ_ITEM] > 0) {
                player->pos = oldPos;
                return false;
            }
        }
        return true;
    }

    // if the movement is invalid, the old position is kept
    if (!isValid(*grid, player->pos, ' ')) {
        player->pos = oldPos;
        return false;
    }
    else { // otherwise, clear the old position and update the grid with the player's new position
        grid->playerLayer[oldPos.y][oldPos.x] = ' ';
        grid->playerLayer[player->pos.y][player->pos.x] = 'X';
        return true;
    }
}

void removeItem(Level* level, Position** allItems, unsigned char** itemLayer, int itemType, int itemIndex) {

    // clear the item from the grid
    itemLayer[allItems[itemType][itemIndex].y][allItems[itemType][itemIndex].x] = ' ';

    // replace the item to be removed with the last item in the list
    allItems[itemType][itemIndex] = allItems[itemType][level->itemCounts[itemType] - 1];

    // decrement the item count for this item type
    level->itemCounts[itemType]--;

    // free the array of that item type if there are no items of that type left
    if (level->itemCounts[itemType] == 0) {
        free(allItems[itemType]);
        allItems[itemType] = NULL;
    }
    else { // otherwise, reallocate memory to shrink the array
        Position* temp = realloc(allItems[itemType], sizeof(Position) * level->itemCounts[itemType]);
        if (temp == NULL) {
            fprintf(stderr, "Memory reallocation for allItems array failed.\n");
        }
        else {
            allItems[itemType] = temp;
        }
    }
}

void hasItem(Level* level, Position** allItems, Position pos, unsigned char** itemLayer) {

    // iterate thru each item type and all items of that type
    for (int i = 0; i < NUM_ITEM_TYPES; i++) {
        if (level->itemCounts[i] == 0) continue;
        for (int j = 0; j < level->itemCounts[i]; j++) {

            // if the player has picked up an item, remove it from play
            if (matchesPosition(allItems[i][j], pos)) {
                removeItem(level, allItems, itemLayer, i, j);
            }
        }
    }
}

bool gameLoop(Level* level, GameBoard* gameElements) {
    bool didWin;

    // initialize the start of both linked lists to NULL for the presence of any terrorist enemies
    Particle* allBombs = NULL;
    Particle* allExplosions = NULL;

    // initialize the start of the bullet linked list to NULL for the presence of shooter enemies
    Bullet* allBullets = NULL;

    unsigned int frameCounter = 1;
    while (true) {

        // measuring the time at the start of the loop iteration
        clock_t frameStart = clock();

        // detects keyboard input for player movement
        if (_kbhit()) {
            if (!movePlayer(*level, &gameElements->grid, &gameElements->player, _getch())) {
                continue;
            }
        }

        if (gameLose(*level, *gameElements, allExplosions)) {
            didWin = false;
            break;
        }

        // updates the countdown timers of all particles (bombs included) as well as their display state on the grid
        updateAllParticles(&allBombs, &allExplosions, gameElements->grid.itemLayer, gameElements->grid.wallLayer, frameCounter);

        updateBullets(&allBullets, &allExplosions, &gameElements->grid);

        // check if the player has collected an item after making a move
        hasItem(level, gameElements->allItems, gameElements->player.pos, gameElements->grid.itemLayer);

        // redraw the game every frame to show the updated positions of all entities
        drawGameState(gameElements->grid, *level);

        moveAllEnemies(frameCounter, *level, &gameElements->grid, gameElements->allEnemies, gameElements->player, &allBombs, &allBullets);

        // game-ending conditions: either the player completes the game objective or gets caught by an enemy
        if (gameWin(*level, gameElements->player.pos)) {
            didWin = true;
            break;
        }
        else if (gameLose(*level, *gameElements, allExplosions)) {
            didWin = false;
            break;
        }

        // measuring the time again at the end of the iteration to calculate the frame delay
        clock_t frameTime = clock() - frameStart;
        if (FRAME_DELAY > frameTime) {
            Sleep(FRAME_DELAY - frameTime); // adding a frame delay prevents screen flickering
        }

        frameCounter++;
    }
        
    freeBombs(allBombs, allExplosions); // free both linked lists before going back to the main menu
    return didWin;
}

void freeBombs(Particle* bombHead, Particle* explosionHead) {

    // free all allocated memory for any bombs that have been set but not yet detonated when the level is over
    while (bombHead != NULL) {
        Particle* temp = bombHead;
        bombHead = bombHead->next;
        free(temp);
    }

    // same memory deallocation process for any explosions
    while (explosionHead != NULL) {
        Particle* temp = explosionHead;
        explosionHead = explosionHead->next;
        free(temp);
    }
}

bool gameWin(Level level, Position pos) {

    // if there are items to collect, ensure that the player collects them all first before reaching the exit
    if (level.objectiveID == ITEM_OBJ) {
        return matchesPosition(pos, level.end) && level.itemCounts[OBJ_ITEM] == 0;
    }
    else { // in a normal game, just check if the exit has been reached
        return matchesPosition(pos, level.end);
    }
}

bool gameLose(Level level, GameBoard game, Particle* explosionHead) {

    // check if any enemies are in the same space as the player
    for (int i = 0; i < NUM_ENEMY_TYPES; i++) {
        if (level.enemyCounts[i] == 0) continue;
        for (int j = 0; j < level.enemyCounts[i]; j++) {
            if (matchesPosition(game.allEnemies[i][j].pos, game.player.pos)) {
                return true;
            }
        }
    }

    // check if the player is caught in an explosion as marked by its particles
    while (explosionHead != NULL) {
        if (matchesPosition(explosionHead->pos, game.player.pos)) {
            return true;
        }
        explosionHead = explosionHead->next;
    }

    return false;
}

unsigned char** initializeGrid() {

    // allocate memory for the grid
    unsigned char** grid = malloc(GRID_SIZE * sizeof(char*));
    if (grid != NULL) {
        for (int i = 0; i < GRID_SIZE; i++) {
            grid[i] = malloc(GRID_SIZE * sizeof(char));
            if (grid[i] == NULL) {
                fprintf(stderr, "\nMALLOC ERROR: Memory allocation to initialize the starting grid failed!\n");
                for (int j = 0; j < i; j++) {
                    free(grid[j]);
                }
                free(grid);
                return NULL;
            }
        }
    }
    else {
        fprintf(stderr, "\nMALLOC ERROR: Memory allocation to initialize the starting grid failed!\n");
        return NULL;
    }

    // initially set each layer's entire grid as empty spaces
    for (int i = 0; i < GRID_SIZE; i++) {
        memset(grid[i], ' ', GRID_SIZE);
    }

    return grid;
}

Position** initializeAllItems(Level level, unsigned char** itemLayer) {

    // allocate memory for each item type, also accounting for the number of items for each item type
    Position** allItems = malloc(sizeof(Position*) * NUM_ITEM_TYPES);
    if (allItems != NULL) {
        for (int i = 0; i < NUM_ITEM_TYPES; i++) {
            if (level.itemCounts[i] > 0) {
                allItems[i] = malloc(sizeof(Position) * level.itemCounts[i]);
                if (allItems[i] == NULL) {
                    for (int j = 0; j < i; j++) {
                        free(allItems[j]);
                    }
                    free(allItems);
                    return NULL;
                }
            }
            else {
                allItems[i] = NULL;
            }
        }
    }
    else return NULL;

    // copy the information from the level data to the actual game
    for (int i = 0; i < NUM_ITEM_TYPES; i++) {
        if (level.itemCounts[i] == 0) continue;
        for (int j = 0; j < level.itemCounts[i]; j++) {
            allItems[i][j] = level.allItems[i][j];
        }
    }

    // populate the itemLayer with the locations of each objective item stored in the level struct
    for (int i = 0; i < level.itemCounts[OBJ_ITEM]; i++) {
        itemLayer[level.allItems[OBJ_ITEM][i].y][level.allItems[OBJ_ITEM][i].x] = '!';
    }

    return allItems;
}

GameBoard initializeGameBoard(Level level, bool isLevel) {
    GameBoard newBoard;

    // initialize each entity layer as a blank grid
    newBoard.grid.playerLayer = initializeGrid();
    newBoard.grid.wallLayer = initializeGrid();
    newBoard.grid.itemLayer = initializeGrid();

    // ensure that memory allocation for each grid was successful
    if (newBoard.grid.playerLayer == NULL) {
        newBoard.hasError = MALLOC_PLAYER_LAYER_FAILED;
    }
    else if (newBoard.grid.wallLayer == NULL) {
        newBoard.hasError = MALLOC_WALL_LAYER_FAILED;
    }
    else if (newBoard.grid.itemLayer == NULL) {
        newBoard.hasError = MALLOC_ITEM_LAYER_FAILED;
    }
    else { // finalize layer initialization by copying data from the level struct to the appropriate layer
        newBoard.allEnemies = initializeAllEnemies(level, newBoard.grid.playerLayer);
        newBoard.allItems = initializeAllItems(level, newBoard.grid.itemLayer);

        // ensure that memory allocation for both arrays was successful
        if (newBoard.allEnemies == NULL) {
            newBoard.hasError = MALLOC_ALL_ENEMIES_FAILED;
        }
        else if (newBoard.allItems == NULL) {
            newBoard.hasError = MALLOC_ALL_ITEMS_FAILED;
        }
        else {
            newBoard.hasError = 0; // no errors, all memory allocation was successful
            newBoard.player = initializePlayer(level);

            // populate the empty wall layer according to the wall location data
            for (int i = 0; i < level.wallCount; i++) {
                newBoard.grid.wallLayer[level.walls[i].y][level.walls[i].x] = 178;
            }

            // isLevel distinguishes between a level and the game over screen
            if (isLevel) {
                newBoard.grid.playerLayer[level.start.y][level.start.x] = 'X'; // mark the player's starting location
                newBoard.grid.wallLayer[level.end.y][level.end.x] = 'E'; // mark the exit
            }
        }
    }
    return newBoard;
}

Player initializePlayer(Level level) {
    Player newPlayer;

    newPlayer.pos = level.start;

    newPlayer.lives = 5;
    newPlayer.facingDirection = 0;

    newPlayer.flashlight.batteryLife = 100;
    newPlayer.flashlight.isActive = false;
    newPlayer.flashlight.isDisabled = true;

    return newPlayer;
}

void initializeRoamArr(Position* roamArr) {
    int index = 0;

    // initialize each index of the array to be every possible position on the grid
    for (int i = 1; i < GRID_SIZE - 2; i++) {
        for (int j = 1; j < GRID_SIZE - 2; j++) {
            roamArr[index++] = (Position){ i, j };
        }
    }
    shuffleArr(roamArr, index); // shuffle the array to randomize the roaming order of the positions
}

void shuffleArr(Position* roamArr, int size) {

    /* the array is shuffled thru the Fisher-Yates algorithm. we start from the end of the array
        to swap with a randomly chosen element from the portion of the array that hasn't been
        shuffled yet, which ensures that each possible permutation of the array is equally likely.
        
        starting from the beginning would potentially introduce bias, as the random choices would be
        influenced by the already shuffled elements. */

    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1); // choose a random index to swap with

        // swap the positions
        Position temp = roamArr[i];
        roamArr[i] = roamArr[j];
        roamArr[j] = temp;
    }
}

Enemy initializeEnemy(unsigned char** playerLayer, Position newPos, char passiveMarker, char aggroMarker, int moveInterval) {
    Enemy newEnemy;

    // initializing the passive roaming mechanics of the enemy
    initializeRoamArr(newEnemy.roamArr);
    newEnemy.roamIndex = 0;

    newEnemy.pos = newPos; // determine the enemy's starting position according to the data stored in the level struct
    newEnemy.playerLSP = INVALID_POS;

    playerLayer[newPos.y][newPos.x] = passiveMarker; // place the enemy on the grid in its initial passive state
    newEnemy.moveInterval = moveInterval; // amount of frames that pass before the enemy moves again
    newEnemy.aggroMarker = aggroMarker;
    newEnemy.passiveMarker = passiveMarker;

    newEnemy.isAggro = false;
    newEnemy.isRepelled = false;
    newEnemy.specialAbility = 0;

    return newEnemy;
}

Enemy** initializeAllEnemies(Level level, unsigned char** playerLayer) {

    // allocate memory for all enemy types
    Enemy** allEnemies = malloc(sizeof(Enemy*) * NUM_ENEMY_TYPES);
    if (allEnemies != NULL) {
        for (int i = 0; i < NUM_ENEMY_TYPES; i++) {

            // allocate memory for each enemy type according to the number of enemies present of that tpye
            if (level.enemyCounts[i] > 0) {
                allEnemies[i] = malloc(sizeof(Enemy) * level.enemyCounts[i]);
                if (allEnemies[i] == NULL) {
                    for (int j = 0; j < i; j++) {
                        free(allEnemies[j]);
                    }
                    free(allEnemies);
                    return NULL;
                }
            }
            else { // if there are no enemies of that type present, then set its corresponding array to NULL
                allEnemies[i] = NULL;
            }
        }
    }
    else return NULL;

    // finalize initialization of each enemy according to their type, move interval, and aggro/passive grid markers
    for (int i = 0; i < NUM_ENEMY_TYPES; i++) {
        if (level.enemyCounts[i] == 0) continue;
        for (int j = 0; j < level.enemyCounts[i]; j++) {            
            allEnemies[i][j] = initializeEnemy(playerLayer, level.allEnemies[i][j], passiveEnemyMarkers[i], aggroEnemyMarkers[i], moveIntervals[i]);
        }
    }
    return allEnemies;
}

void freeGameBoard(GameBoard* gameElements) {
    for (int i = 0; i < GRID_SIZE; i++) {
        free(gameElements->grid.playerLayer[i]);
        free(gameElements->grid.wallLayer[i]);
        free(gameElements->grid.itemLayer[i]);
    }
    free(gameElements->grid.playerLayer);
    free(gameElements->grid.wallLayer);
    free(gameElements->grid.itemLayer);

    for (int i = 0; i < NUM_ENEMY_TYPES; i++) {
        free(gameElements->allEnemies[i]);
    }
    free(gameElements->allEnemies);

    for (int i = 0; i < NUM_ITEM_TYPES; i++) {
        free(gameElements->allItems[i]);
    }
    free(gameElements->allItems);
}

Level parseLevelLayout(const char* fileName) {
    Level newLevel = initializeLevel();
    if (newLevel.hasError) {
        return newLevel;
    }

    // opening the file to parse
    FILE* levelFile = fopen(fileName, "r");
    if (levelFile == NULL) {
        newLevel.hasError = ERROR_OPENING_FILE;
        return newLevel;
    }

    // initialize the starting and ending locations to invalid positions beforehand for error checking
    newLevel.start = INVALID_POS;
    newLevel.end = INVALID_POS;

    // file parsing: using x- and y-coordinates to correspond with each row and column location in the text file
    char line[GRID_SIZE + 2]; // +2 for null-terminating and newline chars
    bool unknownEnemyFlag = false;
    for (int y = 0; fgets(line, sizeof(line), levelFile) != NULL; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            Position currentPos = (Position){ x, y };

            // matching each char in the text file with its corresponding entity,
            // then storing each entity's location in its corresponding array while keeping count of each entity type
            switch (line[x]) {
            case ' ': // ignore all empty spaces
                break;
            case 'X': // setting the starting location
                newLevel.start = currentPos;
                break;
            case 'E': // setting the location objective
                newLevel.end = currentPos;
                break;
            case '#': // storing the location of each wall
                newLevel.walls[newLevel.wallCount++] = currentPos;
                break;
            case 'B':
                newLevel.allEnemies[BASIC_ENEMY][newLevel.enemyCounts[BASIC_ENEMY]++] = currentPos;
                break;
            case 'P':
                newLevel.allEnemies[PATROL_ENEMY][newLevel.enemyCounts[PATROL_ENEMY]++] = currentPos;
                break;
            case 't': // lowercase t: teleporter enemy
                newLevel.allEnemies[TELEPORT_ENEMY][newLevel.enemyCounts[TELEPORT_ENEMY]++] = currentPos;
                break;
            case 'C':
                newLevel.allEnemies[CHASER_ENEMY][newLevel.enemyCounts[CHASER_ENEMY]++] = currentPos;
                break;
            case 'T': // uppercase T: bombper enemy
                newLevel.allEnemies[TRAPPER_ENEMY][newLevel.enemyCounts[TRAPPER_ENEMY]++] = currentPos;
                break;
            case 's': // lowercase s: burst enemy
                newLevel.allEnemies[BURST_ENEMY][newLevel.enemyCounts[BURST_ENEMY]++] = currentPos;
                break;
            case 'M':
                newLevel.allEnemies[MIMIC_ENEMY][newLevel.enemyCounts[MIMIC_ENEMY]++] = currentPos;
                break;
            case 'W':
                newLevel.allEnemies[WALL_BREAKER_ENEMY][newLevel.enemyCounts[WALL_BREAKER_ENEMY]++] = currentPos;
                break;
            case 'S': // uppercase S: shooter enemy
                newLevel.allEnemies[SHOOTER_ENEMY][newLevel.enemyCounts[SHOOTER_ENEMY]++] = currentPos;
                break;
            case '!':
                newLevel.allItems[OBJ_ITEM][newLevel.itemCounts[OBJ_ITEM]++] = currentPos;
                break;
            default: // immediately return an error upon encountering an unidentified entity type
                newLevel.hasError = UNKNOWN_ENEMY_TYPE;
                fclose(levelFile);
                return newLevel;
            }
        }
    }

    // finish initialization only if both the valid start and end positions have been identified
    if (matchesPosition(newLevel.start, INVALID_POS)) {
        newLevel.hasError = NO_PLAYER_EXISTS;
    }
    else if (matchesPosition(newLevel.end, INVALID_POS)) {
        newLevel.hasError = NO_EXIT_EXISTS;
    }
    else {
        // shrinking the wall positions array to avoid memory wastage
        Position* temp = realloc(newLevel.walls, sizeof(Position) * newLevel.wallCount);
        if (temp == NULL) {
            newLevel.hasError = REALLOC_WALL_POSITION_FAILED;
            fclose(levelFile);
            return newLevel;
        }
        newLevel.walls = temp;

        // reallocate memory for the array of each enemy type
        for (int i = 0; i < NUM_ENEMY_TYPES; i++) {

            // if there are no enemies of that enemy type, then free the array storing that type
            if (newLevel.enemyCounts[i] == 0) {
                free(newLevel.allEnemies[i]);
                newLevel.allEnemies[i] = NULL;
            }
            else { // otherwise, shrink the array to store only the updated count of that type
                Position* temp = realloc(newLevel.allEnemies[i], sizeof(Position) * newLevel.enemyCounts[i]);
                if (temp == NULL) {
                    newLevel.hasError = REALLOC_ENEMY_COUNT_FAILED;
                    break;
                }
                newLevel.allEnemies[i] = temp;
            }
        }

        // reallocate memory for each item type array, same process as the enemy arrays
        for (int i = 0; i < NUM_ITEM_TYPES; i++) {
            if (newLevel.itemCounts[i] == 0) {
                free(newLevel.allItems[i]);
                newLevel.allItems[i] = NULL;
            }
            else {
                Position* temp = realloc(newLevel.allItems[i], sizeof(Position) * newLevel.itemCounts[i]);
                if (temp == NULL) {
                    newLevel.hasError = REALLOC_ITEM_COUNT_FAILED;
                    break;
                }
                newLevel.allItems[i] = temp;
            }
        }

        // if any items are present, change the objective of the game to collecting all items
        if (newLevel.itemCounts[OBJ_ITEM] > 0) {
            newLevel.objectiveID = ITEM_OBJ;
        }
    }
    fclose(levelFile);
    return newLevel;
}

Level initializeLevel(void) {
    Level newLevel;

    // allocate memory for all dynamic arrays
    newLevel.walls = malloc(sizeof(Position) * GRID_SIZE * GRID_SIZE);
    newLevel.allItems = malloc(sizeof(Position*) * NUM_ITEM_TYPES);
    newLevel.allEnemies = malloc(sizeof(Position*) * NUM_ENEMY_TYPES);
    newLevel.enemyCounts = malloc(sizeof(int) * NUM_ENEMY_TYPES);
    newLevel.itemCounts = malloc(sizeof(int) * NUM_ITEM_TYPES);

    // return the necessary error code if any memory allocations fail
    if (newLevel.walls == NULL) {
        newLevel.hasError = MALLOC_WALL_POSITION_FAILED;
    }
    else if (newLevel.allItems == NULL) {
        newLevel.hasError = MALLOC_ALL_ITEMS_FAILED;
    }
    else if (newLevel.allEnemies == NULL) {
        newLevel.hasError = MALLOC_ENEMY_POSITION_FAILED;
    }
    else if (newLevel.enemyCounts == NULL) {
        newLevel.hasError = MALLOC_ENEMY_COUNT_FAILED;
    }
    else if (newLevel.itemCounts == NULL) {
        newLevel.hasError = MALLOC_ITEM_COUNT_FAILED;
    }
    else {
        newLevel.hasError = 0; // no errors, all memory allocation was successful
        newLevel.wallCount = 0;
        newLevel.objectiveID = EXIT_OBJ; // default objective: just reach the exit

        // allocate memory for each enemy type
        for (int i = 0; i < NUM_ENEMY_TYPES; i++) {
            Position* temp = malloc(sizeof(Position) * GRID_SIZE * GRID_SIZE);
            if (temp == NULL) {
                newLevel.hasError = MALLOC_ENEMY_POSITION_FAILED;
                break;
            }

            /* note that the memory allocation is equal to the area of the grid. this is to ensure that
                the worst case of memory allocation is taken of, which is when a single enemy type occupies
                every space of the entire game board.

                however, after the number of each enemy type is properly counted in the parseLevelLayout
                function, the memory is later reallocated to a smaller size to avoid any memory wastage. */

            newLevel.allEnemies[i] = temp;
            newLevel.enemyCounts[i] = 0;
        }

        // allocate memory for each item type, again allocating each type for the worst case
        for (int i = 0; i < NUM_ITEM_TYPES; i++) {
            Position* temp = malloc(sizeof(Position) * GRID_SIZE * GRID_SIZE);
            if (temp == NULL) {
                newLevel.hasError = MALLOC_ALL_ITEMS_FAILED;
                break;
            }
            newLevel.allItems[i] = temp;
            newLevel.itemCounts[i] = 0; // the count of each item type is initialized to 0
        }
    }
    return newLevel;
}

void freeLevel(Level* level) {
    for (int i = 0; i < NUM_ENEMY_TYPES; i++) {
        free(level->allEnemies[i]);
    }
    free(level->allEnemies);

    for (int i = 0; i < NUM_ITEM_TYPES; i++) {
        free(level->allItems[i]);
    }
    free(level->allItems);

    free(level->itemCounts);
    free(level->enemyCounts);
    free(level->walls);
}

void setCursorPosition(int x, int y) {
    const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = { x, y };
    SetConsoleCursorPosition(hOut, coord);
}

int menuSelect(const char* choiceList[], int numChoices, Position cursorPos) {
    int currentChoice = 0;

    while (true) {
        setCursorPosition(cursorPos.x, cursorPos.y);

        // print an arrow beside the player's currently selected option
        for (int i = 0; i < numChoices; i++) {
            if (i == currentChoice) {
                printf(" --> ");
            }
            else {
                printf("     ");
            }
            printf("%s\n", choiceList[i]);
        }

        switch (_getch()) {
        case 'W':
        case 'w':
        case 72: // up arrow key
            if (currentChoice > 0) {
                currentChoice--;
            }
            else {
                currentChoice = numChoices - 1; // loop to the bottom
            }
            break;
        case 'S':
        case 's':
        case 80: // down arrow key
            if (currentChoice < numChoices - 1) {
                currentChoice++;
            }
            else {
                currentChoice = 0; // loop to the top
            }
            break;
        case 13: // enter key
            return currentChoice;
        }
    }
}

void levelSelect(void) {
    system("cls");
    printf("This is the level selection menu!\n");

    while (_getch() != '\r');
}

void credits(void) {
    system("cls");
    printf("===================== CREDITS =====================\n\n");
    printf("                  Made by: me\n");
    printf("               Created by: me\n");
    printf("             Developed by: me\n");
    printf("              Produced by: me\n");
    printf("Brought into existence by: me\n");
    printf("              Designed by: me\n");
    printf("               Modeled by: me\n");
    printf("                Forged by: me\n");
    printf("            Programmed by: me\n");
    printf("             Assembled by: me\n");
    printf("          Manufactured by: me\n");
    printf("                Formed by: me\n");
    printf("               Planned by: me\n");
    printf("                Tested by: me\n");
    printf("\n===================================================\n");
    printf("\n --> Return to Main Menu");
        
    while (_getch() != '\r'); // user remains on the credits screen until the enter key is pressed    
}

int mainMenuSequence(void) {
    int selection;
    while (true) {
        system("cls");
        printf("======= MAIN MENU =======\n\n");
        selection = menuSelect(mainMenu, sizeof(mainMenu) / sizeof(char*), mainMenuCursor);
        switch (selection) {
        case 0:
            return selection;
        case 1:
            levelSelect();
            break;
        case 2:
            credits();
            break;
        case 3:
            return selection;
        }
    }
}

void printErrorMessage(ErrorCode err, int level) {
    system("cls");
    fprintf(stderr, "FATAL EXCEPTION ERROR: An error occurred while processing level %d.\n\n", level);
    fprintf(stderr, "Error code %d: ", err);
    switch (err) {
    case ERROR_OPENING_FILE:
        fprintf(stderr, "The level file could not be opened.\n");
        break;
    case UNKNOWN_ENEMY_TYPE:
        fprintf(stderr, "Unknown enemy type present in file.\n");
        break;
    case NO_PLAYER_EXISTS:
        fprintf(stderr, "No starting location for the player in the level is defined.\n");
        break;
    case NO_EXIT_EXISTS:
        fprintf(stderr, "No location objective in the level is defined.\n");
        break;
    case MALLOC_WALL_POSITION_FAILED:
        fprintf(stderr, "Memory allocation for wall position parsing failed.\n");
        break;
    case MALLOC_ENEMY_POSITION_FAILED:
        fprintf(stderr, "Memory allocation for enemy position parsing failed.\n");
        break;
    case MALLOC_ENEMY_COUNT_FAILED:
        fprintf(stderr, "Memory allocation for enemy count parsing failed.\n");
        break;
    case REALLOC_WALL_POSITION_FAILED:
        fprintf(stderr, "Memory reallocation for wall position array failed.\n");
        break;
    case REALLOC_ENEMY_COUNT_FAILED:
        fprintf(stderr, "Memory reallocation for enemy count array failed.\n");
        break;
    case MALLOC_PLAYER_LAYER_FAILED:
        fprintf(stderr, "Memory allocation for the player/enemy layer failed.\n");
        break;
    case MALLOC_ITEM_LAYER_FAILED:
        fprintf(stderr, "Memory allocation for the item layer failed.\n");
        break;
    case MALLOC_WALL_LAYER_FAILED:
        fprintf(stderr, "Memory allocation for the wall layer failed.\n");
        break;
    case MALLOC_ALL_ENEMIES_FAILED:
        fprintf(stderr, "Memory allocation for the enemy matrix failed.\n");
        break;
    case MALLOC_ALL_ITEMS_FAILED:
        fprintf(stderr, "Memory allocation for all items failed.\n");
        break;
    case MALLOC_ITEM_COUNT_FAILED:
        fprintf(stderr, "Memory allocation for item count array failed.\n");
        break;
    }
}

void printObjective(ObjectiveType ID, HANDLE hConsole, int level) {

    // print the appropriate objective message
    if (ID == EXIT_OBJ) {
        printf("GOAL: Reach the ");
        SetConsoleTextAttribute(hConsole, LIGHT_GREEN);
        printf("E");
        SetConsoleTextAttribute(hConsole, WHITE);
        printf("!\n");
    }
    else {
        printf("GOAL: Collect all ");
        SetConsoleTextAttribute(hConsole, LIGHT_BLUE);
        printf("!");
        SetConsoleTextAttribute(hConsole, WHITE);
        printf(" and reach the ");
        SetConsoleTextAttribute(hConsole, LIGHT_GREEN);
        printf("E");
        SetConsoleTextAttribute(hConsole, WHITE);
        printf("!\n");
    }

    // print the current level that the player is on
    SetConsoleTextAttribute(hConsole, YELLOW);
    printf("\tLEVEL %d\n", level);
    SetConsoleTextAttribute(hConsole, WHITE);
}

int main(void) {
    srand(time(NULL));
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // used to change the color of text

    while (true) {

        // print the main menu screen: the infinite loop will terminate if the user quits
        if (mainMenuSequence() == 3) {
            printf("\nAre you sure you want to quit?\n");
            if (menuSelect(quitMenu, sizeof(quitMenu) / sizeof(char*), (Position) { 0, 9 }) == 1) {
                break;
            }
            else continue;
        }

        // iterate thru each level (1-based indices, since the 0th index is the game over screen)
        for (int i = 1; i < 30; i++) {
            bool backToMainFlag = false;
            system("cls");

            // determine if the level file has any errors before continuing to load it into the game
            Level level = parseLevelLayout(allLevelFiles[i]);
            if (level.hasError) {
                printErrorMessage(level.hasError, i);
                freeLevel(&level);
                return level.hasError;
            }

            // initialize all entities based on the level file
            GameBoard game = initializeGameBoard(level, 1);
            if (game.hasError) {
                printErrorMessage(game.hasError, i);
                freeLevel(&level);
                freeGameBoard(&game);
                return game.hasError;
            }

            printObjective(level.objectiveID, hConsole, i);

            // begin the game: this function returns true if the game is won, and false if lost
            if (gameLoop(&level, &game)) {

                // add a game win screen somewhere here ...

                // prompt the player to either continue or return to the main menu upon successfully finishing the level
                printf("\t\nLevel cleared!\n");
                if (menuSelect(continueMenu, sizeof(continueMenu) / sizeof(char*), postlevelCursor) == 1) {
                    backToMainFlag = true;
                }
            }
            else { // print the game over screen from the text file if the player loses
                system("cls");
                Level gameOver = parseLevelLayout(allLevelFiles[0]);
                GameBoard gameOverScreen = initializeGameBoard(gameOver, 0);
                drawGameState(gameOverScreen.grid, gameOver);

                // prompt the player to either restart the level or return to the main menu
                printf("\nYou got caught!\n");
                if (menuSelect(gameOverMenu, sizeof(gameOverMenu) / sizeof(char*), postlevelCursor) == 1) {
                    backToMainFlag = true;
                }
                else i--; // restart the level by decrementing the index (since the for-loop increments it every iteration)

                freeLevel(&gameOver);
                freeGameBoard(&gameOverScreen);
            }

            // clean up the memory allocated to parse the level files and the game board
            freeLevel(&level);
            freeGameBoard(&game);

            // break out of the level-traversal loop to return to the infinite loop
            if (backToMainFlag) break;
        }
    }
    _CrtDumpMemoryLeaks();
    return 0;
}