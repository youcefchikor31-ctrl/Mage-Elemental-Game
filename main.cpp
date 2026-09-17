#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>

// --- هياكل البيانات الأساسية ---
struct Particle {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float size;
    float alpha;
    float life;
    float maxLife;
};

struct Bullet {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
    bool active;
};

struct Enemy {
    Vector2 pos;
    Vector2 vel;
    int type; // 0: ملاحق مباشر, 1: حركة جيبية متعرجة, 2: دبابة بطيئة
    float hp;
    float size;
    float angle;
    Color color;
    float timer;
    bool active;
};

#if defined(__cplusplus)
extern "C" {
#endif

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // تهيئة الشاشة بأعلى دقة ومانع تعرج
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(0, 0, "Neon Protocol: Twin Stick");
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    SetTargetFPS(60);

    // إعدادات اللاعب
    Vector2 playerPos = { screenW / 2.0f, screenH / 2.0f };
    float playerSpeed = 350.0f;
    float fireCooldown = 0.0f;
    int score = 0;
    bool gameOver = false;

    // القوائم
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<Particle> particles;

    float spawnTimer = 0.0f;
    float spawnRate = 1.0f;
    float screenShake = 0.0f;

    // ألوان النيون المشعة
    Color neonCyan = { 0, 255, 255, 255 };
    Color neonMagenta = { 255, 0, 255, 255 };
    Color neonYellow = { 255, 255, 0, 255 };
    Color neonRed = { 255, 50, 50, 255 };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f; // حماية من التقطيع

        if (!gameOver) {
            // --- 1. نظام التحكم المزدوج (Twin-Stick Touch) ---
            Vector2 moveDir = { 0.0f, 0.0f };
            Vector2 shootDir = { 0.0f, 0.0f };
            bool isShooting = false;

            int touchCount = GetTouchPointCount();
            for (int i = 0; i < touchCount || (touchCount == 0 && IsMouseButtonDown(MOUSE_BUTTON_LEFT)); i++) {
                Vector2 touchPos = (touchCount > 0) ? GetTouchPosition(i) : GetMousePosition();

                // النصف الأيسر للحركة
                if (touchPos.x < screenW / 2.0f) {
                    Vector2 stickBase = { screenW * 0.15f, screenH * 0.75f };
                    Vector2 diff = Vector2Subtract(touchPos, stickBase);
                    if (Vector2Length(diff) > 20.0f) {
                        moveDir = Vector2Normalize(diff);
                    }
                }
                // النصف الأيمن للتصويب وإطلاق النار
                else {
                    Vector2 stickBase = { screenW * 0.85f, screenH * 0.75f };
                    Vector2 diff = Vector2Subtract(touchPos, stickBase);
                    if (Vector2Length(diff) > 10.0f) {
                        shootDir = Vector2Normalize(diff);
                        isShooting = true;
                    }
                }
            }

            // لوحة المفاتيح للتجربة على الكمبيوتر
            if (IsKeyDown(KEY_W)) moveDir.y = -1.0f;
            if (IsKeyDown(KEY_S)) moveDir.y = 1.0f;
            if (IsKeyDown(KEY_A)) moveDir.x = -1.0f;
            if (IsKeyDown(KEY_D)) moveDir.x = 1.0f;
            if (Vector2Length(moveDir) > 0) moveDir = Vector2Normalize(moveDir);

            // تحديث موقع اللاعب
            playerPos = Vector2Add(playerPos, Vector2Scale(moveDir, playerSpeed * dt));
            playerPos.x = Clamp(playerPos.x, 20.0f, screenW - 20.0f);
            playerPos.y = Clamp(playerPos.y, 20.0f, screenH - 20.0f);

            // --- 2. نظام إطلاق النار ---
            if (fireCooldown > 0.0f) fireCooldown -= dt;
            if (isShooting && fireCooldown <= 0.0f) {
                Bullet b;
                b.pos = Vector2Add(playerPos, Vector2Scale(shootDir, 25.0f));
                b.vel = Vector2Scale(shootDir, 900.0f); // سرعة طلقات ليزرية
                b.color = neonCyan;
                b.life = 1.5f;
                b.active = true;
                bullets.push_back(b);
                fireCooldown = 0.12f; // معدل إطلاق سريع جداً
            }

            // --- 3. توليد الأعداء بناءً على دوال رياضية ---
            spawnTimer += dt;
            if (spawnTimer >= spawnRate) {
                spawnTimer = 0.0f;
                if (spawnRate > 0.25f) spawnRate -= 0.01f; // زيادة الصعوبة تدريجياً

                Enemy en;
                float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
                float dist = screenW * 0.7f; // ظهور من خارج الشاشة
                en.pos = Vector2Add(playerPos, { cosf(angle) * dist, sinf(angle) * dist });
                en.type = GetRandomValue(0, 2);
                en.timer = 0.0f;
                en.active = true;

                if (en.type == 0) { // مثلث أحمر (ملاحق)
                    en.hp = 1.0f;
                    en.size = 22.0f;
                    en.color = neonRed;
                } else if (en.type == 1) { // مربع قناص متعرج (حركة جيبية)
                    en.hp = 2.0f;
                    en.size = 18.0f;
                    en.color = neonMagenta;
                } else { // مضلع بطيء ضخم
                    en.hp = 5.0f;
                    en.size = 35.0f;
                    en.color = neonYellow;
                }
                enemies.push_back(en);
            }

            // --- 4. تحديث الأعداء وفيزياء التصادم ---
            for (auto &en : enemies) {
                if (!en.active) continue;
                en.timer += dt;

                Vector2 toPlayer = Vector2Subtract(playerPos, en.pos);
                Vector2 dir = Vector2Normalize(toPlayer);

                if (en.type == 0) {
                    en.vel = Vector2Scale(dir, 180.0f);
                    en.angle = atan2f(dir.y, dir.x) * RAD2DEG + 90.0f;
                } 
                else if (en.type == 1) {
                    // حركة تموجية: تطبيق دالة الجيب على المسار العمودي للحركة
                    Vector2 perp = { -dir.y, dir.x };
                    float wave = sinf(en.timer * 6.0f) * 150.0f;
                    en.vel = Vector2Add(Vector2Scale(dir, 140.0f), Vector2Scale(perp, wave));
                    en.angle += 180.0f * dt; // دوران مستمر
                } 
                else {
                    en.vel = Vector2Scale(dir, 80.0f);
                    en.angle += 45.0f * dt;
                }

                en.pos = Vector2Add(en.pos, Vector2Scale(en.vel, dt));

                // فحص الاصطدام باللاعب (خسارة)
                if (Vector2Distance(playerPos, en.pos) < (en.size + 15.0f)) {
                    gameOver = true;
                    screenShake = 0.5f;
                }
            }

            // تحديث الطلقات وتصادمها بالأعداء
            for (auto &b : bullets) {
                if (!b.active) continue;
                b.pos = Vector2Add(b.pos, Vector2Scale(b.vel, dt));
                b.life -= dt;
                if (b.life <= 0.0f || b.pos.x < -50 || b.pos.x > screenW + 50 || b.pos.y < -50 || b.pos.y > screenH + 50) {
                    b.active = false;
                    continue;
                }

                for (auto &en : enemies) {
                    if (!en.active) continue;
                    if (Vector2Distance(b.pos, en.pos) < (en.size + 10.0f)) {
                        b.active = false;
                        en.hp -= 1.0f;
                        
                        // شرارات عند الإصابة
                        for (int k = 0; k < 5; k++) {
                            Particle p;
                            p.pos = b.pos;
                            p.vel = { (float)GetRandomValue(-200, 200), (float)GetRandomValue(-200, 200) };
                            p.color = b.color;
                            p.size = (float)GetRandomValue(4, 8);
                            p.alpha = 1.0f;
                            p.life = 0.0f;
                            p.maxLife = 0.2f;
                            particles.push_back(p);
                        }

                        if (en.hp <= 0.0f) {
                            en.active = false;
                            score += (en.type + 1) * 10;
                            screenShake += 0.05f;

                            // انفجار هندسي كبير عند موت العدو
                            for (int k = 0; k < 25; k++) {
                                Particle p;
                                p.pos = en.pos;
                                float randAngle = (float)GetRandomValue(0, 360) * DEG2RAD;
                                float randSpeed = (float)GetRandomValue(100, 450);
                                p.vel = { cosf(randAngle) * randSpeed, sinf(randAngle) * randSpeed };
                                p.color = en.color;
                                p.size = (float)GetRandomValue(6, 14);
                                p.alpha = 1.0f;
                                p.life = 0.0f;
                                p.maxLife = (float)GetRandomValue(30, 80) / 100.0f;
                                particles.push_back(p);
                            }
                        }
                        break;
                    }
                }
            }
        } else {
            // إعادة التشغيل
            if (GetTouchPointCount() > 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                playerPos = { screenW / 2.0f, screenH / 2.0f };
                bullets.clear();
                enemies.clear();
                particles.clear();
                score = 0;
                spawnRate = 1.0f;
                gameOver = false;
            }
        }

        // تحديث فيزياء الجزيئات (الاحتكاك والقصور الذاتي)
        for (size_t i = 0; i < particles.size();) {
            particles[i].pos = Vector2Add(particles[i].pos, Vector2Scale(particles[i].vel, dt));
            particles[i].vel = Vector2Scale(particles[i].vel, 0.92f); // احتكاك هوائي
            particles[i].life += dt;
            particles[i].alpha = 1.0f - (particles[i].life / particles[i].maxLife);
            particles[i].size *= 0.95f;

            if (particles[i].life >= particles[i].maxLife) particles.erase(particles.begin() + i);
            else i++;
        }

        // --- 5. الرسم وتأثيرات ما بعد المعالجة (Rendering) ---
        Vector2 camOffset = { 0.0f, 0.0f };
        if (screenShake > 0.0f) {
            camOffset.x = (float)GetRandomValue(-10, 10) * screenShake;
            camOffset.y = (float)GetRandomValue(-10, 10) * screenShake;
            screenShake -= dt;
        }

        BeginDrawing();
        ClearBackground({ 10, 10, 15, 255 }); // الفراغ المظلم

        // رسم الشبكة الخلفية المضيئة (Dynamic Grid)
        for (int x = 0; x < screenW; x += 60) {
            DrawLineV({ (float)x + camOffset.x, 0 }, { (float)x + camOffset.x, (float)screenH }, { 25, 30, 45, 255 });
        }
        for (int y = 0; y < screenH; y += 60) {
            DrawLineV({ 0, (float)y + camOffset.y }, { (float)screenW, (float)y + camOffset.y }, { 25, 30, 45, 255 });
        }

        // تأثير الإضاءة الإشعاعية التراكمية
        BeginBlendMode(BLEND_ADDITIVE);

        // رسم الأعداء
        for (const auto &en : enemies) {
            if (!en.active) continue;
            Vector2 drawPos = Vector2Add(en.pos, camOffset);
            
            // التوهج الخارجي
            DrawCircleV(drawPos, en.size * 1.5f, ColorAlpha(en.color, 0.3f));
            
            if (en.type == 0) { // مثلث
                DrawPoly(drawPos, 3, en.size, en.angle, en.color);
                DrawPolyLinesEx(drawPos, 3, en.size, en.angle, 3.0f, WHITE);
            } else if (en.type == 1) { // مربع
                DrawPoly(drawPos, 4, en.size, en.angle, en.color);
                DrawPolyLinesEx(drawPos, 4, en.size, en.angle, 3.0f, WHITE);
            } else { // مضلع ضخم
                DrawPoly(drawPos, 6, en.size, en.angle, en.color);
                DrawPolyLinesEx(drawPos, 6, en.size, en.angle, 4.0f, WHITE);
            }
        }

        // رسم طلقات الليزر
        for (const auto &b : bullets) {
            if (!b.active) continue;
            Vector2 drawPos = Vector2Add(b.pos, camOffset);
            DrawCircleV(drawPos, 12.0f, ColorAlpha(b.color, 0.6f));
            DrawCircleV(drawPos, 6.0f, WHITE);
        }

        // رسم الجزيئات (الانفجارات)
        for (const auto &p : particles) {
            Vector2 drawPos = Vector2Add(p.pos, camOffset);
            DrawCircleV(drawPos, p.size, ColorAlpha(p.color, p.alpha));
        }

        // رسم الساحر/المركبة الهندسية
        Vector2 pDrawPos = Vector2Add(playerPos, camOffset);
        DrawCircleV(pDrawPos, 35.0f, ColorAlpha(neonCyan, 0.2f)); // هالة الضوء
        DrawPoly(pDrawPos, 8, 20.0f, GetTime() * 100.0f, neonCyan);
        DrawCircleV(pDrawPos, 10.0f, WHITE);

        EndBlendMode();

        // واجهة التحكم الشفافة
        Vector2 leftStickBase = { screenW * 0.15f, screenH * 0.75f };
        Vector2 rightStickBase = { screenW * 0.85f, screenH * 0.75f };
        
        DrawCircleLines((int)leftStickBase.x, (int)leftStickBase.y, screenH * 0.12f, ColorAlpha(WHITE, 0.2f));
        DrawText("MOVE", (int)leftStickBase.x - 30, (int)leftStickBase.y - 10, 20, ColorAlpha(WHITE, 0.3f));

        DrawCircleLines((int)rightStickBase.x, (int)rightStickBase.y, screenH * 0.12f, ColorAlpha(WHITE, 0.2f));
        DrawText("AIM & SHOOT", (int)rightStickBase.x - 60, (int)rightStickBase.y - 10, 20, ColorAlpha(WHITE, 0.3f));

        // النتيجة
        DrawText(TextFormat("SCORE: %06d", score), 20, 20, 30, WHITE);

        if (gameOver) {
            DrawRectangle(0, 0, screenW, screenH, ColorAlpha(BLACK, 0.85f));
            DrawText("SYSTEM FAILURE", screenW / 2 - 180, screenH / 2 - 60, 40, neonRed);
            DrawText(TextFormat("FINAL SCORE: %d", score), screenW / 2 - 140, screenH / 2 + 10, 30, WHITE);
            DrawText("TAP TO REBOOT", screenW / 2 - 120, screenH / 2 + 70, 24, neonCyan);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#if defined(__cplusplus)
}
#endif
