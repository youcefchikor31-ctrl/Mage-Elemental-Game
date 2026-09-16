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
};

struct Spell {
    Vector2 pos;
    Vector2 vel;
    ElementType type;
    bool active;
    float timer;
};

// هيكل للتحكم في الأزرار
struct Button {
    Rectangle rect;
    const char* label;
    Color color;
};

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_MSAA_4X_HINT); // تفعيل مانع التعرج لجودة رسومية فائقة
    InitWindow(screenWidth, screenHeight, "Mage Elements Battle");
    SetTargetFPS(60);

    ElementType currentElement = ELEM_FIRE;
    std::vector<Particle> particles;
    std::vector<Spell> spells;

    Vector2 wizardPos = { 250.0f, 480.0f };
    float castAnimTimer = 0.0f;

    // إعداد أزرار العناصر
    Button elemButtons[5] = {
        { { 30,  40, 150, 60 }, "FIRE",  { 230, 41, 55, 255 } },
        { { 190, 40, 150, 60 }, "ICE",   { 66, 203, 245, 255 } },
        { { 350, 40, 150, 60 }, "EARTH", { 180, 120, 60, 255 } },
        { { 510, 40, 150, 60 }, "WIND",  { 130, 230, 170, 255 } },
        { { 670, 40, 150, 60 }, "WATER", { 30, 110, 230, 255 } }
    };

    // زر الهجوم الكبير في الزاوية
    Rectangle attackBtn = { screenWidth - 220, screenHeight - 140, 180, 90 };

    while (!WindowShouldClose()) {
        // --- 1. معالجة الإدخال واللمس ---
        Vector2 touchPos = GetTouchPosition(0);
        bool touched = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || (GetTouchPointCount() > 0 && IsGestureDetected(GESTURE_TAP));
        if (touched) {
            Vector2 inputPos = (GetTouchPointCount() > 0) ? touchPos : GetMousePosition();

            for (int i = 0; i < 5; i++) {
                if (CheckCollisionPointRec(inputPos, elemButtons[i].rect)) {
                    currentElement = (ElementType)i;
                }
            }

            if (CheckCollisionPointRec(inputPos, attackBtn)) {
                castAnimTimer = 0.35f; // تفعيل حركة الهجوم
                Spell s;
                s.pos = { wizardPos.x + 85.0f, wizardPos.y - 45.0f };
                s.vel = { 16.0f, 0.0f };
                s.type = currentElement;
                s.active = true;
                s.timer = 0.0f;
                spells.push_back(s);
            }
        }

        // --- 2. تحديث حركة المقذوفات والجزيئات ---
        if (castAnimTimer > 0.0f) castAnimTimer -= GetFrameTime();

        // تحديث هجمات السحر
        for (auto &s : spells) {
            if (!s.active) continue;
            s.pos = Vector2Add(s.pos, s.vel);
            s.timer += GetFrameTime();

            // توليد جزيئات ملونة خلف الهجمة
            for (int i = 0; i < 6; i++) {
                Particle p;
                p.pos = s.pos;
                p.vel = { (float)GetRandomValue(-20, 10) / 10.0f, (float)GetRandomValue(-20, 20) / 10.0f };
                p.size = (float)GetRandomValue(8, 20);
                p.life = 0.0f;
                p.maxLife = (float)GetRandomValue(20, 45) / 100.0f;
                p.alpha = 1.0f;

                switch (s.type) {
                    case ELEM_FIRE:
                        p.color = (GetRandomValue(0, 1) == 0) ? ORANGE : RED;
                        p.vel.y -= 1.2f; // النار تصعد للأعلى
                        break;
                    case ELEM_ICE:
                        p.color = (GetRandomValue(0, 1) == 0) ? SKYBLUE : WHITE;
                        break;
                    case ELEM_EARTH:
                        p.color = (GetRandomValue(0, 1) == 0) ? DARKBROWN : BROWN;
                        p.vel.y += 1.8f; // الصخور تهبط
                        break;
                    case ELEM_WIND:
                        p.color = (GetRandomValue(0, 1) == 0) ? LIME : RAYWHITE;
                        p.vel.x += (float)GetRandomValue(-4, 4);
                        break;
                    case ELEM_WATER:
                        p.color = (GetRandomValue(0, 1) == 0) ? BLUE : DARKBLUE;
                        p.vel.y += sinf(s.timer * 8.0f) * 2.0f;
                        break;
                }
                particles.push_back(p);
            }

            if (s.pos.x > screenWidth + 50) s.active = false;
        }

        // تحديث وتلاشي الجزيئات (Particles)
        for (size_t i = 0; i < particles.size();) {
            particles[i].pos = Vector2Add(particles[i].pos, particles[i].vel);
            particles[i].life += GetFrameTime();
            particles[i].alpha = 1.0f - (particles[i].life / particles[i].maxLife);
            particles[i].size *= 0.96f;

            if (particles[i].life >= particles[i].maxLife || particles[i].size <= 1.0f) {
                particles.erase(particles.begin() + i);
            } else {
                i++;
            }
        }

        // --- 3. الرسم (Rendering) ---
        BeginDrawing();
        ClearBackground({ 18, 20, 32, 255 }); // خلفية ليلية داكنة لإبراز السحر

        // رسم أرضية الحلبة
        DrawRectangle(0, screenHeight - 120, screenWidth, 120, { 28, 30, 45, 255 });
        DrawLine(0, screenHeight - 120, screenWidth, screenHeight - 120, { 70, 75, 110, 255 });

        // رسم شخصية الساحر (بنائياً)
        Color cloakColor = { 45, 48, 85, 255 };
        DrawTriangle({ wizardPos.x, wizardPos.y - 80 }, { wizardPos.x - 45, wizardPos.y + 70 }, { wizardPos.x + 45, wizardPos.y + 70 }, cloakColor);
        DrawCircle(wizardPos.x, wizardPos.y - 85, 26, { 35, 38, 70, 255 }); // القبعة والرأس
        DrawCircle(wizardPos.x + 8, wizardPos.y - 88, 5, YELLOW); // عين متوهجة

        // رسم عصا الساحر
        float staffOffset = (castAnimTimer > 0.0f) ? 15.0f : 0.0f;
        Vector2 staffTop = { wizardPos.x + 65.0f + staffOffset, wizardPos.y - 50.0f - staffOffset };
        DrawLineEx({ wizardPos.x + 45.0f, wizardPos.y + 60.0f }, staffTop, 7.0f, DARKBROWN);

        // جوهرة العصا المشعة بلون السحر الحالي
        Color gemColor = elemButtons[currentElement].color;
        DrawCircleV(staffTop, 16.0f, ColorAlpha(gemColor, 0.4f));
        DrawCircleV(staffTop, 10.0f, gemColor);
        DrawCircleV(staffTop, 5.0f, WHITE);

        // رسم المقذوفات السحرية
        for (const auto &s : spells) {
            if (!s.active) continue;
            DrawCircleGradient(s.pos.x, s.pos.y, 22, elemButtons[s.type].color, ColorAlpha(WHITE, 0.2f));
            DrawCircleV(s.pos, 12, WHITE);
        }

        // رسم الجزيئات بتأثير التوهج
        BeginBlendMode(BLEND_ADDITIVE);
        for (const auto &p : particles) {
            DrawCircleV(p.pos, p.size, ColorAlpha(p.color, p.alpha));
        }
        EndBlendMode();

        // رسم واجهة الأزرار (UI)
        for (int i = 0; i < 5; i++) {
            bool selected = (currentElement == (ElementType)i);
            DrawRectangleRec(elemButtons[i].rect, selected ? elemButtons[i].color : ColorAlpha(elemButtons[i].color, 0.35f));
            DrawRectangleLinesEx(elemButtons[i].rect, selected ? 4.0f : 1.5f, WHITE);
            DrawText(elemButtons[i].label, elemButtons[i].rect.x + 35, elemButtons[i].rect.y + 18, 22, WHITE);
        }

        // زر إطلاق الهجوم
        DrawRectangleRec(attackBtn, { 220, 50, 50, 255 });
        DrawRectangleLinesEx(attackBtn, 4.0f, GOLD);
        DrawText("ATTACK!", attackBtn.x + 30, attackBtn.y + 30, 28, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
