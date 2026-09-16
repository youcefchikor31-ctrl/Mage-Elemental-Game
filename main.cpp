#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>

struct Obstacle {
    Vector3 pos;
    float radius;
    bool isRock; // true للصخور، false لكومات الحشيش الكثيفة
    Color color;
};

struct Spell3D {
    Vector3 pos;
    Vector3 vel;
    Color color;
    bool active;
    float life;
};

#if defined(__cplusplus)
extern "C" {
#endif

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    InitWindow(0, 0, "Mage 3D Adventure");
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    SetTargetFPS(60);

    // إعداد الكاميرا ثلاثية الأبعاد بمنظور علوي (Top-Down)
    Camera3D camera = { 0 };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // موقع الساحر في الفضاء 3D
    Vector3 playerPos = { 0.0f, 0.0f, 0.0f };
    float playerRadius = 0.6f;
    float playerSpeed = 7.5f;

    // إعداد ذراع التحكم التخيلي (Virtual Joystick)
    Vector2 stickCenter = { screenWidth * 0.15f, screenHeight * 0.75f };
    float stickBaseRadius = screenHeight * 0.12f;
    float stickKnobRadius = screenHeight * 0.05f;
    Vector2 knobPos = stickCenter;
    bool stickActive = false;

    // زر الهجوم السحري
    Rectangle attackBtn = { screenWidth - (screenWidth * 0.20f), screenHeight - (screenHeight * 0.22f), screenWidth * 0.15f, screenHeight * 0.16f };

    // توليد كومات حشيش وصخور في العالم مع إحداثيات تصادم
    std::vector<Obstacle> obstacles;
    obstacles.push_back({ { 4.0f, 0.0f, 3.0f }, 1.4f, false, Color{ 34, 139, 34, 255 } });   // كومة حشيش
    obstacles.push_back({ { -5.0f, 0.0f, -2.0f }, 1.6f, false, Color{ 28, 120, 28, 255 } }); // كومة حشيش
    obstacles.push_back({ { 2.0f, 0.0f, -6.0f }, 1.5f, false, Color{ 30, 130, 30, 255 } });  // كومة حشيش
    obstacles.push_back({ { -3.0f, 0.0f, 5.0f }, 1.2f, true, Color{ 110, 115, 125, 255 } }); // صخرة صلبة
    obstacles.push_back({ { 6.0f, 0.0f, -4.0f }, 1.8f, true, Color{ 90, 95, 105, 255 } });   // صخرة ضخمة

    std::vector<Spell3D> spells;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // 1. معالجة لمس الشاشة وذراع التحكم
        Vector2 moveDir = { 0.0f, 0.0f };
        int touchCount = GetTouchPointCount();

        stickActive = false;
        knobPos = stickCenter;

        for (int i = 0; i < touchCount || (touchCount == 0 && IsMouseButtonDown(MOUSE_BUTTON_LEFT)); i++) {
            Vector2 touchPos = (touchCount > 0) ? GetTouchPosition(i) : GetMousePosition();

            // فحص لمس منطقة عصا التحكم
            if (CheckCollisionPointCircle(touchPos, stickCenter, stickBaseRadius * 1.5f)) {
                stickActive = true;
                Vector2 diff = Vector2Subtract(touchPos, stickCenter);
                float dist = Vector2Length(diff);

                if (dist > stickBaseRadius) {
                    diff = Vector2Scale(Vector2Normalize(diff), stickBaseRadius);
                }
                knobPos = Vector2Add(stickCenter, diff);
                moveDir = Vector2Scale(diff, 1.0f / stickBaseRadius);
            }

            // فحص زر إطلاق السحر
            if (CheckCollisionPointRec(touchPos, attackBtn)) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsGestureDetected(GESTURE_TAP)) {
                    Spell3D sp;
                    sp.pos = (Vector3){ playerPos.x, 0.8f, playerPos.z };
                    sp.vel = (Vector3){ 0.0f, 0.0f, -18.0f }; // قذف للأمام
                    sp.color = ORANGE;
                    sp.active = true;
                    sp.life = 2.0f;
                    spells.push_back(sp);
                }
            }
        }

        // 2. تحديث حركة الساحر واكتشاف الاصطدام بالعوائق (Collision Resolution)
        Vector3 newPos = playerPos;
        newPos.x += moveDir.x * playerSpeed * dt;
        newPos.z += moveDir.y * playerSpeed * dt;

        for (const auto &obs : obstacles) {
            float dist = Vector2Distance({ newPos.x, newPos.z }, { obs.pos.x, obs.pos.z });
            float minDist = playerRadius + obs.radius;

            // إذا وقع اصطدام، ادفع اللاعب للخارج لمنع المرور وسطه
            if (dist < minDist) {
                Vector2 pushDir = Vector2Normalize(Vector2Subtract({ newPos.x, newPos.z }, { obs.pos.x, obs.pos.z }));
                Vector2 resolved = Vector2Add({ obs.pos.x, obs.pos.z }, Vector2Scale(pushDir, minDist));
                newPos.x = resolved.x;
                newPos.z = resolved.y;
            }
        }
        playerPos = newPos;

        // تحديث وضع الكاميرا لتتبع اللاعب بزاوية علوية
        camera.position = (Vector3){ playerPos.x, 14.0f, playerPos.z + 12.0f };
        camera.target = (Vector3){ playerPos.x, 0.0f, playerPos.z };

        // تحديث مقذوفات السحر 3D
        for (auto &sp : spells) {
            if (!sp.active) continue;
            sp.pos = Vector3Add(sp.pos, Vector3Scale(sp.vel, dt));
            sp.life -= dt;
            if (sp.life <= 0.0f) sp.active = false;
        }

        // 3. الرسم والعرض ثلاثي الأبعاد
        BeginDrawing();
        ClearBackground({ 12, 16, 26, 255 });

        BeginMode3D(camera);

        // رسم أرضية عشبية واسعة
        DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 80.0f, 80.0f }, Color{ 22, 60, 32, 255 });
        DrawGrid(40, 2.0f);

        // رسم كومات الحشيش والصخور
        for (const auto &obs : obstacles) {
            if (obs.isRock) {
                // رسم صخرة مجسمة
                DrawSphere(obs.pos, obs.radius, obs.color);
                DrawSphereWires(obs.pos, obs.radius + 0.02f, 6, 6, Color{ 40, 45, 55, 255 });
            } else {
                // رسم كومة حشيش كثيفة (قاعدة عريضة مع تفريعات متداخلة)
                DrawCylinder(obs.pos, obs.radius * 0.85f, obs.radius, 1.2f, 8, obs.color);
                DrawSphere((Vector3){ obs.pos.x, obs.pos.y + 0.8f, obs.pos.z }, obs.radius * 0.75f, Color{ 45, 175, 45, 255 });
            }
        }

        // رسم الساحر (مجسم 3D)
        DrawCylinder((Vector3){ playerPos.x, 0.0f, playerPos.z }, 0.5f, 0.3f, 1.4f, 8, Color{ 55, 60, 110, 255 });
        DrawSphere((Vector3){ playerPos.x, 1.5f, playerPos.z }, 0.4f, Color{ 40, 45, 80, 255 }); // رأس الساحر
        DrawCylinder((Vector3){ playerPos.x + 0.55f, 0.3f, playerPos.z - 0.2f }, 0.06f, 0.06f, 1.8f, 6, DARKBROWN); // العصا
        DrawSphere((Vector3){ playerPos.x + 0.55f, 1.25f, playerPos.z - 0.2f }, 0.16f, GOLD); // جوهرة العصا

        // رسم مقذوفات السحر ثلاثية الأبعاد
        for (const auto &sp : spells) {
            if (!sp.active) continue;
            DrawSphere(sp.pos, 0.4f, sp.color);
        }

        EndMode3D();

        // 4. رسم واجهة التحكم ثنائية الأبعاد (HUD & Touch Controls)
        DrawCircleV(stickCenter, stickBaseRadius, ColorAlpha(DARKGRAY, 0.4f));
        DrawCircleLines((int)stickCenter.x, (int)stickCenter.y, stickBaseRadius, LIGHTGRAY);
        DrawCircleV(knobPos, stickKnobRadius, ColorAlpha(RAYWHITE, 0.7f));

        DrawRectangleRec(attackBtn, Color{ 225, 50, 50, 200 });
        DrawRectangleLinesEx(attackBtn, 3.0f, GOLD);
        DrawText("CAST", (int)(attackBtn.x + attackBtn.width * 0.22f), (int)(attackBtn.y + attackBtn.height * 0.35f), 24, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#if defined(__cplusplus)
}
#endif
