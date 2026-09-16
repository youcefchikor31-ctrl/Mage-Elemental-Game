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

    ElementType currentElement = ELEM_FIRE;
    std::vector<Particle> particles;
    std::vector<Spell> spells;

    Vector2 wizardPos = { screenWidth * 0.18f, screenHeight * 0.68f };
    float castAnimTimer = 0.0f;

    float btnWidth = screenWidth * 0.14f;
    float btnHeight = screenHeight * 0.11f;
    float btnY = screenHeight * 0.05f;

    Button elemButtons[5] = {
        { { screenWidth * 0.03f, btnY, btnWidth, btnHeight }, "FIRE",  { 230, 41, 55, 255 } },
        { { screenWidth * 0.19f, btnY, btnWidth, btnHeight }, "ICE",   { 66, 203, 245, 255 } },
        { { screenWidth * 0.35f, btnY, btnWidth, btnHeight }, "EARTH", { 180, 120, 60, 255 } },
        { { screenWidth * 0.51f, btnY, btnWidth, btnHeight }, "WIND",  { 130, 230, 170, 255 } },
        { { screenWidth * 0.67f, btnY, btnWidth, btnHeight }, "WATER", { 30, 110, 230, 255 } }
    };

    Rectangle attackBtn = { screenWidth - (screenWidth * 0.22f), screenHeight - (screenHeight * 0.22f), screenWidth * 0.18f, screenHeight * 0.16f };

    while (!WindowShouldClose()) {
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
                s.pos = { wizardPos.x + 85.0f, wizardPos.y - 45.0f };
                s.vel = { screenWidth * 0.018f, 0.0f };
                s.type = currentElement;
                s.active = true;
                s.timer = 0.0f;
                spells.push_back(s);
            }
        }

        if (castAnimTimer > 0.0f) castAnimTimer -= GetFrameTime();

        for (auto &s : spells) {
            if (!s.active) continue;
            s.pos = Vector2Add(s.pos, s.vel);
            s.timer += GetFrameTime();

            for (int i = 0; i < 5; i++) {
                Particle p;
                p.pos = s.pos;
                p.vel = { (float)GetRandomValue(-20, 10) / 10.0f, (float)GetRandomValue(-20, 20) / 10.0f };
                p.size = (float)GetRandomValue(10, 24);
                p.life = 0.0f;
                p.maxLife = (float)GetRandomValue(25, 50) / 100.0f;
                p.alpha = 1.0f;

                switch (s.type) {
                    case ELEM_FIRE:
                        p.color = (GetRandomValue(0, 1) == 0) ? ORANGE : RED;
                        p.vel.y -= 1.4f;
                        break;
                    case ELEM_ICE:
                        p.color = (GetRandomValue(0, 1) == 0) ? SKYBLUE : WHITE;
                        break;
                    case ELEM_EARTH:
                        p.color = (GetRandomValue(0, 1) == 0) ? DARKBROWN : BROWN;
                        p.vel.y += 2.0f;
                        break;
                    case ELEM_WIND:
                        p.color = (GetRandomValue(0, 1) == 0) ? LIME : RAYWHITE;
                        p.vel.x += (float)GetRandomValue(-5, 5);
                        break;
                    case ELEM_WATER:
                        p.color = (GetRandomValue(0, 1) == 0) ? BLUE : DARKBLUE;
                        p.vel.y += sinf(s.timer * 8.0f) * 2.5f;
                        break;
                }
                particles.push_back(p);
            }

            if (s.pos.x > screenWidth + 60) s.active = false;
        }

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

        BeginDrawing();
        ClearBackground({ 15, 17, 26, 255 });

        DrawRectangle(0, screenHeight * 0.78f, screenWidth, screenHeight * 0.22f, { 25, 27, 40, 255 });
        DrawLine(0, screenHeight * 0.78f, screenWidth, screenHeight * 0.78f, { 60, 65, 95, 255 });

        Color cloakColor = { 50, 55, 95, 255 };
        DrawTriangle({ wizardPos.x, wizardPos.y - 90 }, { wizardPos.x - 50, wizardPos.y + 75 }, { wizardPos.x + 50, wizardPos.y + 75 }, cloakColor);
        DrawCircle((int)wizardPos.x, (int)wizardPos.y - 95, 28, { 40, 44, 75, 255 });
        DrawCircle((int)wizardPos.x + 10, (int)wizardPos.y - 98, 6, YELLOW);

        float staffOffset = (castAnimTimer > 0.0f) ? 18.0f : 0.0f;
        Vector2 staffTop = { wizardPos.x + 70.0f + staffOffset, wizardPos.y - 55.0f - staffOffset };
        DrawLineEx({ wizardPos.x + 50.0f, wizardPos.y + 65.0f }, staffTop, 8.0f, DARKBROWN);

        Color gemColor = elemButtons[currentElement].color;
        DrawCircleV(staffTop, 20.0f, ColorAlpha(gemColor, 0.4f));
        DrawCircleV(staffTop, 12.0f, gemColor);
        DrawCircleV(staffTop, 6.0f, WHITE);

        // رسم مقذوفات السحر بطبقات توهج شعاعي متوافقة
        for (const auto &s : spells) {
            if (!s.active) continue;
            DrawCircleV(s.pos, 24.0f, ColorAlpha(elemButtons[s.type].color, 0.35f));
            DrawCircleV(s.pos, 16.0f, elemButtons[s.type].color);
            DrawCircleV(s.pos, 8.0f, WHITE);
        }

        BeginBlendMode(BLEND_ADDITIVE);
        for (const auto &p : particles) {
            DrawCircleV(p.pos, p.size, ColorAlpha(p.color, p.alpha));
        }
        EndBlendMode();

        for (int i = 0; i < 5; i++) {
            bool selected = (currentElement == (ElementType)i);
            DrawRectangleRec(elemButtons[i].rect, selected ? elemButtons[i].color : ColorAlpha(elemButtons[i].color, 0.35f));
            DrawRectangleLinesEx(elemButtons[i].rect, selected ? 4.0f : 2.0f, WHITE);
            DrawText(elemButtons[i].label, (int)(elemButtons[i].rect.x + 20), (int)(elemButtons[i].rect.y + 18), 22, WHITE);
        }

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
