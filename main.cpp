#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// ============================================================
// ELEMENTAL MAGE 3D
// Raylib - Standalone C++
// Mobile Multi-Touch + 3D Arena + Spells + Enemies
// ============================================================

enum ElementType
{
    ELEM_FIRE = 0,
    ELEM_ICE,
    ELEM_EARTH,
    ELEM_WIND,
    ELEM_WATER
};

// ============================================================
// PARTICLE
// ============================================================

struct Particle3D
{
    Vector3 pos;
    Vector3 vel;

    Color color;

    float size;
    float life;
    float maxLife;

    bool additive;
};

// ============================================================
// SPELL
// ============================================================

struct Spell3D
{
    Vector3 pos;
    Vector3 vel;

    ElementType type;

    float radius;
    float damage;

    float life;
    float maxLife;

    bool active;
    bool hasHit;
};

// ============================================================
// ENEMY
// ============================================================

struct Enemy3D
{
    Vector3 pos;

    float hp;
    float maxHp;

    float speed;
    float radius;

    int type;

    bool active;

    float hitTimer;
    float freezeTimer;
    float attackCooldown;

    float rotation;
};

// ============================================================
// GROUND EFFECT
// ============================================================

struct GroundEffect3D
{
    Vector3 pos;

    float radius;
    float maxRadius;

    float life;
    float maxLife;

    Color color;

    ElementType type;
};

// ============================================================
// SHOCKWAVE
// ============================================================

struct Shockwave3D
{
    Vector3 pos;

    float radius;
    float maxRadius;

    float life;
    float maxLife;

    Color color;
};

// ============================================================
// DAMAGE TEXT
// ============================================================

struct DamageText3D
{
    Vector3 pos;

    int damage;

    Color color;

    float life;
};

// ============================================================
// TOUCH SYSTEM
// ============================================================

static const int MAX_TOUCH_IDS = 16;

int joystickTouchId = -1;
int attackTouchId = -1;

int previousTouchIds[MAX_TOUCH_IDS];
int previousTouchCount = 0;

bool IsTouchIdDown(int id)
{
    if (id < 0)
        return false;

    int count = GetTouchPointCount();

    for (int i = 0; i < count; i++)
    {
        if (GetTouchPointId(i) == id)
            return true;
    }

    return false;
}

bool IsNewTouch(int id)
{
    for (int i = 0; i < previousTouchCount; i++)
    {
        if (previousTouchIds[i] == id)
            return false;
    }

    return true;
}

Vector2 GetTouchById(int id)
{
    int count = GetTouchPointCount();

    for (int i = 0; i < count; i++)
    {
        if (GetTouchPointId(i) == id)
            return GetTouchPosition(i);
    }

    return { -99999.0f, -99999.0f };
}

// ============================================================
// COLOR HELPERS
// ============================================================

Color ElementColor(ElementType type)
{
    switch (type)
    {
        case ELEM_FIRE:
            return { 255, 65, 25, 255 };

        case ELEM_ICE:
            return { 80, 210, 255, 255 };

        case ELEM_EARTH:
            return { 170, 110, 55, 255 };

        case ELEM_WIND:
            return { 100, 255, 170, 255 };

        case ELEM_WATER:
            return { 40, 130, 255, 255 };
    }

    return WHITE;
}

const char* ElementName(ElementType type)
{
    switch (type)
    {
        case ELEM_FIRE:  return "FIRE";
        case ELEM_ICE:   return "ICE";
        case ELEM_EARTH: return "EARTH";
        case ELEM_WIND:  return "WIND";
        case ELEM_WATER: return "WATER";
    }

    return "UNKNOWN";
}

// ============================================================
// ADD PARTICLE
// ============================================================

void AddParticle(
    std::vector<Particle3D>& particles,
    Vector3 pos,
    Vector3 vel,
    Color color,
    float size,
    float life,
    bool additive = true)
{
    Particle3D p;

    p.pos = pos;
    p.vel = vel;

    p.color = color;

    p.size = size;

    p.life = 0.0f;
    p.maxLife = life;

    p.additive = additive;

    particles.push_back(p);
}

// ============================================================
// ADD EXPLOSION
// ============================================================

void AddExplosion(
    std::vector<Particle3D>& particles,
    Vector3 pos,
    Color color,
    int amount,
    float power)
{
    for (int i = 0; i < amount; i++)
    {
        float a = ((float)GetRandomValue(0, 359)) * DEG2RAD;

        float up = (float)GetRandomValue(-60, 100);

        float speed =
            (float)GetRandomValue(
                (int)(power * 0.4f),
                (int)power
            );

        Vector3 dir =
        {
            cosf(a) * speed,
            up,
            sinf(a) * speed
        };

        AddParticle(
            particles,
            pos,
            dir,
            color,
            (float)GetRandomValue(3, 9),
            0.45f + (float)GetRandomValue(0, 30) / 100.0f
        );
    }
}

// ============================================================
// DRAW MAGIC CIRCLE
// ============================================================

void DrawMagicCircle(
    Vector3 pos,
    float radius,
    Color color,
    float rotation)
{
    DrawCircle3D(
        pos,
        radius,
        { 1, 0, 0 },
        90.0f,
        ColorAlpha(color, 0.18f)
    );

    DrawCircle3D(
        pos,
        radius,
        { 1, 0, 0 },
        90.0f,
        ColorAlpha(color, 0.55f)
    );

    for (int i = 0; i < 4; i++)
    {
        float a =
            rotation +
            i * PI / 2.0f;

        Vector3 p1 =
        {
            pos.x + cosf(a) * radius,
            pos.y + 0.04f,
            pos.z + sinf(a) * radius
        };

        Vector3 p2 =
        {
            pos.x + cosf(a + PI) * radius,
            pos.y + 0.04f,
            pos.z + sinf(a + PI) * radius
        };

        DrawLine3D(
            p1,
            p2,
            ColorAlpha(color, 0.35f)
        );
    }
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    // --------------------------------------------------------
    // FULLSCREEN WINDOW
    // --------------------------------------------------------

    SetConfigFlags(
        FLAG_WINDOW_RESIZABLE |
        FLAG_MSAA_4X_HINT
    );

    InitWindow(
        1280,
        720,
        "Elemental Mage 3D"
    );

    SetTargetFPS(60);

    // --------------------------------------------------------
    // SCREEN
    // --------------------------------------------------------

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // --------------------------------------------------------
    // CAMERA
    // --------------------------------------------------------

    Camera3D camera = { 0 };

    camera.position =
    {
        0.0f,
        9.5f,
        10.5f
    };

    camera.target =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camera.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camera.fovy = 60.0f;

    camera.projection = CAMERA_PERSPECTIVE;

    // --------------------------------------------------------
    // PLAYER
    // --------------------------------------------------------

    Vector3 playerPos =
    {
        0.0f,
        1.0f,
        0.0f
    };

    Vector3 playerFacing =
    {
        0.0f,
        0.0f,
        -1.0f
    };

    float playerSpeed = 7.0f;

    float playerHP = 100.0f;
    float playerMaxHP = 100.0f;

    float playerMana = 100.0f;
    float playerMaxMana = 100.0f;

    // --------------------------------------------------------
    // ARENA
    // --------------------------------------------------------

    const float ARENA_SIZE = 70.0f;

    // --------------------------------------------------------
    // ELEMENT
    // --------------------------------------------------------

    ElementType currentElement = ELEM_FIRE;

    // --------------------------------------------------------
    // GAME
    // --------------------------------------------------------

    std::vector<Particle3D> particles;
    std::vector<Spell3D> spells;
    std::vector<Enemy3D> enemies;
    std::vector<GroundEffect3D> groundEffects;
    std::vector<Shockwave3D> shockwaves;
    std::vector<DamageText3D> damageTexts;

    int score = 0;

    int wave = 1;

    float waveTimer = 0.0f;

    const float WAVE_TIME = 25.0f;

    bool gameOver = false;

    // --------------------------------------------------------
    // SPAWN
    // --------------------------------------------------------

    float spawnTimer = 0.0f;

    // --------------------------------------------------------
    // ATTACK
    // --------------------------------------------------------

    float castCooldown = 0.0f;

    // سريع جداً لكن ليس مجنوناً
    const float CAST_INTERVAL = 0.10f;

    // --------------------------------------------------------
    // EFFECTS
    // --------------------------------------------------------

    float screenShake = 0.0f;

    float gameTime = 0.0f;

    // --------------------------------------------------------
    // JOYSTICK
    // --------------------------------------------------------

    Vector2 stickCenter =
    {
        screenWidth * 0.14f,
        screenHeight * 0.78f
    };

    float stickRadius =
        screenHeight * 0.115f;

    float knobRadius =
        screenHeight * 0.048f;

    Vector2 knobPos = stickCenter;

    Vector2 moveDir2D = { 0, 0 };

    // --------------------------------------------------------
    // BUTTONS
    // --------------------------------------------------------

    Rectangle attackButton =
    {
        screenWidth * 0.80f,
        screenHeight * 0.70f,
        screenWidth * 0.14f,
        screenHeight * 0.18f
    };

    float elementButtonWidth =
        screenWidth * 0.115f;

    float elementButtonHeight =
        screenHeight * 0.075f;

    Rectangle elementButtons[5];

    for (int i = 0; i < 5; i++)
    {
        elementButtons[i] =
        {
            screenWidth * 0.025f +
            i * (elementButtonWidth + screenWidth * 0.012f),

            screenHeight * 0.035f,

            elementButtonWidth,
            elementButtonHeight
        };
    }

    // ========================================================
    // MAIN LOOP
    // ========================================================

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        if (dt > 0.05f)
            dt = 0.05f;

        gameTime += dt;

        // ====================================================
        // RESIZE UI
        // ====================================================

        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

        stickCenter =
        {
            screenWidth * 0.14f,
            screenHeight * 0.78f
        };

        stickRadius =
            screenHeight * 0.115f;

        knobRadius =
            screenHeight * 0.048f;

        attackButton =
        {
            screenWidth * 0.80f,
            screenHeight * 0.70f,
            screenWidth * 0.14f,
            screenHeight * 0.18f
        };

        elementButtonWidth =
            screenWidth * 0.115f;

        elementButtonHeight =
            screenHeight * 0.075f;

        for (int i = 0; i < 5; i++)
        {
            elementButtons[i] =
            {
                screenWidth * 0.025f +
                i * (elementButtonWidth + screenWidth * 0.012f),

                screenHeight * 0.035f,

                elementButtonWidth,
                elementButtonHeight
            };
        }

        // ====================================================
        // GAME
        // ====================================================

        if (!gameOver)
        {
            // ------------------------------------------------
            // MANA
            // ------------------------------------------------

            playerMana += 20.0f * dt;

            if (playerMana > playerMaxMana)
                playerMana = playerMaxMana;

            // ------------------------------------------------
            // COOLDOWN
            // ------------------------------------------------

            if (castCooldown > 0.0f)
            {
                castCooldown -= dt;

                if (castCooldown < 0.0f)
                    castCooldown = 0.0f;
            }

            // ------------------------------------------------
            // WAVE
            // ------------------------------------------------

            waveTimer += dt;

            if (waveTimer >= WAVE_TIME)
            {
                waveTimer = 0.0f;
                wave++;
            }

            // =================================================
            // MOVEMENT
            // =================================================

            moveDir2D = { 0, 0 };

            bool joystickActive = false;

            int touchCount = GetTouchPointCount();

            // -------------------------------------------------
            // NEW TOUCHES
            // -------------------------------------------------

            for (int i = 0; i < touchCount; i++)
            {
                int id = GetTouchPointId(i);

                Vector2 pos =
                    GetTouchPosition(i);

                if (!IsNewTouch(id))
                    continue;

                // ---------------------------------------------
                // ELEMENT BUTTONS
                // ---------------------------------------------

                for (int e = 0; e < 5; e++)
                {
                    if (CheckCollisionPointRec(
                            pos,
                            elementButtons[e]))
                    {
                        currentElement =
                            (ElementType)e;
                    }
                }

                // ---------------------------------------------
                // JOYSTICK
                // ---------------------------------------------

                if (joystickTouchId == -1)
                {
                    if (CheckCollisionPointCircle(
                            pos,
                            stickCenter,
                            stickRadius * 1.65f))
                    {
                        joystickTouchId = id;
                    }
                }

                // ---------------------------------------------
                // ATTACK
                // ---------------------------------------------

                if (attackTouchId == -1)
                {
                    if (CheckCollisionPointRec(
                            pos,
                            attackButton))
                    {
                        attackTouchId = id;
                    }
                }
            }

            // ------------------------------------------------
            // REMOVE RELEASED TOUCHES
            // ------------------------------------------------

            if (joystickTouchId != -1)
            {
                if (!IsTouchIdDown(joystickTouchId))
                {
                    joystickTouchId = -1;

                    knobPos =
                        stickCenter;
                }
            }

            if (attackTouchId != -1)
            {
                if (!IsTouchIdDown(attackTouchId))
                {
                    attackTouchId = -1;
                }
            }

            // =================================================
            // JOYSTICK CONTINUES OUTSIDE CIRCLE
            // =================================================

            if (joystickTouchId != -1)
            {
                Vector2 touchPos =
                    GetTouchById(
                        joystickTouchId
                    );

                if (touchPos.x > -90000.0f)
                {
                    Vector2 diff =
                        Vector2Subtract(
                            touchPos,
                            stickCenter
                        );

                    float len =
                        Vector2Length(diff);

                    if (len > stickRadius)
                    {
                        diff =
                            Vector2Scale(
                                Vector2Normalize(diff),
                                stickRadius
                            );
                    }

                    knobPos =
                        Vector2Add(
                            stickCenter,
                            diff
                        );

                    moveDir2D =
                        Vector2Scale(
                            diff,
                            1.0f / stickRadius
                        );

                    if (Vector2Length(moveDir2D) > 0.05f)
                    {
                        playerFacing =
                        {
                            moveDir2D.x,
                            0.0f,
                            moveDir2D.y
                        };

                        playerFacing =
                            Vector3Normalize(
                                playerFacing
                            );
                    }

                    joystickActive = true;
                }
            }

            // =================================================
            // KEYBOARD
            // =================================================

            if (IsKeyDown(KEY_W))
                moveDir2D.y -= 1.0f;

            if (IsKeyDown(KEY_S))
                moveDir2D.y += 1.0f;

            if (IsKeyDown(KEY_A))
                moveDir2D.x -= 1.0f;

            if (IsKeyDown(KEY_D))
                moveDir2D.x += 1.0f;

            if (Vector2Length(moveDir2D) > 1.0f)
            {
                moveDir2D =
                    Vector2Normalize(
                        moveDir2D
                    );
            }

            // =================================================
            // PLAYER MOVEMENT
            // =================================================

            Vector3 movement =
            {
                moveDir2D.x,
                0.0f,
                moveDir2D.y
            };

            playerPos =
                Vector3Add(
                    playerPos,
                    Vector3Scale(
                        movement,
                        playerSpeed * dt
                    )
                );

            // ------------------------------------------------
            // ARENA LIMIT
            // ------------------------------------------------

            float limit =
                ARENA_SIZE * 0.5f - 2.0f;

            playerPos.x =
                Clamp(
                    playerPos.x,
                    -limit,
                    limit
                );

            playerPos.z =
                Clamp(
                    playerPos.z,
                    -limit,
                    limit
                );

            // =================================================
            // ATTACK FUNCTION
            // =================================================

            bool attackHeld =
                attackTouchId != -1;

            // Keyboard attack
            if (IsKeyDown(KEY_SPACE))
                attackHeld = true;

            // =================================================
            // CAST
            // =================================================

            if (attackHeld &&
                castCooldown <= 0.0f &&
                playerMana >= 15.0f)
            {
                playerMana -= 15.0f;

                castCooldown =
                    CAST_INTERVAL;

                // ---------------------------------------------
                // AUTO AIM
                // ---------------------------------------------

                Vector3 targetDirection =
                    playerFacing;

                float nearestDistance =
                    25.0f;

                for (const auto& enemy : enemies)
                {
                    if (!enemy.active)
                        continue;

                    float distance =
                        Vector3Distance(
                            playerPos,
                            enemy.pos
                        );

                    if (distance < nearestDistance)
                    {
                        nearestDistance =
                            distance;

                        Vector3 dir =
                            Vector3Subtract(
                                enemy.pos,
                                playerPos
                            );

                        if (Vector3Length(dir) > 0.01f)
                        {
                            targetDirection =
                                Vector3Normalize(dir);
                        }
                    }
                }

                // ---------------------------------------------
                // SPELL
                // ---------------------------------------------

                Spell3D spell;

                spell.pos =
                    Vector3Add(
                        playerPos,
                        Vector3Scale(
                            targetDirection,
                            1.8f
                        )
                    );

                spell.type =
                    currentElement;

                spell.active = true;

                spell.hasHit = false;

                spell.life = 1.6f;

                spell.maxLife = 1.6f;

                // ---------------------------------------------
                // ELEMENT STATS
                // ---------------------------------------------

                switch (currentElement)
                {
                    case ELEM_FIRE:

                        spell.vel =
                            Vector3Scale(
                                targetDirection,
                                22.0f
                            );

                        spell.radius = 0.65f;

                        spell.damage = 45.0f;

                        spell.life = 1.6f;

                        screenShake = 0.10f;

                        break;

                    case ELEM_ICE:

                        spell.vel =
                            Vector3Scale(
                                targetDirection,
                                27.0f
                            );

                        spell.radius = 0.50f;

                        spell.damage = 30.0f;

                        spell.life = 1.4f;

                        break;

                    case ELEM_EARTH:

                        spell.vel =
                            Vector3Scale(
                                targetDirection,
                                17.0f
                            );

                        spell.radius = 0.85f;

                        spell.damage = 80.0f;

                        spell.life = 1.7f;

                        screenShake = 0.20f;

                        break;

                    case ELEM_WIND:

                        spell.vel =
                            Vector3Scale(
                                targetDirection,
                                34.0f
                            );

                        spell.radius = 0.45f;

                        spell.damage = 25.0f;

                        spell.life = 1.0f;

                        break;

                    case ELEM_WATER:

                        spell.vel =
                            Vector3Scale(
                                targetDirection,
                                24.0f
                            );

                        spell.radius = 0.70f;

                        spell.damage = 35.0f;

                        spell.life = 1.5f;

                        break;
                }

                spell.maxLife =
                    spell.life;

                spells.push_back(spell);

                // ---------------------------------------------
                // CAST PARTICLES
                // ---------------------------------------------

                Color c =
                    ElementColor(
                        currentElement
                    );

                AddExplosion(
                    particles,
                    spell.pos,
                    c,
                    12,
                    4.0f
                );
            }

            // =================================================
            // ENEMY SPAWNING
            // =================================================

            spawnTimer += dt;

            float spawnInterval =
                fmaxf(
                    0.35f,
                    1.2f -
                    (wave - 1) * 0.08f
                );

            int maxEnemies =
                15 +
                (wave - 1) * 3;

            if (spawnTimer >= spawnInterval &&
                (int)enemies.size() < maxEnemies)
            {
                spawnTimer = 0.0f;

                Enemy3D enemy;

                float angle =
                    (float)GetRandomValue(
                        0,
                        359
                    ) * DEG2RAD;

                float distance =
                    (float)GetRandomValue(
                        20,
                        30
                    );

                enemy.pos =
                {
                    playerPos.x +
                    cosf(angle) * distance,

                    1.0f,

                    playerPos.z +
                    sinf(angle) * distance
                };

                float limit =
                    ARENA_SIZE * 0.5f -
                    3.0f;

                enemy.pos.x =
                    Clamp(
                        enemy.pos.x,
                        -limit,
                        limit
                    );

                enemy.pos.z =
                    Clamp(
                        enemy.pos.z,
                        -limit,
                        limit
                    );

                enemy.active = true;

                enemy.hitTimer = 0.0f;

                enemy.freezeTimer = 0.0f;

                enemy.attackCooldown = 0.0f;

                enemy.rotation = 0.0f;

                enemy.type =
                    GetRandomValue(0, 2);

                float waveMultiplier =
                    1.0f +
                    (wave - 1) * 0.12f;

                if (enemy.type == 0)
                {
                    // FAST
                    enemy.maxHp =
                        60.0f *
                        waveMultiplier;

                    enemy.speed =
                        4.5f +
                        wave * 0.08f;

                    enemy.radius = 0.8f;
                }
                else if (enemy.type == 1)
                {
                    // GOLEM
                    enemy.maxHp =
                        180.0f *
                        waveMultiplier;

                    enemy.speed =
                        2.3f +
                        wave * 0.04f;

                    enemy.radius = 1.25f;
                }
                else
                {
                    // SHADOW
                    enemy.maxHp =
                        90.0f *
                        waveMultiplier;

                    enemy.speed =
                        3.5f +
                        wave * 0.06f;

                    enemy.radius = 0.95f;
                }

                enemy.hp =
                    enemy.maxHp;

                enemies.push_back(enemy);
            }

            // =================================================
            // ENEMY AI
            // =================================================

            for (auto& enemy : enemies)
            {
                if (!enemy.active)
                    continue;

                if (enemy.hitTimer > 0.0f)
                    enemy.hitTimer -= dt;

                if (enemy.attackCooldown > 0.0f)
                    enemy.attackCooldown -= dt;

                if (enemy.freezeTimer > 0.0f)
                {
                    enemy.freezeTimer -= dt;

                    if (GetRandomValue(0, 5) == 0)
                    {
                        AddParticle(
                            particles,
                            enemy.pos,
                            {
                                (float)GetRandomValue(-20, 20),
                                3.0f,
                                (float)GetRandomValue(-20, 20)
                            },
                            SKYBLUE,
                            0.10f,
                            0.35f
                        );
                    }

                    continue;
                }

                Vector3 toPlayer =
                    Vector3Subtract(
                        playerPos,
                        enemy.pos
                    );

                float distance =
                    Vector3Length(
                        toPlayer
                    );

                if (distance > 2.0f)
                {
                    Vector3 direction =
                        Vector3Normalize(
                            toPlayer
                        );

                    enemy.pos =
                        Vector3Add(
                            enemy.pos,
                            Vector3Scale(
                                direction,
                                enemy.speed * dt
                            )
                        );

                    enemy.rotation +=
                        dt * 90.0f;
                }

                // ------------------------------------------------
                // DAMAGE PLAYER
                // ------------------------------------------------

                if (distance <
                    enemy.radius + 1.0f)
                {
                    if (enemy.attackCooldown <= 0.0f)
                    {
                        playerHP -= 12.0f;

                        enemy.attackCooldown =
                            0.6f;

                        screenShake =
                            0.12f;

                        AddExplosion(
                            particles,
                            playerPos,
                            RED,
                            8,
                            3.0f
                        );

                        if (playerHP <= 0.0f)
                        {
                            playerHP = 0.0f;

                            gameOver = true;
                        }
                    }
                }
            }

            // =================================================
            // SPELL PHYSICS
            // =================================================

            for (auto& spell : spells)
            {
                if (!spell.active)
                    continue;

                spell.pos =
                    Vector3Add(
                        spell.pos,
                        Vector3Scale(
                            spell.vel,
                            dt
                        )
                    );

                spell.life -= dt;

                if (spell.life <= 0.0f)
                {
                    spell.active = false;
                    continue;
                }

                Color spellColor =
                    ElementColor(
                        spell.type
                    );

                // ------------------------------------------------
                // TRAILS
                // ------------------------------------------------

                int trailAmount = 1;

                if (spell.type == ELEM_FIRE)
                    trailAmount = 3;

                if (spell.type == ELEM_WIND)
                    trailAmount = 2;

                for (int i = 0; i < trailAmount; i++)
                {
                    Vector3 trail =
                    {
                        spell.pos.x +
                        (float)GetRandomValue(-20, 20) * 0.01f,

                        spell.pos.y +
                        (float)GetRandomValue(-20, 20) * 0.01f,

                        spell.pos.z +
                        (float)GetRandomValue(-20, 20) * 0.01f
                    };

                    AddParticle(
                        particles,
                        trail,
                        {
                            (float)GetRandomValue(-10, 10),
                            (float)GetRandomValue(-10, 10),
                            (float)GetRandomValue(-10, 10)
                        },
                        spellColor,
                        0.08f +
                        (float)GetRandomValue(0, 8) * 0.01f,
                        0.22f,
                        true
                    );
                }

                // =================================================
                // COLLISION
                // =================================================

                for (auto& enemy : enemies)
                {
                    if (!enemy.active)
                        continue;

                    float distance =
                        Vector3Distance(
                            spell.pos,
                            enemy.pos
                        );

                    if (distance <
                        spell.radius +
                        enemy.radius)
                    {
                        if (spell.hasHit)
                            break;

                        spell.hasHit = true;

                        enemy.hp -=
                            spell.damage;

                        enemy.hitTimer =
                            0.18f;

                        // ------------------------------------------------
                        // ICE
                        // ------------------------------------------------

                        if (spell.type == ELEM_ICE)
                        {
                            enemy.freezeTimer =
                                1.8f;
                        }

                        // ------------------------------------------------
                        // EARTH PUSH
                        // ------------------------------------------------

                        if (spell.type ==
                            ELEM_EARTH)
                        {
                            Vector3 push =
                                Vector3Normalize(
                                    Vector3Subtract(
                                        enemy.pos,
                                        playerPos
                                    )
                                );

                            enemy.pos =
                                Vector3Add(
                                    enemy.pos,
                                    Vector3Scale(
                                        push,
                                        2.2f
                                    )
                                );
                        }

                        // ------------------------------------------------
                        // DAMAGE TEXT
                        // ------------------------------------------------

                        DamageText3D text;

                        text.pos =
                            enemy.pos;

                        text.damage =
                            (int)spell.damage;

                        text.color =
                            spellColor;

                        text.life = 0.7f;

                        damageTexts.push_back(
                            text
                        );

                        // ------------------------------------------------
                        // SHOCKWAVE
                        // ------------------------------------------------

                        Shockwave3D shock;

                        shock.pos =
                            enemy.pos;

                        shock.radius = 0.0f;

                        shock.maxRadius =
                            enemy.radius * 2.2f;

                        shock.life = 0.3f;

                        shock.maxLife = 0.3f;

                        shock.color =
                            spellColor;

                        shockwaves.push_back(
                            shock
                        );

                        // ------------------------------------------------
                        // HIT PARTICLES
                        // ------------------------------------------------

                        AddExplosion(
                            particles,
                            enemy.pos,
                            spellColor,
                            14,
                            7.0f
                        );

                        // ------------------------------------------------
                        // DEATH
                        // ------------------------------------------------

                        if (enemy.hp <= 0.0f)
                        {
                            enemy.active =
                                false;

                            score +=
                                100 +
                                wave * 10;

                            AddExplosion(
                                particles,
                                enemy.pos,
                                spellColor,
                                32,
                                12.0f
                            );

                            Shockwave3D deathShock;

                            deathShock.pos =
                                enemy.pos;

                            deathShock.radius =
                                0.0f;

                            deathShock.maxRadius =
                                4.0f;

                            deathShock.life =
                                0.45f;

                            deathShock.maxLife =
                                0.45f;

                            deathShock.color =
                                spellColor;

                            shockwaves.push_back(
                                deathShock
                            );
                        }

                        // -----------------------------------------
                        // Projectile stops after impact
                        // -----------------------------------------

                        spell.active =
                            false;

                        break;
                    }
                }
            }
        }
        else
        {
            // =================================================
            // GAME OVER
            // =================================================

            bool restart =
                IsMouseButtonPressed(
                    MOUSE_BUTTON_LEFT
                );

            if (GetTouchPointCount() > 0)
                restart = true;

            if (restart)
            {
                playerPos =
                {
                    0,
                    1,
                    0
                };

                playerHP =
                    playerMaxHP;

                playerMana =
                    playerMaxMana;

                score = 0;

                wave = 1;

                waveTimer = 0.0f;

                spawnTimer = 0.0f;

                castCooldown = 0.0f;

                joystickTouchId = -1;

                attackTouchId = -1;

                enemies.clear();

                spells.clear();

                particles.clear();

                groundEffects.clear();

                shockwaves.clear();

                damageTexts.clear();

                gameOver = false;
            }
        }

        // ====================================================
        // PARTICLES UPDATE
        // ====================================================

        for (size_t i = 0;
             i < particles.size();)
        {
            Particle3D& p =
                particles[i];

            p.pos =
                Vector3Add(
                    p.pos,
                    Vector3Scale(
                        p.vel,
                        dt
                    )
                );

            // Gravity
            p.vel.y -=
                5.0f * dt;

            p.life += dt;

            float lifeRatio =
                p.life / p.maxLife;

            p.size *=
                1.0f - 0.9f * dt;

            if (p.life >= p.maxLife)
            {
                particles.erase(
                    particles.begin() + i
                );
            }
            else
            {
                i++;
            }
        }

        // ====================================================
        // SHOCKWAVES
        // ====================================================

        for (size_t i = 0;
             i < shockwaves.size();)
        {
            shockwaves[i].life -= dt;

            float progress =
                1.0f -
                shockwaves[i].life /
                shockwaves[i].maxLife;

            shockwaves[i].radius =
                shockwaves[i].maxRadius *
                progress;

            if (shockwaves[i].life <= 0.0f)
            {
                shockwaves.erase(
                    shockwaves.begin() + i
                );
            }
            else
            {
                i++;
            }
        }

        // ====================================================
        // DAMAGE TEXT
        // ====================================================

        for (size_t i = 0;
             i < damageTexts.size();)
        {
            damageTexts[i].life -= dt;

            damageTexts[i].pos.y +=
                1.5f * dt;

            if (damageTexts[i].life <= 0.0f)
            {
                damageTexts.erase(
                    damageTexts.begin() + i
                );
            }
            else
            {
                i++;
            }
        }

        // ====================================================
        // CAMERA
        // ====================================================

        camera.target =
        {
            playerPos.x,
            playerPos.y + 0.8f,
            playerPos.z
        };

        Vector3 desiredCameraPosition =
        {
            playerPos.x -
            playerFacing.x * 7.5f,

            playerPos.y +
            7.5f,

            playerPos.z -
            playerFacing.z * 7.5f
        };

        // Smooth camera
        camera.position =
            Vector3Lerp(
                camera.position,
                desiredCameraPosition,
                1.0f -
                powf(
                    0.001f,
                    dt
                )
            );

        // ====================================================
        // SCREEN SHAKE
        // ====================================================

        if (screenShake > 0.0f)
        {
            screenShake -= dt;

            camera.position.x +=
                (float)GetRandomValue(
                    -20,
                    20
                ) * 0.01f;

            camera.position.y +=
                (float)GetRandomValue(
                    -20,
                    20
                ) * 0.01f;
        }

        // ====================================================
        // SAVE TOUCH IDS
        // ====================================================

        previousTouchCount = 0;

        int currentCount =
            GetTouchPointCount();

        for (int i = 0;
             i < currentCount &&
             previousTouchCount <
             MAX_TOUCH_IDS;
             i++)
        {
            previousTouchIds[
                previousTouchCount++
            ] =
                GetTouchPointId(i);
        }

        // ====================================================
        // DRAW
        // ====================================================

        BeginDrawing();

        ClearBackground(
            { 8, 10, 18, 255 }
        );

        BeginMode3D(camera);

        // ====================================================
        // ARENA FLOOR
        // ====================================================

        DrawPlane(
            { 0, 0, 0 },
            { ARENA_SIZE, ARENA_SIZE },
            { 25, 29, 42, 255 }
        );

        // ====================================================
        // GRID
        // ====================================================

        for (int x = -(int)ARENA_SIZE / 2;
             x <= (int)ARENA_SIZE / 2;
             x += 2)
        {
            DrawLine3D(
                { (float)x, 0.01f, -ARENA_SIZE / 2 },
                { (float)x, 0.01f, ARENA_SIZE / 2 },
                { 40, 45, 65, 255 }
            );
        }

        for (int z = -(int)ARENA_SIZE / 2;
             z <= (int)ARENA_SIZE / 2;
             z += 2)
        {
            DrawLine3D(
                { -ARENA_SIZE / 2, 0.01f, (float)z },
                { ARENA_SIZE / 2, 0.01f, (float)z },
                { 40, 45, 65, 255 }
            );
        }

        // ====================================================
        // ARENA BORDER
        // ====================================================

        float half =
            ARENA_SIZE / 2.0f;

        DrawCube(
            { 0, 0.5f, -half },
            ARENA_SIZE,
            1,
            1,
            { 60, 70, 100, 255 }
        );

        DrawCube(
            { 0, 0.5f, half },
            ARENA_SIZE,
            1,
            1,
            { 60, 70, 100, 255 }
        );

        DrawCube(
            { -half, 0.5f, 0 },
            1,
            1,
            ARENA_SIZE,
            { 60, 70, 100, 255 }
        );

        DrawCube(
            { half, 0.5f, 0 },
            1,
            1,
            ARENA_SIZE,
            { 60, 70, 100, 255 }
        );

        // ====================================================
        // MAGIC ARENA CENTER
        // ====================================================

        DrawMagicCircle(
            { 0, 0.05f, 0 },
            8.0f,
            { 70, 100, 180, 255 },
            gameTime
        );

        // ====================================================
        // SHOCKWAVES
        // ====================================================

        for (const auto& shock : shockwaves)
        {
            float alpha =
                shock.life /
                shock.maxLife;

            DrawCircle3D(
                shock.pos,
                shock.radius,
                { 1, 0, 0 },
                90,
                ColorAlpha(
                    shock.color,
                    alpha * 0.8f
                )
            );
        }

        // ====================================================
        // SPELLS
        // ====================================================

        for (const auto& spell : spells)
        {
            if (!spell.active)
                continue;

            Color c =
                ElementColor(
                    spell.type
                );

            // Outer aura
            DrawSphere(
                spell.pos,
                spell.radius * 1.8f,
                ColorAlpha(
                    c,
                    0.10f
                )
            );

            // Main orb
            DrawSphere(
                spell.pos,
                spell.radius,
                c
            );

            // Core
            DrawSphere(
                spell.pos,
                spell.radius * 0.42f,
                WHITE
            );

            // Special effects
            if (spell.type ==
                ELEM_FIRE)
            {
                DrawSphere(
                    spell.pos,
                    spell.radius * 1.35f,
                    ColorAlpha(
                        ORANGE,
                        0.20f
                    )
                );
            }

            if (spell.type ==
                ELEM_WATER)
            {
                DrawCircle3D(
                    spell.pos,
                    spell.radius * 1.5f,
                    { 0, 1, 0 },
                    90,
                    ColorAlpha(
                        SKYBLUE,
                        0.30f
                    )
                );
            }
        }

        // ====================================================
        // ENEMIES
        // ====================================================

        for (const auto& enemy : enemies)
        {
            if (!enemy.active)
                continue;

            Color enemyColor;

            if (enemy.type == 0)
                enemyColor =
                    { 190, 45, 45, 255 };

            else if (enemy.type == 1)
                enemyColor =
                    { 100, 95, 100, 255 };

            else
                enemyColor =
                    { 80, 30, 130, 255 };

            if (enemy.hitTimer > 0.0f)
                enemyColor = WHITE;

            if (enemy.freezeTimer > 0.0f)
                enemyColor = SKYBLUE;

            // Shadow
            DrawCylinder(
                {
                    enemy.pos.x,
                    0.08f,
                    enemy.pos.z
                },
                enemy.radius * 0.9f,
                enemy.radius * 0.9f,
                0.05f,
                16,
                ColorAlpha(
                    BLACK,
                    0.35f
                )
            );

            // Body
            if (enemy.type == 1)
            {
                DrawCube(
                    {
                        enemy.pos.x,
                        enemy.pos.y,
                        enemy.pos.z
                    },
                    enemy.radius * 1.7f,
                    enemy.radius * 2.0f,
                    enemy.radius * 1.7f,
                    enemyColor
                );

                DrawCubeWires(
                    enemy.pos,
                    enemy.radius * 1.7f,
                    enemy.radius * 2.0f,
                    enemy.radius * 1.7f,
                    BLACK
                );
            }
            else
            {
                DrawSphere(
                    enemy.pos,
                    enemy.radius,
                    enemyColor
                );

                DrawSphere(
                    {
                        enemy.pos.x,
                        enemy.pos.y +
                        enemy.radius * 0.55f,
                        enemy.pos.z
                    },
                    enemy.radius * 0.45f,
                    ColorAlpha(
                        enemyColor,
                        0.75f
                    )
                );
            }

            // Eyes
            Vector3 eyeOffset =
            {
                0,
                enemy.radius * 0.25f,
                -enemy.radius * 0.85f
            };

            DrawSphere(
                Vector3Add(
                    enemy.pos,
                    eyeOffset
                ),
                enemy.radius * 0.12f,
                RED
            );

            // HP bar in 3D
            float hpPercent =
                enemy.hp /
                enemy.maxHp;

            Vector3 barPos =
            {
                enemy.pos.x,
                enemy.pos.y +
                enemy.radius +
                0.55f,
                enemy.pos.z
            };

            DrawCube(
                barPos,
                enemy.radius * 2.0f,
                0.10f,
                0.08f,
                { 100, 20, 20, 255 }
            );

            DrawCube(
                {
                    barPos.x -
                    enemy.radius *
                    (1.0f - hpPercent),
                    barPos.y + 0.01f,
                    barPos.z
                },
                enemy.radius * 2.0f *
                hpPercent,
                0.12f,
                0.09f,
                GREEN
            );
        }

        // ====================================================
        // PLAYER
        // ====================================================

        Color playerColor =
            ElementColor(
                currentElement
            );

        // Player aura
        DrawSphere(
            playerPos,
            1.65f +
            sinf(gameTime * 5.0f) * 0.12f,
            ColorAlpha(
                playerColor,
                0.12f
            )
        );

        // Body robe
        DrawCylinder(
            {
                playerPos.x,
                playerPos.y - 0.15f,
                playerPos.z
            },
            0.72f,
            0.95f,
            1.8f,
            20,
            { 45, 50, 90, 255 }
        );

        // Head
        DrawSphere(
            {
                playerPos.x,
                playerPos.y + 1.15f,
                playerPos.z
            },
            0.48f,
            { 205, 165, 130, 255 }
        );

        // Hat
        DrawCylinder(
            {
                playerPos.x,
                playerPos.y + 1.65f,
                playerPos.z
            },
            0.70f,
            0.12f,
            0.75f,
            20,
            { 25, 25, 45, 255 }
        );

        DrawCylinder(
            {
                playerPos.x,
                playerPos.y + 2.05f,
                playerPos.z
            },
            0.05f,
            0.48f,
            1.0f,
            20,
            { 25, 25, 45, 255 }
        );

        // Staff
        Vector3 staffStart =
        {
            playerPos.x,
            playerPos.y + 0.1f,
            playerPos.z
        };

        Vector3 staffEnd =
            Vector3Add(
                playerPos,
                Vector3Scale(
                    playerFacing,
                    2.1f
                )
            );

        staffEnd.y += 0.65f;

        DrawLine3D(
            staffStart,
            staffEnd,
            DARKBROWN
        );

        DrawSphere(
            staffEnd,
            0.27f,
            playerColor
        );

        DrawSphere(
            staffEnd,
            0.11f,
            WHITE
        );

        // ====================================================
        // PARTICLES
        // ====================================================

        BeginBlendMode(
            BLEND_ADDITIVE
        );

        for (const auto& p : particles)
        {
            float alpha =
                1.0f -
                p.life /
                p.maxLife;

            DrawSphere(
                p.pos,
                p.size,
                ColorAlpha(
                    p.color,
                    alpha
                )
            );
        }

        EndBlendMode();

        EndMode3D();

        // ====================================================
        // HUD
        // ====================================================

        // ----------------------------------------------------
        // ELEMENT BUTTONS
        // ----------------------------------------------------

        for (int e = 0; e < 5; e++)
        {
            Color c =
                ElementColor(
                    (ElementType)e
                );

            bool selected =
                currentElement ==
                (ElementType)e;

            DrawRectangleRec(
                elementButtons[e],
                selected
                    ? c
                    : ColorAlpha(c, 0.25f)
            );

            DrawRectangleLinesEx(
                elementButtons[e],
                selected ? 3.0f : 1.0f,
                WHITE
            );

            DrawText(
                ElementName(
                    (ElementType)e
                ),
                (int)(
                    elementButtons[e].x +
                    elementButtons[e].width *
                    0.15f
                ),
                (int)(
                    elementButtons[e].y +
                    elementButtons[e].height *
                    0.30f
                ),
                18,
                WHITE
            );
        }

        // ----------------------------------------------------
        // HP
        // ----------------------------------------------------

        DrawRectangle(
            25,
            screenHeight - 75,
            260,
            22,
            { 55, 15, 20, 255 }
        );

        DrawRectangle(
            25,
            screenHeight - 75,
            (int)(
                260 *
                playerHP /
                playerMaxHP
            ),
            22,
            RED
        );

        DrawRectangleLines(
            25,
            screenHeight - 75,
            260,
            22,
            WHITE
        );

        DrawText(
            "HP",
            32,
            screenHeight - 72,
            16,
            WHITE
        );

        // ----------------------------------------------------
        // MANA
        // ----------------------------------------------------

        DrawRectangle(
            25,
            screenHeight - 45,
            260,
            18,
            { 15, 30, 65, 255 }
        );

        DrawRectangle(
            25,
            screenHeight - 45,
            (int)(
                260 *
                playerMana /
                playerMaxMana
            ),
            18,
            SKYBLUE
        );

        DrawRectangleLines(
            25,
            screenHeight - 45,
            260,
            18,
            WHITE
        );

        DrawText(
            "MANA",
            32,
            screenHeight - 43,
            14,
            WHITE
        );

        // ----------------------------------------------------
        // SCORE
        // ----------------------------------------------------

        DrawText(
            TextFormat(
                "SCORE: %d",
                score
            ),
            25,
            125,
            25,
            GOLD
        );

        DrawText(
            TextFormat(
                "WAVE: %d",
                wave
            ),
            25,
            155,
            21,
            RAYWHITE
        );

        // ----------------------------------------------------
        // JOYSTICK
        // ----------------------------------------------------

        DrawCircleV(
            stickCenter,
            stickRadius,
            ColorAlpha(
                DARKGRAY,
                0.50f
            )
        );

        DrawCircleLines(
            (int)stickCenter.x,
            (int)stickCenter.y,
            stickRadius,
            LIGHTGRAY
        );

        DrawCircleV(
            knobPos,
            knobRadius,
            ColorAlpha(
                ElementColor(
                    currentElement
                ),
                0.85f
            )
        );

        DrawCircleLines(
            (int)knobPos.x,
            (int)knobPos.y,
            knobRadius,
            WHITE
        );

        // ----------------------------------------------------
        // CAST BUTTON
        // ----------------------------------------------------

        bool casting =
            attackTouchId != -1 ||
            IsKeyDown(KEY_SPACE);

        Color castColor =
            casting
                ? ElementColor(
                    currentElement
                  )
                : ColorAlpha(
                    ElementColor(
                        currentElement
                    ),
                    0.55f
                  );

        DrawRectangleRec(
            attackButton,
            castColor
        );

        DrawRectangleLinesEx(
            attackButton,
            4,
            GOLD
        );

        int castTextWidth =
            MeasureText(
                "CAST",
                26
            );

        DrawText(
            "CAST",
            (int)(
                attackButton.x +
                attackButton.width / 2 -
                castTextWidth / 2
            ),
            (int)(
                attackButton.y +
                attackButton.height / 2 -
                13
            ),
            26,
            WHITE
        );

        // ----------------------------------------------------
        // CROSSHAIR
        // ----------------------------------------------------

        DrawCircleLines(
            screenWidth / 2,
            screenHeight / 2,
            5,
            ColorAlpha(
                WHITE,
                0.45f
            )
        );

        // ====================================================
        // GAME OVER
        // ====================================================

        if (gameOver)
        {
            DrawRectangle(
                0,
                0,
                screenWidth,
                screenHeight,
                ColorAlpha(
                    BLACK,
                    0.78f
                )
            );

            const char* title =
                "YOU WERE OVERWHELMED";

            int titleWidth =
                MeasureText(
                    title,
                    38
                );

            DrawText(
                title,
                screenWidth / 2 -
                titleWidth / 2,
                screenHeight / 2 -
                70,
                38,
                RED
            );

            const char* retry =
                "TAP TO RESTART";

            int retryWidth =
                MeasureText(
                    retry,
                    25
                );

            DrawText(
                retry,
                screenWidth / 2 -
                retryWidth / 2,
                screenHeight / 2,
                25,
                WHITE
            );

            const char* result =
                TextFormat(
                    "SCORE %d   WAVE %d",
                    score,
                    wave
                );

            int resultWidth =
                MeasureText(
                    result,
                    24
                );

            DrawText(
                result,
                screenWidth / 2 -
                resultWidth / 2,
                screenHeight / 2 + 50,
                24,
                GOLD
            );
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
