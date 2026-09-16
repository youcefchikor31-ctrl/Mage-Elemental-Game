#include "raylib.h"
#include "raymath.h"
#include <vector>
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
    bool hasGravity;
};

// أشواك الثلج وصخور الأرض المنبثقة
struct GroundFormation {
    Vector2 basePos;
    float currentHeight;
    float targetHeight;
    float width;
    float angleOffset;
    Color color;
    Color highlightColor;
    bool isIce; // true للثلج، false لصخور الأرض
    float life;
};

struct Spell {
    Vector2 pos;
    Vector2 vel;
    ElementType type;
    bool active;
    float timer;
    float lastGroundSpawnX; // لتعقب مسافة تفجير الأرض
};

struct Button {
    Rectangle rect;
    const char* label;
    Color color;
};

#if defined(__cplusplus)
extern "C" {
#endif

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    InitWindow(0, 0, "Mage Elements Battle");
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    SetTargetFPS(60);

    float groundY = screenHeight * 0.76f;
    Vector2 wizardPos = { screenWidth * 0.16f, groundY - 20.0f };
    Vector2 targetPos = { screenWidth * 0.84f, groundY - 30.0f };

    ElementType currentElement = ELEM_FIRE;
    std::vector<Particle> particles;
    std::vector<GroundFormation> groundFormations;
    std::vector<Spell> spells;

    float castAnimTimer = 0.0f;
    float targetHitFlash = 0.0f;

    // أزرار العناصر العلوية
    float btnWidth = screenWidth * 0.14f;
    float btnHeight = screenHeight * 0.11f;
    float btnY = screenHeight * 0.04f;

    Button elemButtons[5] = {
        { { screenWidth * 0.03f, btnY, btnWidth, btnHeight }, "FIRE",  { 240, 60, 20, 255 } },
        { { screenWidth * 0.19f, btnY, btnWidth, btnHeight }, "ICE",   { 80, 210, 255, 255 } },
        { { screenWidth * 0.35f, btnY, btnWidth, btnHeight }, "EARTH", { 160, 110, 50, 255 } },
        { { screenWidth * 0.51f, btnY, btnWidth, btnHeight }, "WIND",  { 130, 240, 180, 255 } },
        { { screenWidth * 0.67f, btnY, btnWidth, btnHeight }, "WATER", { 30, 130, 255, 255 } }
    };

    Rectangle attackBtn = { screenWidth - (screenWidth * 0.22f), screenHeight - (screenHeight * 0.20f), screenWidth * 0.18f, screenHeight * 0.15f };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // 1. الإدخال واللمس
        if (GetTouchPointCount() > 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 inputPos = (GetTouchPointCount() > 0) ? GetTouchPosition(0) : GetMousePosition();

            for (int i = 0; i < 5; i++) {
                if (CheckCollisionPointRec(inputPos, elemButtons[i].rect)) {
                    currentElement = (ElementType)i;
                }
            }

            if (CheckCollisionPointRec(inputPos, attackBtn)) {
                castAnimTimer = 0.35f;
                Spell s;
                s.type = currentElement;
                s.active = true;
                s.timer = 0.0f;

                if (currentElement == ELEM_ICE || currentElement == ELEM_EARTH) {
                    // زحف أرضي مباشر لتفجير الصخور والبلورات
                    s.pos = { wizardPos.x + 40.0f, groundY };
                    s.vel = { screenWidth * 0.022f, 0.0f };
                    s.lastGroundSpawnX = s.pos.x;
                } else {
                    // مقذوفات تنطلق من رأس العصا
                    s.pos = { wizardPos.x + 75.0f, wizardPos.y - 70.0f };
                    s.vel = { screenWidth * 0.019f, 0.0f };
                    s.lastGroundSpawnX = 0;
                }
                spells.push_back(s);
            }
        }

        if (castAnimTimer > 0.0f) castAnimTimer -= dt;
        if (targetHitFlash > 0.0f) targetHitFlash -= dt;

        // 2. تحديث السحر والفيزياء
        for (auto &s : spells) {
            if (!s.active) continue;
            s.pos = Vector2Add(s.pos, s.vel);
            s.timer += dt;

            // --- سلوك النار (Fire Vortex) ---
            if (s.type == ELEM_FIRE) {
                for (int i = 0; i < 9; i++) {
                    Particle p;
                    p.pos = { s.pos.x + (float)GetRandomValue(-12, 12), s.pos.y + (float)GetRandomValue(-12, 12) };
                    p.vel = { (float)GetRandomValue(-25, -5) / 5.0f, (float)GetRandomValue(-40, 10) / 10.0f }; // تصاعد ألسنة اللهب
                    p.color = (GetRandomValue(0, 3) == 0) ? YELLOW : ((GetRandomValue(0, 1) == 0) ? ORANGE : RED);
                    p.size = (float)GetRandomValue(14, 32);
                    p.alpha = 1.0f;
                    p.life = 0.0f;
                    p.maxLife = (float)GetRandomValue(25, 45) / 100.0f;
                    p.hasGravity = false;
                    particles.push_back(p);
                }
                // دخان رمادي يتصاعد
                Particle smoke;
                smoke.pos = s.pos;
                smoke.vel = { (float)GetRandomValue(-15, -5) / 10.0f, -1.8f };
                smoke.color = { 70, 70, 75, 255 };
                smoke.size = (float)GetRandomValue(18, 36);
                smoke.alpha = 0.6f;
                smoke.life = 0.0f;
                smoke.maxLife = 0.65f;
                smoke.hasGravity = false;
                particles.push_back(smoke);
            }

            // --- سلوك الثلج: تفجير بلورات وأشواك من الأرض متتابعة نحو الخصم ---
            else if (s.type == ELEM_ICE) {
                if (s.pos.x - s.lastGroundSpawnX >= 36.0f) {
                    s.lastGroundSpawnX = s.pos.x;
                    GroundFormation gf;
                    gf.basePos = { s.pos.x, groundY };
                    gf.currentHeight = 0.0f;
                    gf.targetHeight = (float)GetRandomValue((int)(screenHeight * 0.10f), (int)(screenHeight * 0.18f));
                    gf.width = (float)GetRandomValue(18, 30);
                    gf.angleOffset = (float)GetRandomValue(-10, 15);
                    gf.color = { 100, 215, 255, 255 };
                    gf.highlightColor = { 220, 245, 255, 255 };
                    gf.isIce = true;
                    gf.life = 2.0f; // تبقى بارزة قليلاً ثم تذوب
                    groundFormations.push_back(gf);

                    // شظايا وتوهج جليدي متطاير
                    for (int k = 0; k < 7; k++) {
                        Particle p;
                        p.pos = { s.pos.x, groundY };
                        p.vel = { (float)GetRandomValue(-20, 20) / 10.0f, (float)GetRandomValue(-35, -10) / 10.0f };
                        p.color = (GetRandomValue(0, 1) == 0) ? SKYBLUE : WHITE;
                        p.size = (float)GetRandomValue(5, 12);
                        p.alpha = 1.0f;
                        p.life = 0.0f;
                        p.maxLife = 0.5f;
                        p.hasGravity = true;
                        particles.push_back(p);
                    }
                }
            }

            // --- سلوك صخور الأرض: تمزيق الأرض وانبثاق كتل صلبة متدحرجة ---
            else if (s.type == ELEM_EARTH) {
                if (s.pos.x - s.lastGroundSpawnX >= 42.0f) {
                    s.lastGroundSpawnX = s.pos.x;
                    GroundFormation gf;
                    gf.basePos = { s.pos.x, groundY };
                    gf.currentHeight = 0.0f;
                    gf.targetHeight = (float)GetRandomValue((int)(screenHeight * 0.08f), (int)(screenHeight * 0.14f));
                    gf.width = (float)GetRandomValue(25, 45);
                    gf.angleOffset = (float)GetRandomValue(-25, 25);
                    gf.color = { 110, 80, 50, 255 };
                    gf.highlightColor = { 150, 115, 75, 255 };
                    gf.isIce = false;
                    gf.life = 2.4f;
                    groundFormations.push_back(gf);

                    // صخور صغيرة وغبار يتناثر للأعلى
                    for (int k = 0; k < 10; k++) {
                        Particle p;
                        p.pos = { s.pos.x, groundY };
                        p.vel = { (float)GetRandomValue(-30, 30) / 10.0f, (float)GetRandomValue(-45, -15) / 10.0f };
                        p.color = (GetRandomValue(0, 1) == 0) ? DARKBROWN : BROWN;
                        p.size = (float)GetRandomValue(6, 16);
                        p.alpha = 1.0f;
                        p.life = 0.0f;
                        p.maxLife = 0.7f;
                        p.hasGravity = true;
                        particles.push_back(p);
                    }
                }
            }

            // --- سلوك الماء: تيار وموجة تتدفق بذبذبة جيبية ورذاذ مائي ---
            else if (s.type == ELEM_WATER) {
                s.pos.y += sinf(s.timer * 14.0f) * 3.5f;
                for (int i = 0; i < 8; i++) {
                    Particle p;
                    p.pos = { s.pos.x + (float)GetRandomValue(-8, 8), s.pos.y + (float)GetRandomValue(-10, 10) };
                    p.vel = { (float)GetRandomValue(-20, 0) / 10.0f, (float)GetRandomValue(-15, 25) / 10.0f };
                    p.color = (GetRandomValue(0, 2) == 0) ? WHITE : ((GetRandomValue(0, 1) == 0) ? SKYBLUE : BLUE);
                    p.size = (float)GetRandomValue(10, 22);
                    p.alpha = 0.9f;
                    p.life = 0.0f;
                    p.maxLife = 0.4f;
                    p.hasGravity = true;
                    particles.push_back(p);
                }
            }

            // --- سلوك الرياح: شفرات وهوامش إعصار حلزونية تدور بسرعة ---
            else if (s.type == ELEM_WIND) {
                for (int i = 0; i < 7; i++) {
                    float orbit = s.timer * 22.0f + (i * 1.0f);
                    Particle p;
                    p.pos = { s.pos.x + cosf(orbit) * 26.0f, s.pos.y + sinf(orbit) * 26.0f };
                    p.vel = { (float)GetRandomValue(10, 25) / 10.0f, (float)GetRandomValue(-10, 10) / 10.0f };
                    p.color = (GetRandomValue(0, 1) == 0) ? RAYWHITE : LIME;
                    p.size = (float)GetRandomValue(8, 18);
                    p.alpha = 0.8f;
                    p.life = 0.0f;
                    p.maxLife = 0.35f;
                    p.hasGravity = false;
                    particles.push_back(p);
                }
            }

            // فحص الاصطدام بالهدف
            if (CheckCollisionCircles(s.pos, 35.0f, targetPos, 45.0f)) {
                s.active = false;
                targetHitFlash = 0.25f;

                // انفجار تأثير عند الإصابة
                for (int j = 0; j < 35; j++) {
                    Particle p;
                    p.pos = targetPos;
                    p.vel = { (float)GetRandomValue(-40, 40) / 10.0f, (float)GetRandomValue(-40, 20) / 10.0f };
                    p.color = elemButtons[s.type].color;
                    p.size = (float)GetRandomValue(10, 24);
                    p.alpha = 1.0f;
                    p.life = 0.0f;
                    p.maxLife = 0.6f;
                    p.hasGravity = true;
                    particles.push_back(p);
                }
            }

            if (s.pos.x > screenWidth + 80) s.active = false;
        }

        // تحديث نمو وتلاشي الصخور وبلورات الجليد
        for (size_t i = 0; i < groundFormations.size();) {
            if (groundFormations[i].currentHeight < groundFormations[i].targetHeight) {
                groundFormations[i].currentHeight += dt * 450.0f; // انبثاق سريع للأعلى
                if (groundFormations[i].currentHeight > groundFormations[i].targetHeight)
                    groundFormations[i].currentHeight = groundFormations[i].targetHeight;
            }

            groundFormations[i].life -= dt;
            if (groundFormations[i].life <= 0.0f) {
                groundFormations.erase(groundFormations.begin() + i);
            } else {
                i++;
            }
        }

        // تحديث الجزيئات
        for (size_t i = 0; i < particles.size();) {
            particles[i].pos = Vector2Add(particles[i].pos, particles[i].vel);
            if (particles[i].hasGravity) particles[i].vel.y += 0.25f; // قوة الجاذبية
            particles[i].life += dt;
            particles[i].alpha = 1.0f - (particles[i].life / particles[i].maxLife);
            particles[i].size *= 0.965f;

            if (particles[i].life >= particles[i].maxLife || particles[i].size <= 1.0f) {
                particles.erase(particles.begin() + i);
            } else {
                i++;
            }
        }

        // 3. الرسم والعرض (Rendering)
        BeginDrawing();
        ClearBackground({ 14, 16, 25, 255 });

        // رسم خلفية الأرضية
        DrawRectangle(0, groundY, screenWidth, screenHeight - groundY, { 22, 24, 35, 255 });
        DrawLineEx({ 0, groundY }, { (float)screenWidth, groundY }, 3.0f, { 55, 60, 85, 255 });

        // رسم تكوينات الأرض والجليد المنبثقة
        for (const auto &gf : groundFormations) {
            Vector2 p1 = { gf.basePos.x - gf.width * 0.5f, groundY };
            Vector2 p2 = { gf.basePos.x + gf.width * 0.5f, groundY };
            Vector2 tip = { gf.basePos.x + gf.angleOffset, groundY - gf.currentHeight };

            if (gf.isIce) {
                // رسم بلورة ثلجية حادة ثلاثية الأبعاد
                DrawTriangle(tip, p1, { gf.basePos.x, groundY }, gf.color);
                DrawTriangle(tip, { gf.basePos.x, groundY }, p2, gf.highlightColor);
                DrawLineEx(tip, { gf.basePos.x, groundY }, 2.0f, WHITE);
            } else {
                // رسم كتلة صخرية بازلتية متشققة
                DrawTriangle(tip, p1, p2, gf.color);
                DrawTriangle(tip, { gf.basePos.x - gf.width * 0.2f, groundY }, p2, gf.highlightColor);
                DrawLineEx(p1, tip, 2.5f, BLACK);
                DrawLineEx(tip, p2, 2.5f, BLACK);
            }
        }

        // رسم الساحر
        Color cloakColor = { 50, 54, 90, 255 };
        DrawTriangle({ wizardPos.x, wizardPos.y - 85 }, { wizardPos.x - 45, wizardPos.y + 20 }, { wizardPos.x + 45, wizardPos.y + 20 }, cloakColor);
        DrawCircle((int)wizardPos.x, (int)wizardPos.y - 90, 26, { 38, 42, 70, 255 });
        DrawCircle((int)wizardPos.x + 8, (int)wizardPos.y - 94, 5, YELLOW); // عين متوهجة

        // رسم عصا الساحر
        float staffOffset = (castAnimTimer > 0.0f) ? 16.0f : 0.0f;
        Vector2 staffTop = { wizardPos.x + 65.0f + staffOffset, wizardPos.y - 50.0f - staffOffset };
        DrawLineEx({ wizardPos.x + 45.0f, wizardPos.y + 15.0f }, staffTop, 7.0f, DARKBROWN);

        Color gemColor = elemButtons[currentElement].color;
        DrawCircleV(staffTop, 18.0f, ColorAlpha(gemColor, 0.35f));
        DrawCircleV(staffTop, 10.0f, gemColor);
        DrawCircleV(staffTop, 5.0f, WHITE);

        // رسم الخصم / دمية التدريب
        Color dummyColor = (targetHitFlash > 0.0f) ? WHITE : Color{ 140, 45, 45, 255 };
        DrawRectangle((int)targetPos.x - 22, (int)targetPos.y - 65, 44, 85, dummyColor);
        DrawCircle((int)targetPos.x, (int)targetPos.y - 85, 22, dummyColor);
        DrawRectangle((int)targetPos.x - 30, (int)targetPos.y - 20, 60, 10, DARKBROWN); // عمود التثبيت
        DrawLineEx({ targetPos.x, targetPos.y + 20 }, { targetPos.x, groundY }, 8.0f, DARKBROWN);

        // رسم مقذوفات الهواء والماء والنار
        for (const auto &s : spells) {
            if (!s.active) continue;
            if (s.type == ELEM_FIRE) {
                DrawCircleV(s.pos, 26.0f, ColorAlpha(ORANGE, 0.45f));
                DrawCircleV(s.pos, 16.0f, RED);
                DrawCircleV(s.pos, 9.0f, WHITE); // النواة الساخنة
            } else if (s.type == ELEM_WATER) {
                DrawCircleV(s.pos, 22.0f, ColorAlpha(BLUE, 0.5f));
                DrawCircleV(s.pos, 14.0f, SKYBLUE);
                DrawCircleV(s.pos, 7.0f, WHITE);
            } else if (s.type == ELEM_WIND) {
                DrawRing(s.pos, 12.0f, 24.0f, 0, 360, 16, ColorAlpha(LIME, 0.5f));
                DrawCircleV(s.pos, 8.0f, WHITE);
            }
        }

        // رسم الجزيئات بتأثير الوهج المشع (Additive Glow)
        BeginBlendMode(BLEND_ADDITIVE);
        for (const auto &p : particles) {
            DrawCircleV(p.pos, p.size, ColorAlpha(p.color, p.alpha));
        }
        EndBlendMode();

        // رسم واجهة الأزرار العلوية
        for (int i = 0; i < 5; i++) {
            bool selected = (currentElement == (ElementType)i);
            DrawRectangleRec(elemButtons[i].rect, selected ? elemButtons[i].color : ColorAlpha(elemButtons[i].color, 0.35f));
            DrawRectangleLinesEx(elemButtons[i].rect, selected ? 4.0f : 2.0f, WHITE);
            DrawText(elemButtons[i].label, (int)(elemButtons[i].rect.x + 20), (int)(elemButtons[i].rect.y + 18), 22, WHITE);
        }

        // زر الهجوم (ATTACK)
        DrawRectangleRec(attackBtn, { 225, 45, 45, 255 });
        DrawRectangleLinesEx(attackBtn, 4.0f, GOLD);
        DrawText("ATTACK", (int)(attackBtn.x + (attackBtn.width * 0.18f)), (int)(attackBtn.y + (attackBtn.height * 0.32f)), 28, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#if defined(__cplusplus)
}
#endif
