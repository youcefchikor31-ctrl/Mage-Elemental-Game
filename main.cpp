#include "raylib.h"
#include "raymath.h"
#include <deque>
#include <cmath>

#if defined(__cplusplus)
extern "C" {
#endif

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // تهيئة الشاشة لأبعاد الهاتف تلقائياً
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(0, 0, "Mobile Neon Snake");
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    SetTargetFPS(60);

    // إعدادات الشبكة (Grid)
    int gridSize = 20;
    // حساب حجم المربع ليتناسب مع عرض الشاشة
    float cellSize = (screenW < screenH ? screenW : screenH) * 0.9f / gridSize; 
    // توسيط الشبكة في النصف العلوي من الشاشة
    Vector2 gridOffset = { (screenW - cellSize * gridSize) / 2.0f, (screenH - cellSize * gridSize) / 4.0f };

    // إعدادات الثعبان (نستخدم deque لأنها الأفضل رياضياً للثعبان: نضيف من الأمام ونحذف من الخلف)
    std::deque<Vector2> snake;
    snake.push_back({10, 10});
    snake.push_back({10, 11});
    snake.push_back({10, 12});
    
    Vector2 dir = {0, -1}; // يبدأ بالحركة للأعلى
    Vector2 nextDir = dir;

    // إعدادات التفاحة
    Vector2 food = { (float)GetRandomValue(0, gridSize - 1), (float)GetRandomValue(0, gridSize - 1) };

    // متغيرات اللعب
    float moveTimer = 0.0f;
    float moveSpeed = 0.18f; // سرعة الثعبان (تتحدث كل 0.18 ثانية)
    int score = 0;
    bool gameOver = false;

    // إعدادات ذراع التحكم (D-Pad) للهواتف
    float dpadRadius = screenW * 0.22f;
    Vector2 dpadCenter = { screenW / 2.0f, screenH * 0.82f };

    // ألوان النيون
    Color neonGreen = { 57, 255, 20, 255 };
    Color neonRed = { 255, 30, 30, 255 };
    Color gridColor = { 30, 35, 50, 255 };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- 1. معالجة الإدخال (Touch & D-Pad) ---
        if (GetTouchPointCount() > 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 touchPos = (GetTouchPointCount() > 0) ? GetTouchPosition(0) : GetMousePosition();

            if (gameOver) {
                // إعادة تشغيل اللعبة
                snake.clear();
                snake.push_back({10, 10});
                snake.push_back({10, 11});
                snake.push_back({10, 12});
                dir = {0, -1};
                nextDir = dir;
                score = 0;
                moveSpeed = 0.18f;
                gameOver = false;
            } else {
                // حساب اللمس على ذراع التحكم الدائري (تحديد الاتجاه بالرياضيات)
                Vector2 diff = Vector2Subtract(touchPos, dpadCenter);
                if (Vector2Length(diff) < dpadRadius * 1.5f) { // مساحة اللمس أوسع قليلاً لسهولة التحكم
                    if (diff.y < -fabs(diff.x) && dir.y == 0) nextDir = {0, -1};      // أعلى
                    else if (diff.y > fabs(diff.x) && dir.y == 0) nextDir = {0, 1};   // أسفل
                    else if (diff.x < -fabs(diff.y) && dir.x == 0) nextDir = {-1, 0}; // يسار
                    else if (diff.x > fabs(diff.y) && dir.x == 0) nextDir = {1, 0};   // يمين
                }
            }
        }

        // --- 2. منطق وتحديث اللعبة (Game Loop) ---
        if (!gameOver) {
            moveTimer += dt;
            if (moveTimer >= moveSpeed) {
                moveTimer = 0.0f;
                dir = nextDir;
                Vector2 head = snake.front();
                Vector2 newHead = Vector2Add(head, dir);

                // فحص الاصطدام بالجدران
                if (newHead.x < 0 || newHead.x >= gridSize || newHead.y < 0 || newHead.y >= gridSize) {
                    gameOver = true;
                }

                // فحص الاصطدام بالجسد
                for (const auto& segment : snake) {
                    if (newHead.x == segment.x && newHead.y == segment.y) {
                        gameOver = true;
                    }
                }

                if (!gameOver) {
                    snake.push_front(newHead); // تحريك الرأس خطوة للأمام

                    // فحص أكل التفاحة
                    if (newHead.x == food.x && newHead.y == food.y) {
                        score += 10;
                        moveSpeed = fmax(0.06f, moveSpeed - 0.005f); // زيادة السرعة تدريجياً
                        
                        // توليد تفاحة جديدة في مكان فارغ
                        bool validPos = false;
                        while (!validPos) {
                            validPos = true;
                            food = { (float)GetRandomValue(0, gridSize - 1), (float)GetRandomValue(0, gridSize - 1) };
                            for (const auto& segment : snake) {
                                if (food.x == segment.x && food.y == segment.y) {
                                    validPos = false;
                                    break;
                                }
                            }
                        }
                    } else {
                        // إذا لم يأكل، نحذف آخر قطعة من الذيل لكي يبدو أنه يتحرك
                        snake.pop_back();
                    }
                }
            }
        }

        // --- 3. الرسم (Rendering) ---
        BeginDrawing();
        ClearBackground({ 15, 18, 25, 255 }); // خلفية ليلية داكنة

        // رسم حدود الشبكة
        DrawRectangleLinesEx({ gridOffset.x - 2, gridOffset.y - 2, cellSize * gridSize + 4, cellSize * gridSize + 4 }, 4.0f, WHITE);
        
        // رسم الخطوط الداخلية للشبكة
        for (int i = 0; i <= gridSize; i++) {
            DrawLineV({ gridOffset.x + i * cellSize, gridOffset.y }, { gridOffset.x + i * cellSize, gridOffset.y + gridSize * cellSize }, gridColor);
            DrawLineV({ gridOffset.x, gridOffset.y + i * cellSize }, { gridOffset.x + gridSize * cellSize, gridOffset.y + i * cellSize }, gridColor);
        }

        // رسم التفاحة بتأثير نبض مشع
        float pulse = (sinf(GetTime() * 8.0f) + 1.0f) * 0.5f;
        Vector2 foodCenter = { gridOffset.x + food.x * cellSize + cellSize / 2, gridOffset.y + food.y * cellSize + cellSize / 2 };
        DrawCircleV(foodCenter, (cellSize / 2) * 1.4f, ColorAlpha(neonRed, 0.4f * pulse));
        DrawCircleV(foodCenter, (cellSize / 2) * 0.8f, neonRed);

        // رسم الثعبان
        for (size_t i = 0; i < snake.size(); i++) {
            Rectangle segmentRect = { gridOffset.x + snake[i].x * cellSize + 1, gridOffset.y + snake[i].y * cellSize + 1, cellSize - 2, cellSize - 2 };
            
            if (i == 0) {
                // رأس الثعبان أبيض مشع
                DrawRectangleRec(segmentRect, WHITE);
                // رسم عيون للرأس
                DrawCircle(segmentRect.x + cellSize * 0.3f, segmentRect.y + cellSize * 0.3f, cellSize * 0.15f, BLACK);
                DrawCircle(segmentRect.x + cellSize * 0.7f, segmentRect.y + cellSize * 0.3f, cellSize * 0.15f, BLACK);
            } else {
                // باقي الجسد أخضر نيون
                DrawRectangleRec(segmentRect, neonGreen);
            }
        }

        // رسم النتيجة
        DrawText(TextFormat("SCORE: %d", score), gridOffset.x, gridOffset.y - 40, 28, WHITE);

        // رسم ذراع التحكم (D-Pad) في أسفل الشاشة
        DrawCircleV(dpadCenter, dpadRadius, ColorAlpha(DARKGRAY, 0.5f));
        DrawCircleLines((int)dpadCenter.x, (int)dpadCenter.y, dpadRadius, LIGHTGRAY);
        
        // رسم أسهم توجيهية داخل الـ D-Pad
        DrawTriangle({ dpadCenter.x, dpadCenter.y - dpadRadius * 0.8f }, { dpadCenter.x - 20, dpadCenter.y - dpadRadius * 0.3f }, { dpadCenter.x + 20, dpadCenter.y - dpadRadius * 0.3f }, WHITE); // أعلى
        DrawTriangle({ dpadCenter.x, dpadCenter.y + dpadRadius * 0.8f }, { dpadCenter.x + 20, dpadCenter.y + dpadRadius * 0.3f }, { dpadCenter.x - 20, dpadCenter.y + dpadRadius * 0.3f }, WHITE); // أسفل
        DrawTriangle({ dpadCenter.x - dpadRadius * 0.8f, dpadCenter.y }, { dpadCenter.x - dpadRadius * 0.3f, dpadCenter.y - 20 }, { dpadCenter.x - dpadRadius * 0.3f, dpadCenter.y + 20 }, WHITE); // يسار
        DrawTriangle({ dpadCenter.x + dpadRadius * 0.8f, dpadCenter.y }, { dpadCenter.x + dpadRadius * 0.3f, dpadCenter.y + 20 }, { dpadCenter.x + dpadRadius * 0.3f, dpadCenter.y - 20 }, WHITE); // يمين

        // شاشة الخسارة
        if (gameOver) {
            DrawRectangle(0, 0, screenW, screenH, ColorAlpha(BLACK, 0.7f));
            DrawText("GAME OVER!", screenW / 2 - MeasureText("GAME OVER!", 40) / 2, screenH / 2 - 50, 40, RED);
            DrawText("TAP TO RESTART", screenW / 2 - MeasureText("TAP TO RESTART", 24) / 2, screenH / 2 + 20, 24, WHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#if defined(__cplusplus)
}
#endif
