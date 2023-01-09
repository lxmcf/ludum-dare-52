//----------------------------------------------------------------------------------
// NOTE!
//
// Please forgive my awful code, the whole idea of this was to bash it out quickly
// and not get bogged down in pretty code or massive optimisations.
//
// Mamaaa, Just killed a thread...
//----------------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

//----------------------------------------------------------------------------------
// Game config
//----------------------------------------------------------------------------------
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const char* WINDOW_TITLE = "Rotten Harvest";

const int VIRTUAL_SCALE = 3;
const int VIRTUAL_WIDTH = WINDOW_WIDTH / VIRTUAL_SCALE;
const int VIRTUAL_HEIGHT = WINDOW_HEIGHT / VIRTUAL_SCALE;
const float VIRTUAL_RATIO = (float)VIRTUAL_WIDTH / (float)VIRTUAL_HEIGHT;

#define TARGET_FPS 60
#define PLAYER_SPEED 100

#define GROUND_GREEN CLITERAL(Color){ 139, 147, 64, 255 }
#define MAX_TEXTURES 32
#define MAX_SOUNDS 4
#define MAX_COLLIDERS 12
#define MAX_PLOTS 20

#define FALSE 0
#define TRUE 1

//----------------------------------------------------------------------------------
// Enums
//----------------------------------------------------------------------------------
typedef enum {
    BEETROOT,
    CARROT,
    CORN,
    TOMATO,
    TURNIP,
    NONE
} RH_Crop;

typedef enum {
    IDLE,
    MOVING
} RH_PlayerState;

typedef enum {
    RIGHT,
    UP,
    LEFT,
    DOWN
} RH_Directon;

//----------------------------------------------------------------------------------
// Structs
//----------------------------------------------------------------------------------
typedef struct TextureSheet {
    Texture texture;

    int frames;

    int frameWidth;
    int frameHeight;

    Vector2 origin;
} TextureSheet;

typedef struct FarmPlot {
    Vector2 position;
    RH_Crop currentCrop;

    float cropTimer;
    float cropTimerMax;

    float cropGrowthRate;

    int interactable;

    int watered;
    int purchased;
} FarmPlot;

//----------------------------------------------------------------------------------
// Variables
//----------------------------------------------------------------------------------
static Camera2D worldSpaceCamera = { 0 };
static Camera2D screenSpaceCamera = { 0 };

static RenderTexture2D targetTexture;

static Rectangle sourceRectangle;
static Rectangle destinationRectangle;

// Random Values
static int barnColour;

// Texture ID's
static int walkLeft;
static int walkRight;
static int walkUp;
static int walkDown;

static int idleLeft;
static int idleRight;
static int idleUp;
static int idleDown;

static int cropSeed;
static int cropBeetroot;
static int cropCarrot;
static int cropCorn;
static int cropTomato;
static int cropTurnip;

static int barn;

static int farmPlot;

static int interact;

// Sound ID's
static int pickupCoin;

// Player Animations
static int walkingAnimation[4];
static int idleAnimation[4];
static int currentAnimation = DOWN;
static float currentFrame = 0.0f;

// Player Values
static Vector2 position;
static RH_PlayerState state;

static int totalCoins;

static int beetrootSeeds;
static int carrotSeeds;
static int cornSeeds;
static int tomatoSeeds;
static int turnipSeeds;

// Asset Pools
static TextureSheet texturePool[MAX_TEXTURES];
static int textureCount;

static Sound soundPool[MAX_SOUNDS];
static int soundCount;

// Collisions
static Rectangle collisions[MAX_COLLIDERS];
static int collisionsCount;

// Plots
static FarmPlot farmPlots[MAX_PLOTS];
static int farmPlotsCount;

static int farmPlotsCrops[5];

//----------------------------------------------------------------------------------
// Function definition
//----------------------------------------------------------------------------------
// Textures
int LoadTexturesheet (const char* filename, int frames, Vector2 origin);
void DrawTexturesheet (int textureID, Vector2 position, int frame);
void UnloadTexturesheet (TextureSheet texture);

// Sounds
int LoadSoundFile (const char* filename);
void PlaySoundFile (int soundID);

// Farm Plot
int CreateFarmPlot (Vector2 position);

//----------------------------------------------------------------------------------
// Function decleration
//----------------------------------------------------------------------------------
// Textures
int LoadTexturesheet (const char* filename, int frames, Vector2 origin) {
    TextureSheet texture = { 0 };

    texture.texture = LoadTexture (filename);
    texture.frames = frames;
    texture.origin = origin;

    texture.frameWidth = texture.texture.width / texture.frames;
    texture.frameHeight = texture.texture.height;

    texturePool[textureCount] = texture;

    return textureCount++;
}

void DrawTexturesheet (int textureID, Vector2 position, int frame) {
    TextureSheet sheet = texturePool[textureID];

    int relative_frame = frame % sheet.frames;

    Rectangle texture_rectangle = { frame * sheet.frameWidth, 0, sheet.frameWidth, sheet.frameHeight };

    DrawTextureRec (sheet.texture, texture_rectangle, Vector2Subtract (position, sheet.origin), WHITE);
}

void UnloadTexturesheet (TextureSheet texture) {
    UnloadTexture (texture.texture);
}

// Sounds
int LoadSoundFile (const char* filename) {
    Sound sound = LoadSound (filename);

    soundPool[soundCount] = sound;

    return soundCount++;
}

void PlaySoundFile (int soundID) {
    PlaySound (soundPool[soundID]);
}

// Farm Plot
int CreateFarmPlot (Vector2 position) {
    FarmPlot plot = { 0 };

    plot.position = (Vector2) { position.x, position.y };
    plot.purchased = FALSE;
    plot.watered = FALSE;
    plot.interactable = FALSE;

    plot.cropGrowthRate = 0.1f;
    plot.cropTimer = 0.0f;
    plot.cropTimerMax = 3.0f;

    plot.currentCrop = NONE;

    farmPlots[farmPlotsCount] = plot;

    return farmPlotsCount++;
}

//----------------------------------------------------------------------------------
// Game functions
//----------------------------------------------------------------------------------
static void LoadGame (void);
static void InitGame (void);
static void UpdateGame (void);
static void DrawGame (void);
static void UnloadGame (void);

//----------------------------------------------------------------------------------
// Main loop
//----------------------------------------------------------------------------------
int main (int argc, const char* argv[]) {
    InitWindow (WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    InitAudioDevice ();

    LoadGame ();

    InitGame ();

    while (!WindowShouldClose ()) {
        UpdateGame ();

        BeginTextureMode (targetTexture);
            ClearBackground (GROUND_GREEN);

            BeginMode2D (worldSpaceCamera);
                DrawGame ();
            EndMode2D ();
        EndTextureMode ();

        BeginDrawing ();
            ClearBackground (RED);

            BeginMode2D (screenSpaceCamera);
                DrawTexturePro (targetTexture.texture, sourceRectangle, destinationRectangle, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
            EndMode2D ();
        EndDrawing ();
    }

    CloseWindow ();

    return 0;
}

static void LoadGame (void) {
    // Textures
    walkLeft = LoadTexturesheet ("data/sprites/player/walk_left.png", 6, (Vector2){ 7.0f, 16.0f });
    walkRight = LoadTexturesheet ("data/sprites/player/walk_right.png", 6, (Vector2){ 7.0f, 16.0f });
    walkUp = LoadTexturesheet ("data/sprites/player/walk_up.png", 6, (Vector2){ 7.0f, 16.0f });
    walkDown = LoadTexturesheet ("data/sprites/player/walk_down.png", 6, (Vector2){ 7.0f, 16.0f });

    idleLeft = LoadTexturesheet ("data/sprites/player/idle_left.png", 6, (Vector2){ 7.0f, 16.0f });
    idleRight = LoadTexturesheet ("data/sprites/player/idle_right.png", 6, (Vector2){ 7.0f, 16.0f });
    idleUp = LoadTexturesheet ("data/sprites/player/idle_up.png", 6, (Vector2){ 7.0f, 16.0f });
    idleDown = LoadTexturesheet ("data/sprites/player/idle_down.png", 6, (Vector2){ 7.0f, 16.0f });

    cropSeed = LoadTexturesheet ("data/sprites/crops/seed.png", 1, (Vector2){ 7.0f, 7.0f });
    cropBeetroot = LoadTexturesheet ("data/sprites/crops/beetroot.png", 3, (Vector2){ 8.0f, 14.0f });
    cropCarrot = LoadTexturesheet ("data/sprites/crops/carrot.png", 3, (Vector2){ 8.0f, 14.0f });
    cropCorn = LoadTexturesheet ("data/sprites/crops/corn.png", 3, (Vector2){ 8.0f, 14.0f });
    cropTomato = LoadTexturesheet ("data/sprites/crops/tomato.png", 3, (Vector2){ 8.0f, 14.0f });
    cropTurnip = LoadTexturesheet ("data/sprites/crops/turnip.png", 3, (Vector2){ 8.0f, 14.0f });

    farmPlotsCrops[BEETROOT] = cropBeetroot;
    farmPlotsCrops[CARROT] = cropCarrot;
    farmPlotsCrops[CORN] = cropCorn;
    farmPlotsCrops[TOMATO] = cropTomato;
    farmPlotsCrops[TURNIP] = cropTurnip;

    barn = LoadTexturesheet ("data/sprites/barns.png", 6, Vector2Zero ());
    farmPlot = LoadTexturesheet ("data/sprites/plot.png", 2, (Vector2){ 7.0f, 7.0f });
    interact = LoadTexturesheet ("data/sprites/interact.png", 1, (Vector2){ 6.0f, 12.0f });

    pickupCoin = LoadSoundFile ("data/sounds/coin.ogg");
}

static void InitGame (void) {
    SetTargetFPS (TARGET_FPS);

    // Target Textures
    targetTexture = LoadRenderTexture (VIRTUAL_WIDTH, VIRTUAL_HEIGHT);

    // Camera
    worldSpaceCamera.zoom = 1.0f;
    screenSpaceCamera.zoom = 1.0f;

    sourceRectangle = (Rectangle){ 0.0f, 0.0f, (float)targetTexture.texture.width, -(float)targetTexture.texture.height };
    destinationRectangle = (Rectangle){ -VIRTUAL_RATIO, -VIRTUAL_RATIO, WINDOW_WIDTH + (VIRTUAL_RATIO * 2), WINDOW_HEIGHT + (VIRTUAL_RATIO * 2) };

    walkingAnimation[UP] = walkUp;
    walkingAnimation[DOWN] = walkDown;
    walkingAnimation[LEFT] = walkLeft;
    walkingAnimation[RIGHT] = walkRight;

    idleAnimation[UP] = idleUp;
    idleAnimation[DOWN] = idleDown;
    idleAnimation[LEFT] = idleLeft;
    idleAnimation[RIGHT] = idleRight;

    barnColour = GetRandomValue (0, 5);

    // Player Values
    position = (Vector2){ VIRTUAL_WIDTH / 2, VIRTUAL_HEIGHT / 2 };
    state = IDLE;

    beetrootSeeds = 0;
    carrotSeeds = 0;
    cornSeeds = 0;
    tomatoSeeds = 0;
    turnipSeeds = 0;

    // Collisions
    collisions[collisionsCount] = (Rectangle){ 0, 0, texturePool[barn].frameWidth + 4, texturePool[barn].frameHeight + 2 };
    collisionsCount++;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 10; j++) {
            CreateFarmPlot ((Vector2){(14.0f * 16.0f) + (i * 16.0f), (j * 16.0f) + 24.0f });
        }
    }
}

static void UpdateGame (void) {
    int xaxis = IsKeyDown (KEY_D) - IsKeyDown (KEY_A);
    int yaxis = IsKeyDown (KEY_S) - IsKeyDown (KEY_W);

    if (xaxis != 0 || yaxis != 0) {
        Vector2 velocity = { (xaxis * PLAYER_SPEED) * GetFrameTime (), (yaxis * PLAYER_SPEED) * GetFrameTime () };

        Vector2 newPosition = Vector2Add (position, velocity);

        for (int i = 0; i < collisionsCount; i++) {
            if (!CheckCollisionPointRec(newPosition, collisions[i])) {
                position = newPosition;
            }
        }

        // Get needed animation
        float direction = Vector2Angle (Vector2Zero (), velocity) * RAD2DEG;
        direction = direction <= 0 ? fabsf (-direction) : 360.0f - direction;

        currentAnimation = (int)direction / 90;

        state = MOVING;
    } else {
        state = IDLE;
    }

    for (int i = 0; i < farmPlotsCount; i++) {
        // NOTE: Try to ignore this...
        farmPlots[i].interactable = Vector2Distance (position, farmPlots[i].position) < 8.0f
                                                    && (farmPlots[i].cropTimer >= farmPlots[i].cropTimerMax - 1.0f ||
                                                        farmPlots[i].currentCrop == NONE);

        if (farmPlots[i].interactable) {
            if (IsKeyPressed(KEY_E)) {
                if (farmPlots[i].cropTimer >= farmPlots[i].cropTimerMax - 1.0f) {
                    farmPlots[i].currentCrop = NONE;
                    farmPlots[i].cropTimer = 0.0f;

                    PlaySoundFile (pickupCoin);
                }
            }

            if (IsKeyPressed(KEY_ONE)) {
                farmPlots[i].currentCrop = BEETROOT;
            }

            if (IsKeyPressed(KEY_TWO)) {
                farmPlots[i].currentCrop = CARROT;
            }

            if (IsKeyPressed(KEY_THREE)) {
                farmPlots[i].currentCrop = CORN;
            }

            if (IsKeyPressed(KEY_FOUR)) {
                farmPlots[i].currentCrop = TOMATO;
            }

            if (IsKeyPressed(KEY_FIVE)) {
                farmPlots[i].currentCrop = TURNIP;
            }
        }

        if (farmPlots[i].currentCrop != NONE) {
            if (farmPlots[i].cropTimer < farmPlots[i].cropTimerMax) {
                farmPlots[i].cropTimer += GetFrameTime () * farmPlots[i].cropGrowthRate;
            }
        }
    }

    currentFrame += (GetFrameTime () * 10.0f);
}

static void DrawGame (void) {
    DrawTexturesheet (barn, Vector2Zero (), barnColour);

    for (int i = 0; i < farmPlotsCount; i++) {
        DrawTexturesheet (farmPlot, farmPlots[i].position, farmPlots[i].watered);

        if (farmPlots[i].currentCrop != NONE) {
            if (farmPlots[i].cropTimer < 1.0f) {
                DrawTexturesheet (cropSeed, farmPlots[i].position, 0);
            } else {
                DrawTexturesheet (farmPlotsCrops[farmPlots[i].currentCrop], farmPlots[i].position, (int)farmPlots[i].cropTimer - 1);
            }
        }

        if (farmPlots[i].interactable) {
            DrawTexturesheet (interact, Vector2Subtract (position, (Vector2){ 0.0f, 18.0f }), 0);
        }
    }

    if (state == IDLE) {
        DrawTexturesheet (idleAnimation[currentAnimation], position, (int)currentFrame);
    } else {
        DrawTexturesheet (walkingAnimation[currentAnimation], position, (int)currentFrame);
    }
}

static void UnloadGame (void) {
    UnloadRenderTexture (targetTexture);

    for (int i = 0; i < textureCount; i++) {
        UnloadTexturesheet (texturePool[i]);
    }
}
