#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cmath>

enum ElementType {
    ELEM_FIRE,
    ELEM_ICE,
    ELEM_EARTH,
    ELEM_WIND,
    ELEM_WATER
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float size;
    float alpha;
    float life;
    float maxLife;
    bool additive;
};

struct IceCrystal {
    Vector2 pos;
    float currentHeight;
    float maxHeight;
    float width;
    float angle;
    float life;
};

struct RockFragment {
    Vector2 pos;
    Vector2 vel;
    float size;
    float angle;
    float rotSpeed;
    float life;
};

struct Spell {
    Vector2 pos;
    Vector2 vel;
    ElementType type;
    float radius;
    float damage;
    float timer;
    float life;
    bool active;
};

struct Enemy {
    Vector2 pos;
    float hp;
    float maxHp;
    float speed;
    float radius;
    Color color;
    float hitTimer;
    float freezeTimer;
    bool active;
};

struct FloatingText {
    Vector2 pos;
    char text[16];
    Color color;
    float life;
};

#if defined(__cplusplus)
extern "C" {
#endif

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    InitWindow(0, 0, "Elemental Mage: Arena Survival");
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    SetTargetFPS(60);

    // إعدادات اللاعب والساحة
    Vector2 playerPos = { 1000.0f, 1000.0f };
    Vector2 playerFacing = { 1.0f, 0.0f };
    float playerSpeed = 260.0f;
    float playerHp = 100.0f;
    float playerMaxHp = 100.0f;
    float playerMana = 100.0f;
    float playerMaxMana = 100.0f;

    const float arenaSize = 2000.0f;
    Camera2D camera = { 0 };
    camera.target = playerPos;
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
    camera.zoom = 1.0f;

    // عناصر التحكم باللمس المتعدد (Touch Controls)
    Vector2 stickCenter = { screenWidth * 0.14f, screenHeight * 0.74f };
    float stickBaseRadius = screenHeight * 0.12f;
    float stickKnobRadius = screenHeight * 0.05f;
    Vector2 knobPos = stickCenter;
    bool stickTouchFound = false;

    // أزرار العناصر والقتال
    ElementType currentElement = ELEM_FIRE;
    float btnW = screenWidth * 0.13f;
    float btnH = screenHeight * 0.09f;
    float btnY = screenHeight * 0.04f;

    Rectangle elemButtons[5] = {
        { screenWidth * 0.04f, btnY, btnW, btnH }, // FIRE
        { screenWidth * 0.18f, btnY, btnW, btnH }, // ICE
        { screenWidth * 0.32f, btnY, btnW, btnH }, // EARTH
        { screenWidth * 0.46f, btnY, btnW, btnH }, // WIND
        { screenWidth * 0.60f, btnY, btnW, btnH }  // WATER
    };
    Color elemColors[5] = {
        { 245, 60, 20, 255 },  // FIRE
        { 70, 210, 255, 255 }, // ICE
        { 160, 110, 50, 255 }, // EARTH
        { 120, 240, 160, 255 },// WIND
        { 30, 130, 255, 255 }  // WATER
    };
    const char* elemNames[5] = { "FIRE", "ICE", "EARTH", "WIND", "WATER" };

    Rectangle attackBtn = { screenWidth - (screenWidth * 0.20f), screenHeight - (screenHeight * 0.23f), screenWidth * 0.16f, screenHeight * 0.17f };

    // مصفوفات الكائنات في اللعبة
    std::vector<Particle> particles;
    std::vector<IceCrystal> iceCrystals;
    std::vector<RockFragment> rocks;
    std::vector<Spell> spells;
    std::vector<Enemy> enemies;
    std::vector<FloatingText> floatTexts;

    float spawnTimer = 0.0f;
    float screenShake = 0.0f;
    int score = 0;
    bool gameOver = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;

        if (!gameOver) {
            // تجديد المانا تلقائياً
            if (playerMana < playerMaxMana) playerMana += 18.0f * dt;

            // --- 1. معالجة اللمس المتعدد للهواتف (Multi-touch System) ---
            stickTouchFound = false;
            Vector2 moveDir = { 0.0f, 0.0f };
            int touchCount = GetTouchPointCount();

            for (int i = 0; i < touchCount || (touchCount == 0 && IsMouseButtonDown(MOUSE_BUTTON_LEFT)); i++) {
                Vector2 tPos = (touchCount > 0) ? GetTouchPosition(i) : GetMousePosition();

                // ذراع التحكم في الجهة اليسرى
                if (CheckCollisionPointCircle(tPos, stickCenter, stickBaseRadius * 1.6f)) {
                    stickTouchFound = true;
                    Vector2 diff = Vector2Subtract(tPos, stickCenter);
                    float dLen = Vector2Length(diff);
                    if (dLen > stickBaseRadius) diff = Vector2Scale(Vector2Normalize(diff), stickBaseRadius);
                    knobPos = Vector2Add(stickCenter, diff);
                    moveDir = Vector2Scale(diff, 1.0f / stickBaseRadius);
                    if (Vector2Length(moveDir) > 0.15f) playerFacing = Vector2Normalize(moveDir);
                }

                // اختيار العناصر السحرية
                if (touchCount > 0 ? IsGestureDetected(GESTURE_TAP) : IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    for (int e = 0; e < 5; e++) {
                        if (CheckCollisionPointRec(tPos, elemButtons[e])) {
                            currentElement = (ElementType)e;
                        }
                    }
                }

                // زر إطلاق السحر (CAST)
                if (CheckCollisionPointRec(tPos, attackBtn)) {
                    bool canCast = (touchCount > 0) ? IsGestureDetected(GESTURE_TAP) || IsGestureDetected(GESTURE_HOLD) : IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
                    if (canCast && playerMana >= 15.0f) {
                        playerMana -= 15.0f;
                        Vector2 targetCastDir = playerFacing;

                        // توجيه تلقائي نحو أقرب وحش إن وجد
                        float nearestDist = 500.0f;
                        for (const auto &en : enemies) {
                            if (!en.active) continue;
                            float d = Vector2Distance(playerPos, en.pos);
                            if (d < nearestDist) {
                                nearestDist = d;
                                targetCastDir = Vector2Normalize(Vector2Subtract(en.pos, playerPos));
                            }
                        }

                        // إنشاء تعويذة السحر بحسب العنصر
                        Spell sp;
                        sp.pos = Vector2Add(playerPos, Vector2Scale(targetCastDir, 35.0f));
                        sp.type = currentElement;
                        sp.active = true;
                        sp.timer = 0.0f;

                        if (currentElement == ELEM_FIRE) {
                            sp.vel = Vector2Scale(targetCastDir, 420.0f);
                            sp.radius = 32.0f;
                            sp.damage = 45.0f;
                            sp.life = 1.6f;
                            screenShake = 0.12f;
                        } else if (currentElement == ELEM_ICE) {
                            sp.vel = Vector2Scale(targetCastDir, 520.0f);
                            sp.radius = 24.0f;
                            sp.damage = 30.0f;
                            sp.life = 1.2f;
                        } else if (currentElement == ELEM_EARTH) {
                            sp.vel = Vector2Scale(targetCastDir, 320.0f);
                            sp.radius = 48.0f;
                            sp.damage = 80.0f;
                            sp.life = 1.8f;
                            screenShake = 0.28f;
                        } else if (currentElement == ELEM_WIND) {
                            sp.vel = Vector2Scale(targetCastDir, 650.0f);
                            sp.radius = 26.0f;
                            sp.damage = 25.0f;
                            sp.life = 1.0f;
                        } else if (currentElement == ELEM_WATER) {
                            sp.vel = Vector2Scale(targetCastDir, 460.0f);
                            sp.radius = 36.0f;
                            sp.damage = 35.0f;
                            sp.life = 1.5f;
                        }
                        spells.push_back(sp);
                    }
                }
            }

            if (!stickTouchFound) knobPos = stickCenter;

            // تحديث حركة الساحر داخل حدود الساحة
            playerPos = Vector2Add(playerPos, Vector2Scale(moveDir, playerSpeed * dt));
            playerPos.x = Clamp(playerPos.x, 60.0f, arenaSize - 60.0f);
            playerPos.y = Clamp(playerPos.y, 60.0f, arenaSize - 60.0f);

            // --- 2. توليد وتحديث الوحوش والذكاء الاصطناعي ---
            spawnTimer += dt;
            if (spawnTimer >= 1.6f && enemies.size() < 40) {
                spawnTimer = 0.0f;
                Enemy en;
                float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
                float dist = (float)GetRandomValue(500, 750);
                en.pos = Vector2Add(playerPos, { cosf(angle) * dist, sinf(angle) * dist });
                en.active = true;
                en.hitTimer = 0.0f;
                en.freezeTimer = 0.0f;

                // تنوع الوحوش
                int rType = GetRandomValue(0, 2);
                if (rType == 0) { // زاحف سريع (Chaser)
                    en.hp = en.maxHp = 60.0f;
                    en.speed = 175.0f;
                    en.radius = 20.0f;
                    en.color = { 180, 40, 40, 255 };
                } else if (rType == 1) { // غول صخري ضخم (Golem)
                    en.hp = en.maxHp = 180.0f;
                    en.speed = 95.0f;
                    en.radius = 34.0f;
                    en.color = { 90, 85, 95, 255 };
                } else { // شبح الظل (Shadow)
                    en.hp = en.maxHp = 90.0f;
                    en.speed = 135.0f;
                    en.radius = 24.0f;
                    en.color = { 75, 30, 110, 255 };
                }
                enemies.push_back(en);
            }

            // حركة الوحوش نحو الساحر وفحص الضرر
            for (auto &en : enemies) {
                if (!en.active) continue;
                if (en.hitTimer > 0.0f) en.hitTimer -= dt;

                if (en.freezeTimer > 0.0f) {
                    en.freezeTimer -= dt;
                    continue; // متجمد بالكامل
                }

                Vector2 toPlayer = Vector2Subtract(playerPos, en.pos);
                float dist = Vector2Length(toPlayer);

                if (dist > 10.0f) {
                    en.pos = Vector2Add(en.pos, Vector2Scale(Vector2Normalize(toPlayer), en.speed * dt));
                }

                // هجوم الوحش على الساحر
                if (dist < (en.radius + 24.0f)) {
                    playerHp -= 20.0f * dt;
                    screenShake = 0.08f;
                    if (playerHp <= 0.0f) {
                        playerHp = 0.0f;
                        gameOver = true;
                    }
                }
            }

            // --- 3. فيزياء المقذوفات وتأثيرات العناصر الحقيقية ---
            for (auto &sp : spells) {
                if (!sp.active) continue;
                sp.pos = Vector2Add(sp.pos, Vector2Scale(sp.vel, dt));
                sp.timer += dt;
                sp.life -= dt;
                if (sp.life <= 0.0f) sp.active = false;

                // سلوك النار (لهب مشتعل ودخان)
                if (sp.type == ELEM_FIRE) {
                    for (int k = 0; k < 4; k++) {
                        Particle p;
                        p.pos = Vector2Add(sp.pos, { (float)GetRandomValue(-10, 10), (float)GetRandomValue(-10, 10) });
                        p.vel = { (float)GetRandomValue(-30, 30), (float)GetRandomValue(-50, 10) };
                        p.color = (GetRandomValue(0, 2) == 0) ? YELLOW : (GetRandomValue(0, 1) == 0 ? ORANGE : RED);
                        p.size = (float)GetRandomValue(12, 26);
                        p.alpha = 1.0f;
                        p.life = 0.0f;
                        p.maxLife = 0.4f;
                        p.additive = true;
                        particles.push_back(p);
                    }
                }
                // سلوك الثلج: يزرع بلورات متجمدة في مساره
                else if (sp.type == ELEM_ICE) {
                    if (GetRandomValue(0, 2) == 0) {
                        IceCrystal ic;
                        ic.pos = sp.pos;
                        ic.currentHeight = 0.0f;
                        ic.maxHeight = (float)GetRandomValue(28, 48);
                        ic.width = (float)GetRandomValue(12, 20);
                        ic.angle = (float)GetRandomValue(-25, 25);
                        ic.life = 2.0f;
                        iceCrystals.push_back(ic);
                    }
                    Particle p;
                    p.pos = sp.pos;
                    p.vel = { (float)GetRandomValue(-20, 20), (float)GetRandomValue(-20, 20) };
                    p.color = SKYBLUE;
                    p.size = (float)GetRandomValue(8, 16);
                    p.alpha = 1.0f;
                    p.life = 0.0f;
                    p.maxLife = 0.35f;
                    p.additive = true;
                    particles.push_back(p);
                }
                // سلوك صخور الأرض: ينبثق حطام وشظايا صلبة
                else if (sp.type == ELEM_EARTH) {
                    RockFragment rf;
                    rf.pos = sp.pos;
                    rf.vel = { (float)GetRandomValue(-70, 70), (float)GetRandomValue(-70, 70) };
                    rf.size = (float)GetRandomValue(14, 28);
                    rf.angle = (float)GetRandomValue(0, 360);
                    rf.rotSpeed = (float)GetRandomValue(-180, 180);
                    rf.life = 1.2f;
                    rocks.push_back(rf);
                }
                // سلوك الرياح: شفرات دوارة
                else if (sp.type == ELEM_WIND) {
                    for (int k = 0; k < 3; k++) {
                        Particle p;
                        p.pos = sp.pos;
                        p.vel = { cosf(sp.timer * 20.0f + k) * 90.0f, sinf(sp.timer * 20.0f + k) * 90.0f };
                        p.color = LIME;
                        p.size = (float)GetRandomValue(8, 18);
                        p.alpha = 0.8f;
                        p.life = 0.0f;
                        p.maxLife = 0.25f;
                        p.additive = true;
                        particles.push_back(p);
                    }
                }
                // سلوك الماء: موجة جيبية ورذاذ فقاعات
                else if (sp.type == ELEM_WATER) {
                    Vector2 wavePos = sp.pos;
                    wavePos.y += sinf(sp.timer * 16.0f) * 18.0f;
                    for (int k = 0; k < 3; k++) {
                        Particle p;
                        p.pos = wavePos;
                        p.vel = { (float)GetRandomValue(-40, 40), (float)GetRandomValue(-40, 40) };
                        p.color = (GetRandomValue(0, 1) == 0) ? SKYBLUE : BLUE;
                        p.size = (float)GetRandomValue(10, 20);
                        p.alpha = 0.9f;
                        p.life = 0.0f;
                        p.maxLife = 0.35f;
                        p.additive = true;
                        particles.push_back(p);
                    }
                }

                // فحص تصادم السحر بالوحوش
                for (auto &en : enemies) {
                    if (!en.active) continue;
                    if (CheckCollisionCircles(sp.pos, sp.radius, en.pos, en.radius)) {
                        en.hp -= sp.damage;
                        en.hitTimer = 0.18f;

                        if (sp.type == ELEM_ICE) en.freezeTimer = 1.8f; // تجميد الوحش
                        if (sp.type == ELEM_EARTH) en.pos = Vector2Add(en.pos, Vector2Scale(playerFacing, 35.0f)); // دفع قوي

                        // نص ضرر عائم
                        FloatingText ft;
                        ft.pos = en.pos;
                        snprintf(ft.text, sizeof(ft.text), "-%d", (int)sp.damage);
                        ft.color = elemColors[sp.type];
                        ft.life = 0.6f;
                        floatTexts.push_back(ft);

                        // انفجار بصري عند الإصابة
                        for (int k = 0; k < 14; k++) {
                            Particle p;
                            p.pos = en.pos;
                            p.vel = { (float)GetRandomValue(-120, 120), (float)GetRandomValue(-120, 120) };
                            p.color = elemColors[sp.type];
                            p.size = (float)GetRandomValue(6, 16);
                            p.alpha = 1.0f;
                            p.life = 0.0f;
                            p.maxLife = 0.45f;
                            p.additive = true;
                            particles.push_back(p);
                        }

                        if (en.hp <= 0.0f) {
                            en.active = false;
                            score += 100;
                        }

                        // السحر الصلب يخترق، والسحر الانفجاري ينفجر
                        if (sp.type == ELEM_FIRE) sp.active = false;
                    }
                }
            }
        } else {
            // إعادة المحاولة عند النقر بعد الخسارة
            if (GetTouchPointCount() > 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                playerHp = playerMaxHp;
                playerMana = playerMaxMana;
                playerPos = { 1000.0f, 1000.0f };
                enemies.clear();
                spells.clear();
                score = 0;
                gameOver = false;
            }
        }

        // --- 4. تحديث الأجسام الثانوية واهتزاز الشاشة ---
        for (size_t i = 0; i < iceCrystals.size();) {
            if (iceCrystals[i].currentHeight < iceCrystals[i].maxHeight)
                iceCrystals[i].currentHeight += dt * 320.0f;
            iceCrystals[i].life -= dt;
            if (iceCrystals[i].life <= 0.0f) iceCrystals.erase(iceCrystals.begin() + i);
            else i++;
        }

        for (size_t i = 0; i < rocks.size();) {
            rocks[i].pos = Vector2Add(rocks[i].pos, Vector2Scale(rocks[i].vel, dt));
            rocks[i].angle += rocks[i].rotSpeed * dt;
            rocks[i].life -= dt;
            if (rocks[i].life <= 0.0f) rocks.erase(rocks.begin() + i);
            else i++;
        }

        for (size_t i = 0; i < particles.size();) {
            particles[i].pos = Vector2Add(particles[i].pos, Vector2Scale(particles[i].vel, dt));
            particles[i].life += dt;
            particles[i].alpha = 1.0f - (particles[i].life / particles[i].maxLife);
            particles[i].size *= 0.95f;
            if (particles[i].life >= particles[i].maxLife) particles.erase(particles.begin() + i);
            else i++;
        }

        for (size_t i = 0; i < floatTexts.size();) {
            floatTexts[i].pos.y -= 35.0f * dt;
            floatTexts[i].life -= dt;
            if (floatTexts[i].life <= 0.0f) floatTexts.erase(floatTexts.begin() + i);
            else i++;
        }

        // تحديث الكاميرا مع اهتزاز الشاشة (Screen Shake)
        camera.target = playerPos;
        if (screenShake > 0.0f) {
            camera.target.x += (float)GetRandomValue(-12, 12);
            camera.target.y += (float)GetRandomValue(-12, 12);
            screenShake -= dt;
        }

        // --- 5. الرسم والإخراج البصري المتقدم ---
        BeginDrawing();
        ClearBackground({ 14, 16, 24, 255 });

        BeginMode2D(camera);

        // رسم أرضية الساحة وشبكة البلاط السحري
        DrawRectangle(0, 0, (int)arenaSize, (int)arenaSize, { 22, 25, 38, 255 });
        for (int x = 0; x < (int)arenaSize; x += 100) DrawLine(x, 0, x, (int)arenaSize, { 32, 36, 52, 255 });
        for (int y = 0; y < (int)arenaSize; y += 100) DrawLine(0, y, (int)arenaSize, y, { 32, 36, 52, 255 });
        DrawRectangleLinesEx({ 0, 0, arenaSize, arenaSize }, 8.0f, { 80, 90, 130, 255 });

        // رسم بلورات الثلج البارزة
        for (const auto &ic : iceCrystals) {
            Vector2 tip = { ic.pos.x, ic.pos.y - ic.currentHeight };
            Vector2 p1 = { ic.pos.x - ic.width * 0.5f, ic.pos.y };
            Vector2 p2 = { ic.pos.x + ic.width * 0.5f, ic.pos.y };
            DrawTriangle(tip, p1, p2, SKYBLUE);
            DrawLineEx(tip, { ic.pos.x, ic.pos.y }, 2.0f, WHITE);
        }

        // رسم كتل الصخور الدوارة
        for (const auto &rf : rocks) {
            DrawPoly(rf.pos, 5, rf.size, rf.angle, DARKBROWN);
            DrawPolyLinesEx(rf.pos, 5, rf.size, rf.angle, 2.0f, BROWN);
        }

        // رسم الوحوش مع أشرطة صحتهم
        for (const auto &en : enemies) {
            if (!en.active) continue;
            Color drawCol = (en.hitTimer > 0.0f) ? WHITE : ((en.freezeTimer > 0.0f) ? SKYBLUE : en.color);
            DrawCircleV(en.pos, en.radius, drawCol);
            DrawCircleLines((int)en.pos.x, (int)en.pos.y, en.radius, BLACK);

            // شريط صحة الوحش
            float barW = en.radius * 2.0f;
            DrawRectangle((int)(en.pos.x - en.radius), (int)(en.pos.y - en.radius - 12), (int)barW, 5, RED);
            DrawRectangle((int)(en.pos.x - en.radius), (int)(en.pos.y - en.radius - 12), (int)(barW * (en.hp / en.maxHp)), 5, GREEN);
        }

        // رسم مقذوفات السحر
        for (const auto &sp : spells) {
            if (!sp.active) continue;
            DrawCircleV(sp.pos, sp.radius * 1.3f, ColorAlpha(elemColors[sp.type], 0.35f));
            DrawCircleV(sp.pos, sp.radius, elemColors[sp.type]);
            DrawCircleV(sp.pos, sp.radius * 0.45f, WHITE);
        }

        // رسم الجزيئات بتوهج متراكب
        BeginBlendMode(BLEND_ADDITIVE);
        for (const auto &p : particles) {
            DrawCircleV(p.pos, p.size, ColorAlpha(p.color, p.alpha));
        }
        EndBlendMode();

        // رسم الساحر (الشخصية)
        DrawCircleV(playerPos, 22.0f, { 45, 52, 90, 255 }); // العباءة
        DrawCircleV(playerPos, 12.0f, { 35, 40, 70, 255 }); // القبعة
        // عصا الساحر موجهة باتجاه الحركة
        Vector2 staffPos = Vector2Add(playerPos, Vector2Scale(playerFacing, 26.0f));
        DrawLineEx(playerPos, staffPos, 6.0f, DARKBROWN);
        DrawCircleV(staffPos, 10.0f, elemColors[currentElement]);
        DrawCircleV(staffPos, 5.0f, WHITE);

        // أرقام الضرر العائمة
        for (const auto &ft : floatTexts) {
            DrawText(ft.text, (int)ft.pos.x, (int)ft.pos.y, 22, ft.color);
        }

        EndMode2D();

        // --- 6. واجهة المستخدم للهواتف (Touch HUD) ---
        // أزرار العناصر العلوية
        for (int e = 0; e < 5; e++) {
            bool sel = (currentElement == (ElementType)e);
            DrawRectangleRec(elemButtons[e], sel ? elemColors[e] : ColorAlpha(elemColors[e], 0.3f));
            DrawRectangleLinesEx(elemButtons[e], sel ? 4.0f : 1.5f, WHITE);
            DrawText(elemNames[e], (int)(elemButtons[e].x + btnW * 0.18f), (int)(elemButtons[e].y + btnH * 0.3f), 20, WHITE);
        }

        // أشرطة الصحة والمانا للاعب في الزاوية العلوية
        DrawRectangle(30, screenHeight - 65, 220, 22, { 60, 20, 20, 255 });
        DrawRectangle(30, screenHeight - 65, (int)(220 * (playerHp / playerMaxHp)), 22, RED);
        DrawRectangleLines(30, screenHeight - 65, 220, 22, WHITE);
        DrawText("HP", 35, screenHeight - 62, 16, WHITE);

        DrawRectangle(30, screenHeight - 35, 220, 18, { 20, 35, 70, 255 });
        DrawRectangle(30, screenHeight - 35, (int)(220 * (playerMana / playerMaxMana)), 18, SKYBLUE);
        DrawRectangleLines(30, screenHeight - 35, 220, 18, WHITE);
        DrawText("MANA", 35, screenHeight - 33, 14, WHITE);

        // نقاط القتل
        DrawText(TextFormat("SCORE: %d", score), 30, (int)(btnY + btnH + 15), 24, GOLD);

        // ذراع التحكم (Virtual Joystick)
        DrawCircleV(stickCenter, stickBaseRadius, ColorAlpha(DARKGRAY, 0.45f));
        DrawCircleLines((int)stickCenter.x, (int)stickCenter.y, stickBaseRadius, LIGHTGRAY);
        DrawCircleV(knobPos, stickKnobRadius, ColorAlpha(elemColors[currentElement], 0.75f));

        // زر إطلاق السحر (CAST)
        DrawRectangleRec(attackBtn, { 210, 45, 45, 220 });
        DrawRectangleLinesEx(attackBtn, 4.0f, GOLD);
        DrawText("CAST", (int)(attackBtn.x + attackBtn.width * 0.22f), (int)(attackBtn.y + attackBtn.height * 0.35f), 26, WHITE);

        // شاشة نهاية اللعبة
        if (gameOver) {
            DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.8f));
            DrawText("YOU WERE OVERWHELMED!", screenWidth / 2 - 240, screenHeight / 2 - 50, 36, RED);
            DrawText("TAP ANYWHERE TO RETRY", screenWidth / 2 - 180, screenHeight / 2 + 20, 26, WHITE);
            DrawText(TextFormat("FINAL SCORE: %d", score), screenWidth / 2 - 120, screenHeight / 2 + 70, 28, GOLD);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#if defined(__cplusplus)
}
#endif
