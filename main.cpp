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
    float drag; // 0..1, applied per second (higher = more friction)
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
    char text[24];
    Color color;
    float life;
    float maxLife;
    float size;
};

// أثر أرضي: صقيع / تشقق / حلقة ماء / حرق - يعيش على الأرض ويتلاشى
struct GroundEffect {
    Vector2 pos;
    float radius;
    float maxRadius;
    float life;
    float maxLife;
    Color color;
    int type; // 0 ripple(water) 1 crack(earth) 2 frost(ice) 3 scorch(fire)
    float angle;
};

// موجة صدمة دائرية عند الإصابة أو الموت
struct Shockwave {
    Vector2 pos;
    float radius;
    float maxRadius;
    float life;
    float maxLife;
    Color color;
    float thickness;
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
    float zoomPunch = 0.0f; // نبضة تكبير عند القذف/الإصابة، تعود تدريجياً لـ0

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
    std::vector<Particle> ambientDust; // غبار محيطي جوي في الساحة
    std::vector<IceCrystal> iceCrystals;
    std::vector<RockFragment> rocks;
    std::vector<Spell> spells;
    std::vector<Enemy> enemies;
    std::vector<FloatingText> floatTexts;
    std::vector<GroundEffect> groundEffects;
    std::vector<Shockwave> shockwaves;

    float spawnTimer = 0.0f;
    float screenShake = 0.0f;
    int score = 0;
    bool gameOver = false;

    // نظام الموجات (Waves)
    int wave = 1;
    float waveTimer = 0.0f;
    const float waveDuration = 25.0f;
    float waveAnnounceTimer = 0.0f;
    char waveAnnounceText[32] = "";

    // نظام الكومبو
    int comboCount = 0;
    float comboTimer = 0.0f;
    const float comboWindow = 1.4f;

    // تهيئة غبار محيطي أولي
    for (int i = 0; i < 60; i++) {
        Particle d;
        d.pos = { (float)GetRandomValue(0, (int)arenaSize), (float)GetRandomValue(0, (int)arenaSize) };
        d.vel = { (float)GetRandomValue(-8, 8), (float)GetRandomValue(-8, 8) };
        d.color = { 120, 130, 170, 255 };
        d.size = (float)GetRandomValue(2, 5);
        d.alpha = (float)GetRandomValue(20, 60) / 100.0f;
        d.life = 0.0f;
        d.maxLife = 999999.0f;
        d.additive = false;
        d.drag = 0.0f;
        ambientDust.push_back(d);
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;
        float gameTime = (float)GetTime();

        if (!gameOver) {
            // تجديد المانا تلقائياً
            if (playerMana < playerMaxMana) playerMana += 18.0f * dt;

            // تقليل نبضة التكبير تدريجياً
            if (zoomPunch > 0.0f) {
                zoomPunch -= dt * 3.0f;
                if (zoomPunch < 0.0f) zoomPunch = 0.0f;
            }

            // تحديث مؤقت الكومبو
            if (comboTimer > 0.0f) {
                comboTimer -= dt;
                if (comboTimer <= 0.0f) comboCount = 0;
            }

            // نظام الموجات: كل waveDuration ثانية تزيد صعوبة الوحوش
            waveTimer += dt;
            if (waveTimer >= waveDuration) {
                waveTimer = 0.0f;
                wave++;
                snprintf(waveAnnounceText, sizeof(waveAnnounceText), "WAVE %d", wave);
                waveAnnounceTimer = 2.2f;
            }
            if (waveAnnounceTimer > 0.0f) waveAnnounceTimer -= dt;

            // --- 1. معالجة اللمس المتعدد للهواتف (Multi-touch System) ---
            stickTouchFound = false;
            Vector2 moveDir = { 0.0f, 0.0f };
            int touchCount = GetTouchPointCount();

            // إصلاح: عدّاد إدخال صحيح بدل شرط حلقة كان يسبب تعليقاً لا نهائياً
            // عند استخدام الماوس (touchCount == 0) في النسخة الأصلية
            int inputCount = (touchCount > 0) ? touchCount
                                               : (IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? 1 : 0);

            for (int i = 0; i < inputCount; i++) {
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
                        zoomPunch = 0.06f; // نبضة تكبير بصرية عند القذف
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

                        // نبضة توهج انطلاق عند طرف العصا
                        for (int k = 0; k < 10; k++) {
                            Particle p;
                            p.pos = sp.pos;
                            float a = (float)GetRandomValue(0, 360) * DEG2RAD;
                            float spd = (float)GetRandomValue(60, 180);
                            p.vel = { cosf(a) * spd, sinf(a) * spd };
                            p.color = elemColors[currentElement];
                            p.size = (float)GetRandomValue(4, 10);
                            p.alpha = 1.0f;
                            p.life = 0.0f;
                            p.maxLife = 0.3f;
                            p.additive = true;
                            p.drag = 2.0f;
                            particles.push_back(p);
                        }
                    }
                }
            }

            if (!stickTouchFound) knobPos = stickCenter;

            // تحديث حركة الساحر داخل حدود الساحة
            playerPos = Vector2Add(playerPos, Vector2Scale(moveDir, playerSpeed * dt));
            playerPos.x = Clamp(playerPos.x, 60.0f, arenaSize - 60.0f);
            playerPos.y = Clamp(playerPos.y, 60.0f, arenaSize - 60.0f);

            // أثر خطوات خفيف عند الحركة
            if (Vector2Length(moveDir) > 0.2f && GetRandomValue(0, 4) == 0) {
                Particle p;
                p.pos = Vector2Add(playerPos, { (float)GetRandomValue(-6, 6), 18.0f });
                p.vel = { 0, -10.0f };
                p.color = { 200, 200, 220, 255 };
                p.size = (float)GetRandomValue(3, 6);
                p.alpha = 0.5f;
                p.life = 0.0f;
                p.maxLife = 0.4f;
                p.additive = false;
                p.drag = 1.0f;
                particles.push_back(p);
            }

            // --- 2. توليد وتحديث الوحوش والذكاء الاصطناعي ---
            spawnTimer += dt;
            float spawnInterval = fmaxf(0.5f, 1.6f - (wave - 1) * 0.12f);
            int maxEnemies = 40 + (wave - 1) * 6;
            if (spawnTimer >= spawnInterval && (int)enemies.size() < maxEnemies) {
                spawnTimer = 0.0f;
                Enemy en;
                float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
                float dist = (float)GetRandomValue(500, 750);
                en.pos = Vector2Add(playerPos, { cosf(angle) * dist, sinf(angle) * dist });
                en.active = true;
                en.hitTimer = 0.0f;
                en.freezeTimer = 0.0f;

                float waveMul = 1.0f + (wave - 1) * 0.10f;

                // تنوع الوحوش
                int rType = GetRandomValue(0, 2);
                if (rType == 0) { // زاحف سريع (Chaser)
                    en.hp = en.maxHp = 60.0f * waveMul;
                    en.speed = 175.0f + (wave - 1) * 4.0f;
                    en.radius = 20.0f;
                    en.color = { 180, 40, 40, 255 };
                } else if (rType == 1) { // غول صخري ضخم (Golem)
                    en.hp = en.maxHp = 180.0f * waveMul;
                    en.speed = 95.0f + (wave - 1) * 2.0f;
                    en.radius = 34.0f;
                    en.color = { 90, 85, 95, 255 };
                } else { // شبح الظل (Shadow)
                    en.hp = en.maxHp = 90.0f * waveMul;
                    en.speed = 135.0f + (wave - 1) * 3.0f;
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
                    // جزيئات صقيع خفيفة على الوحش المتجمد
                    if (GetRandomValue(0, 6) == 0) {
                        Particle p;
                        p.pos = Vector2Add(en.pos, { (float)GetRandomValue(-10, 10), (float)GetRandomValue(-10, 10) });
                        p.vel = { 0, -12.0f };
                        p.color = SKYBLUE;
                        p.size = (float)GetRandomValue(3, 6);
                        p.alpha = 0.8f;
                        p.life = 0.0f;
                        p.maxLife = 0.35f;
                        p.additive = true;
                        p.drag = 0.5f;
                        particles.push_back(p);
                    }
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
                Vector2 prevPos = sp.pos;
                sp.pos = Vector2Add(sp.pos, Vector2Scale(sp.vel, dt));
                sp.timer += dt;
                sp.life -= dt;
                if (sp.life <= 0.0f) sp.active = false;

                // سلوك النار (لهب مشتعل ودخان + شرر)
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
                        p.drag = 0.5f;
                        particles.push_back(p);
                    }
                    // دخان خفيف خلف الكرة النارية
                    if (GetRandomValue(0, 1) == 0) {
                        Particle p;
                        p.pos = sp.pos;
                        p.vel = { (float)GetRandomValue(-15, 15), (float)GetRandomValue(-40, -10) };
                        p.color = { 70, 70, 70, 255 };
                        p.size = (float)GetRandomValue(10, 18);
                        p.alpha = 0.35f;
                        p.life = 0.0f;
                        p.maxLife = 0.6f;
                        p.additive = false;
                        p.drag = 0.3f;
                        particles.push_back(p);
                    }
                }
                // سلوك الثلج: يزرع بلورات متجمدة في مساره + صقيع أرضي
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

                        GroundEffect ge;
                        ge.pos = sp.pos;
                        ge.radius = 0.0f;
                        ge.maxRadius = (float)GetRandomValue(20, 36);
                        ge.life = ge.maxLife = 1.2f;
                        ge.color = ColorAlpha(SKYBLUE, 0.5f);
                        ge.type = 2;
                        ge.angle = 0;
                        groundEffects.push_back(ge);
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
                    p.drag = 0.5f;
                    particles.push_back(p);
                }
                // سلوك صخور الأرض: ينبثق حطام وشظايا صلبة + تشققات
                else if (sp.type == ELEM_EARTH) {
                    RockFragment rf;
                    rf.pos = sp.pos;
                    rf.vel = { (float)GetRandomValue(-70, 70), (float)GetRandomValue(-70, 70) };
                    rf.size = (float)GetRandomValue(14, 28);
                    rf.angle = (float)GetRandomValue(0, 360);
                    rf.rotSpeed = (float)GetRandomValue(-180, 180);
                    rf.life = 1.2f;
                    rocks.push_back(rf);

                    if (GetRandomValue(0, 2) == 0) {
                        GroundEffect ge;
                        ge.pos = sp.pos;
                        ge.radius = 0.0f;
                        ge.maxRadius = (float)GetRandomValue(24, 40);
                        ge.life = ge.maxLife = 1.0f;
                        ge.color = ColorAlpha(BROWN, 0.6f);
                        ge.type = 1;
                        ge.angle = (float)GetRandomValue(0, 360);
                        groundEffects.push_back(ge);
                    }
                    // غبار ترابي
                    for (int k = 0; k < 2; k++) {
                        Particle p;
                        p.pos = sp.pos;
                        p.vel = { (float)GetRandomValue(-25, 25), (float)GetRandomValue(-15, 5) };
                        p.color = { 140, 120, 90, 255 };
                        p.size = (float)GetRandomValue(6, 14);
                        p.alpha = 0.5f;
                        p.life = 0.0f;
                        p.maxLife = 0.5f;
                        p.additive = false;
                        p.drag = 0.4f;
                        particles.push_back(p);
                    }
                }
                // سلوك الرياح: شفرات دوارة (إعصار)
                else if (sp.type == ELEM_WIND) {
                    for (int k = 0; k < 4; k++) {
                        Particle p;
                        p.pos = sp.pos;
                        float spiralA = sp.timer * 22.0f + k * (PI * 0.5f);
                        p.vel = { cosf(spiralA) * 110.0f, sinf(spiralA) * 110.0f };
                        p.color = (k % 2 == 0) ? LIME : (Color){ 200, 255, 220, 255 };
                        p.size = (float)GetRandomValue(6, 16);
                        p.alpha = 0.85f;
                        p.life = 0.0f;
                        p.maxLife = 0.22f;
                        p.additive = true;
                        p.drag = 1.5f;
                        particles.push_back(p);
                    }
                }
                // سلوك الماء: موجة جيبية ورذاذ فقاعات + حلقات تموج
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
                        p.drag = 0.6f;
                        particles.push_back(p);
                    }
                    if (GetRandomValue(0, 3) == 0) {
                        GroundEffect ge;
                        ge.pos = sp.pos;
                        ge.radius = 0.0f;
                        ge.maxRadius = (float)GetRandomValue(18, 30);
                        ge.life = ge.maxLife = 0.7f;
                        ge.color = ColorAlpha(SKYBLUE, 0.5f);
                        ge.type = 0;
                        ge.angle = 0;
                        groundEffects.push_back(ge);
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

                        // موجة صدمة عند الإصابة
                        Shockwave sw;
                        sw.pos = en.pos;
                        sw.radius = 0.0f;
                        sw.maxRadius = en.radius * 1.8f;
                        sw.life = sw.maxLife = 0.25f;
                        sw.color = elemColors[sp.type];
                        sw.thickness = 3.0f;
                        shockwaves.push_back(sw);

                        // نص ضرر عائم
                        FloatingText ft;
                        ft.pos = en.pos;
                        snprintf(ft.text, sizeof(ft.text), "-%d", (int)sp.damage);
                        ft.color = elemColors[sp.type];
                        ft.life = ft.maxLife = 0.6f;
                        ft.size = 22.0f;
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
                            p.drag = 0.4f;
                            particles.push_back(p);
                        }

                        if (en.hp <= 0.0f) {
                            en.active = false;

                            // كومبو
                            comboCount++;
                            comboTimer = comboWindow;
                            int mult = 1 + (comboCount / 3);
                            int gained = 100 * mult;
                            score += gained;

                            // نص كومبو إذا أعلى من 1
                            if (comboCount >= 2) {
                                FloatingText ct;
                                ct.pos = Vector2Add(en.pos, { 0, -30 });
                                snprintf(ct.text, sizeof(ct.text), "COMBO x%d", comboCount);
                                ct.color = GOLD;
                                ct.life = ct.maxLife = 0.8f;
                                ct.size = 20.0f;
                                floatTexts.push_back(ct);
                            }

                            // موجة صدمة موت أكبر
                            Shockwave dsw;
                            dsw.pos = en.pos;
                            dsw.radius = 0.0f;
                            dsw.maxRadius = en.radius * 3.0f;
                            dsw.life = dsw.maxLife = 0.4f;
                            dsw.color = elemColors[sp.type];
                            dsw.thickness = 4.0f;
                            shockwaves.push_back(dsw);

                            // انفجار موت أكبر
                            for (int k = 0; k < 26; k++) {
                                Particle p;
                                p.pos = en.pos;
                                float a = (float)GetRandomValue(0, 360) * DEG2RAD;
                                float spd = (float)GetRandomValue(80, 260);
                                p.vel = { cosf(a) * spd, sinf(a) * spd };
                                p.color = (GetRandomValue(0, 1) == 0) ? elemColors[sp.type] : WHITE;
                                p.size = (float)GetRandomValue(5, 14);
                                p.alpha = 1.0f;
                                p.life = 0.0f;
                                p.maxLife = 0.5f;
                                p.additive = true;
                                p.drag = 0.6f;
                                particles.push_back(p);
                            }
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
                groundEffects.clear();
                shockwaves.clear();
                score = 0;
                wave = 1;
                waveTimer = 0.0f;
                comboCount = 0;
                comboTimer = 0.0f;
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
            float dragFactor = 1.0f - Clamp(particles[i].drag * dt, 0.0f, 0.9f);
            particles[i].vel = Vector2Scale(particles[i].vel, dragFactor);
            particles[i].pos = Vector2Add(particles[i].pos, Vector2Scale(particles[i].vel, dt));
            particles[i].life += dt;
            particles[i].alpha = 1.0f - (particles[i].life / particles[i].maxLife);
            particles[i].size *= 0.95f;
            if (particles[i].life >= particles[i].maxLife) particles.erase(particles.begin() + i);
            else i++;
        }

        // تحديث الغبار المحيطي (يتجول ببطء داخل الساحة)
        for (auto &d : ambientDust) {
            d.pos = Vector2Add(d.pos, Vector2Scale(d.vel, dt));
            if (d.pos.x < 0 || d.pos.x > arenaSize) d.vel.x *= -1;
            if (d.pos.y < 0 || d.pos.y > arenaSize) d.vel.y *= -1;
        }

        for (size_t i = 0; i < floatTexts.size();) {
            floatTexts[i].pos.y -= 35.0f * dt;
            floatTexts[i].life -= dt;
            if (floatTexts[i].life <= 0.0f) floatTexts.erase(floatTexts.begin() + i);
            else i++;
        }

        for (size_t i = 0; i < groundEffects.size();) {
            groundEffects[i].life -= dt;
            float t = 1.0f - (groundEffects[i].life / groundEffects[i].maxLife);
            groundEffects[i].radius = groundEffects[i].maxRadius * t;
            if (groundEffects[i].life <= 0.0f) groundEffects.erase(groundEffects.begin() + i);
            else i++;
        }

        for (size_t i = 0; i < shockwaves.size();) {
            shockwaves[i].life -= dt;
            float t = 1.0f - (shockwaves[i].life / shockwaves[i].maxLife);
            shockwaves[i].radius = shockwaves[i].maxRadius * t;
            if (shockwaves[i].life <= 0.0f) shockwaves.erase(shockwaves.begin() + i);
            else i++;
        }

        // تحديث الكاميرا مع اهتزاز الشاشة (Screen Shake) ونبضة التكبير
        camera.target = playerPos;
        camera.zoom = 1.0f + zoomPunch;
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

        // غبار محيطي خافت (طبقة جوية خلف كل شيء)
        for (const auto &d : ambientDust) {
            DrawCircleV(d.pos, d.size, ColorAlpha(d.color, d.alpha));
        }

        // آثار أرضية (صقيع/تشقق/تموج/حرق)
        for (const auto &ge : groundEffects) {
            float t = ge.life / ge.maxLife;
            Color c = ColorAlpha(ge.color, ge.color.a / 255.0f * t);
            if (ge.type == 0) { // ripple
                DrawCircleLines((int)ge.pos.x, (int)ge.pos.y, ge.radius, c);
            } else if (ge.type == 1) { // crack
                for (int k = 0; k < 5; k++) {
                    float a = ge.angle + k * 72.0f;
                    Vector2 end = { ge.pos.x + cosf(a * DEG2RAD) * ge.radius, ge.pos.y + sinf(a * DEG2RAD) * ge.radius };
                    DrawLineEx(ge.pos, end, 2.0f, c);
                }
            } else if (ge.type == 2) { // frost
                DrawCircleV(ge.pos, ge.radius, c);
            } else { // scorch
                DrawCircleV(ge.pos, ge.radius, c);
            }
        }

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
            // ظل خفيف تحت الوحش
            DrawEllipse((int)en.pos.x, (int)(en.pos.y + en.radius * 0.6f), en.radius * 0.9f, en.radius * 0.35f, ColorAlpha(BLACK, 0.35f));
            DrawCircleV(en.pos, en.radius, drawCol);
            DrawCircleLines((int)en.pos.x, (int)en.pos.y, en.radius, BLACK);

            // شريط صحة الوحش
            float barW = en.radius * 2.0f;
            DrawRectangle((int)(en.pos.x - en.radius), (int)(en.pos.y - en.radius - 12), (int)barW, 5, RED);
            DrawRectangle((int)(en.pos.x - en.radius), (int)(en.pos.y - en.radius - 12), (int)(barW * (en.hp / en.maxHp)), 5, GREEN);
        }

        // موجات الصدمة (حلقات متمددة)
        for (const auto &sw : shockwaves) {
            float t = sw.life / sw.maxLife;
            DrawCircleLines((int)sw.pos.x, (int)sw.pos.y, sw.radius, ColorAlpha(sw.color, t));
        }

        // رسم مقذوفات السحر مع هالة توهج
        for (const auto &sp : spells) {
            if (!sp.active) continue;
            DrawCircleV(sp.pos, sp.radius * 2.0f, ColorAlpha(elemColors[sp.type], 0.12f));
            DrawCircleV(sp.pos, sp.radius * 1.3f, ColorAlpha(elemColors[sp.type], 0.35f));
            DrawCircleV(sp.pos, sp.radius, elemColors[sp.type]);
            DrawCircleV(sp.pos, sp.radius * 0.45f, WHITE);
        }

        // رسم الجزيئات بتوهج متراكب (additive) والباقي عادي
        BeginBlendMode(BLEND_ADDITIVE);
        for (const auto &p : particles) {
            if (p.additive) DrawCircleV(p.pos, p.size, ColorAlpha(p.color, p.alpha));
        }
        EndBlendMode();
        for (const auto &p : particles) {
            if (!p.additive) DrawCircleV(p.pos, p.size, ColorAlpha(p.color, p.alpha));
        }

        // هالة الساحر بلون العنصر الحالي (نبض خفيف)
        float auraPulse = 4.0f + sinf(gameTime * 4.0f) * 2.0f;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleV(playerPos, 28.0f + auraPulse, ColorAlpha(elemColors[currentElement], 0.25f));
        EndBlendMode();

        // رسم الساحر (الشخصية)
        DrawCircleV(playerPos, 22.0f, { 45, 52, 90, 255 }); // العباءة
        DrawCircleV(playerPos, 12.0f, { 35, 40, 70, 255 }); // القبعة
        // عصا الساحر موجهة باتجاه الحركة
        Vector2 staffPos = Vector2Add(playerPos, Vector2Scale(playerFacing, 26.0f));
        DrawLineEx(playerPos, staffPos, 6.0f, DARKBROWN);
        DrawCircleV(staffPos, 10.0f, elemColors[currentElement]);
        DrawCircleV(staffPos, 5.0f, WHITE);

        // أرقام الضرر والكومبو العائمة
        for (const auto &ft : floatTexts) {
            float t = ft.life / ft.maxLife;
            Color c = ColorAlpha(ft.color, t);
            DrawText(ft.text, (int)ft.pos.x, (int)ft.pos.y, (int)ft.size, c);
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

        // نقاط القتل والموجة
        DrawText(TextFormat("SCORE: %d", score), 30, (int)(btnY + btnH + 15), 24, GOLD);
        DrawText(TextFormat("WAVE: %d", wave), 30, (int)(btnY + btnH + 45), 20, RAYWHITE);

        // إعلان الموجة الجديدة في وسط الشاشة
        if (waveAnnounceTimer > 0.0f) {
            float a = Clamp(waveAnnounceTimer / 2.2f, 0.0f, 1.0f);
            int tw = MeasureText(waveAnnounceText, 48);
            DrawText(waveAnnounceText, screenWidth / 2 - tw / 2, screenHeight / 2 - 140, 48, ColorAlpha(GOLD, a));
        }

        // فيغنيت أحمر ينبض عند انخفاض الصحة
        if (playerHp < playerMaxHp * 0.3f) {
            float pulse = 0.25f + 0.2f * sinf(gameTime * 6.0f);
            DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(RED, pulse * 0.35f));
        }

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
            DrawText(TextFormat("FINAL SCORE: %d   WAVE: %d", score, wave), screenWidth / 2 - 160, screenHeight / 2 + 70, 28, GOLD);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#if defined(__cplusplus)
}
#endif
