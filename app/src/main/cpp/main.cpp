#include <cstdio>
#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>
#include "raylib.h"

struct Star {
    float x;
    float y;
    float speed;
    float size;
    Color color;
};

Texture2D LoadTextureSafe(const char* relativePath) {
    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), "%s%s", GetApplicationDirectory(), relativePath);
    Texture2D tex = LoadTexture(fullPath);
    return tex;
}

Image LoadImageSafe(const char* relativePath) {
    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), "%s%s", GetApplicationDirectory(), relativePath);
    Image img = LoadImage(fullPath);
    return img;
}

Color ScaleColor(Color base, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    Color result;
    result.r = (unsigned char)(base.r * t);
    result.g = (unsigned char)(base.g * t);
    result.b = (unsigned char)(base.b * t);
    result.a = base.a;
    return result;
}

Image RecolorPlayerBody(Image source, Color targetColor) {
    Image result = ImageCopy(source);
    ImageFormat(&result, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Color* pixels = (Color*)result.data;
    int total = result.width * result.height;

    Color shadowColor = ScaleColor(targetColor, 0.7f);

    for (int i = 0; i < total; i++) {
        Color px = pixels[i];
        if (px.a == 0) continue;

        Vector3 hsv = ColorToHSV(px);
        float hue = hsv.x, sat = hsv.y, val = hsv.z;

        if (sat < 0.15f || val < 0.18f) continue;

        Color newColor;
        bool recognized = true;

        if (hue <= 20.0f || hue >= 340.0f) {
            newColor = ScaleColor(targetColor, val);
        } else if (hue >= 200.0f && hue <= 260.0f) {
            newColor = ScaleColor(shadowColor, val);
        } else if (hue >= 100.0f && hue <= 140.0f) {
            newColor = ScaleColor(WHITE, val);
        } else {
            recognized = false;
        }

        if (recognized) {
            newColor.a = px.a;
            pixels[i] = newColor;
        }
    }

    return result;
}

void ApplyPlayerColor(Image sourceImage, Color targetColor, Texture2D* outTexture) {
    if (sourceImage.data == nullptr) return;
    if (outTexture->id > 0) UnloadTexture(*outTexture);
    Image recolored = RecolorPlayerBody(sourceImage, targetColor);
    *outTexture = LoadTextureFromImage(recolored);
    UnloadImage(recolored);
}

void ApplyPlayerColorAll(Image* sourceImages, Texture2D* outTextures, int count, Color targetColor) {
    for (int i = 0; i < count; i++) {
        ApplyPlayerColor(sourceImages[i], targetColor, &outTextures[i]);
    }
}

struct PlayerColorOption {
    const char* name;
    Color color;
};

static PlayerColorOption colorPalette[] = {
    { "Rouge",  Color{ 255,  16,  16, 255 } },
    { "Bleu",   Color{  19,  46, 209, 255 } },
    { "Vert",   Color{  17, 127,  45, 255 } },
    { "Rose",   Color{ 237,  84, 186, 255 } },
    { "Orange", Color{ 239, 125,  13, 255 } },
    { "Jaune",  Color{ 246, 246,  87, 255 } },
    { "Noir",   Color{  63,  71,  78, 255 } },
    { "Blanc",  Color{ 214, 224, 240, 255 } },
    { "Violet", Color{ 107,  47, 187, 255 } },
    { "Marron", Color{ 113,  73,  30, 255 } },
    { "Cyan",   Color{  56, 254, 219, 255 } },
    { "Lime",   Color{  80, 239,  57, 255 } },
};
static const int colorPaletteCount = sizeof(colorPalette) / sizeof(colorPalette[0]);

int main() {
    ChangeDirectory(GetApplicationDirectory());

    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Among Us 2");
    ShowCursor();

    int monitor = GetCurrentMonitor();
    int screenWidth = GetMonitorWidth(monitor);
    int screenHeight = GetMonitorHeight(monitor);
    
    SetWindowSize(screenWidth, screenHeight);
    SetWindowPosition(0, 0);
    SetTargetFPS(60);

    const int STAR_COUNT = 120;
    Star stars[120];
    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].x = (float)(rand() % screenWidth);
        stars[i].y = (float)(rand() % screenHeight);
        stars[i].speed = (float)(rand() % 40 + 10);
        stars[i].size = (float)(rand() % 3 + 1);
        unsigned char alpha = (unsigned char)(rand() % 155 + 100);
        stars[i].color = Color{ 255, 255, 255, alpha };
    }

    int totalFrames = 0;
    while (true) {
        char relativePath[256];
        char fullPath[512];
        snprintf(relativePath, sizeof(relativePath), "Assets/frames/frame_%04d.png", totalFrames + 1);
        snprintf(fullPath, sizeof(fullPath), "%s%s", GetApplicationDirectory(), relativePath);
        if (!FileExists(fullPath)) break;
        totalFrames++;
    }

    float currentFrameTime = 0.0f;
    int currentFrameIndex = 1;
    float frameDuration = 1.0f / 30.0f;

    Texture2D bgTexture = { 0 };
    if (totalFrames > 0) {
        char firstFramePath[512];
        snprintf(firstFramePath, sizeof(firstFramePath), "%sAssets/frames/frame_0001.png", GetApplicationDirectory());
        Image img = LoadImage(firstFramePath);
        if (img.data != nullptr) {
            bgTexture = LoadTextureFromImage(img);
            UnloadImage(img);
        }
    }

    Texture2D btnOnline       = LoadTextureSafe("Assets/Menu/Online.png");
    Texture2D btnOnlineGreen  = LoadTextureSafe("Assets/Menu/OnlineGreen.png");
    Texture2D btnSettings     = LoadTextureSafe("Assets/Menu/Settings.png");
    Texture2D texLogo         = LoadTextureSafe("Assets/Menu/Logo.png");
    Texture2D texSettingsMenu = LoadTextureSafe("Assets/Menu/SettingsMenu.png");
    Texture2D texFb           = LoadTextureSafe("Assets/Menu/Fb.png");

    Texture2D btnHost         = LoadTextureSafe("Assets/Menu/Host.png");
    Texture2D btnHostGreen    = LoadTextureSafe("Assets/Menu/HostGreen.png");
    Texture2D btnJoin         = LoadTextureSafe("Assets/Menu/Join.png");
    Texture2D btnJoinGreen    = LoadTextureSafe("Assets/Menu/Joingreen.png");
    Texture2D btnGoBack       = LoadTextureSafe("Assets/Menu/Goback.png");
    Texture2D texCode         = LoadTextureSafe("Assets/Menu/Code.png");
    Texture2D texCodeGreen    = LoadTextureSafe("Assets/Menu/Codegreen.png");
    Texture2D texDoesntExist  = LoadTextureSafe("Assets/Menu/dosentexist.png");

    Texture2D texSkeldHost    = LoadTextureSafe("Assets/Menu/SkeldHost.png");

    Texture2D texPlayersLabel = LoadTextureSafe("Assets/Menu/Players.png");
    Texture2D texPlayerNum[7]; 
    for (int i = 0; i < 7; i++) {
        char path[128];
        snprintf(path, sizeof(path), "Assets/Menu/%d.png", i + 4);
        texPlayerNum[i] = LoadTextureSafe(path);
    }

    Texture2D texImpostersLabel = LoadTextureSafe("Assets/Menu/Imposters.png");
    Texture2D texImposterNum[3];
    for (int i = 0; i < 3; i++) {
        char path[128];
        snprintf(path, sizeof(path), "Assets/Menu/%d.png", i + 1);
        texImposterNum[i] = LoadTextureSafe(path);
    }

    Texture2D texLobby = LoadTextureSafe("Assets/Game/Lobby.png");

    const int PLAYER_FRAME_COUNT = 13;
    Image playerFrameImagesCPU[PLAYER_FRAME_COUNT];
    Texture2D playerFrameTextures[PLAYER_FRAME_COUNT];
    for (int i = 0; i < PLAYER_FRAME_COUNT; i++) playerFrameTextures[i] = { 0 };

    playerFrameImagesCPU[0] = LoadImageSafe("Assets/Player/Idle.png");
    for (int i = 1; i <= 12; i++) {
        char path[64];
        snprintf(path, sizeof(path), "Assets/Player/walk%d.png", i);
        playerFrameImagesCPU[i] = LoadImageSafe(path);
    }
    for (int i = 0; i < PLAYER_FRAME_COUNT; i++) {
        if (playerFrameImagesCPU[i].data != nullptr) {
            ImageFormat(&playerFrameImagesCPU[i], PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        }
    }

    Color selectedPlayerColor = colorPalette[0].color;
    ApplyPlayerColorAll(playerFrameImagesCPU, playerFrameTextures, PLAYER_FRAME_COUNT, selectedPlayerColor);

    Vector2 playerPos = { 0.0f, 0.0f };
    float playerSpeed = 260.0f;
    float playerScale = 0.55f;
    bool playerFacingLeft = false;
    bool isColorMenuOpen = false;

    bool isPlayerMoving = false;
    int walkFrameIndex = 0;
    float walkAnimTimer = 0.0f;
    float walkFrameDuration = 1.0f / 15.0f;

    int gameState = 0; 
    bool isSettingsOpen = false;
    float menuScaleAnim = 0.0f;
    float animSpeed = 8.0f;

    char roomCode[7] = "\0";
    int letterCount = 0;
    bool isCodeBoxActive = false;
    bool showNotExistError = false;

    int selectedMaxPlayers = 10;
    int selectedImposters  = 1;

    while (!WindowShouldClose()) {
        int currentW = GetScreenWidth();
        int currentH = GetScreenHeight();
        float deltaTime = GetFrameTime();
        
        Vector2 mousePos = GetMousePosition();
        if (GetTouchPointCount() > 0) {
            mousePos = GetTouchPosition(0);
        }

        bool actionPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || (GetTouchPointCount() > 0 && IsGestureDetected(GESTURE_TAP));
        bool actionDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || (GetTouchPointCount() > 0);

        for (int i = 0; i < STAR_COUNT; i++) {
            stars[i].x -= stars[i].speed * deltaTime;
            if (stars[i].x < 0) {
                stars[i].x = (float)currentW;
                stars[i].y = (float)(rand() % currentH);
            }
        }

        Rectangle lobbyImageRect = { 0.0f, 0.0f, (float)currentW, (float)currentH };
        if (texLobby.id > 0) {
            float texAspect = (float)texLobby.width / (float)texLobby.height;
            float screenAspect = (float)currentW / (float)currentH;
            float drawW, drawH;
            if (screenAspect > texAspect) {
                drawH = (float)currentH;
                drawW = drawH * texAspect;
            } else {
                drawW = (float)currentW;
                drawH = drawW / texAspect;
            }
            lobbyImageRect = { ((float)currentW - drawW) / 2.0f, ((float)currentH - drawH) / 2.0f, drawW, drawH };
        }

        if (IsCursorHidden()) ShowCursor();

        if (IsKeyPressed(KEY_F11)) {
            if (IsWindowState(FLAG_WINDOW_UNDECORATED)) {
                ClearWindowState(FLAG_WINDOW_UNDECORATED);
                SetWindowSize(1280, 720);
                SetWindowPosition(100, 100);
            } else {
                SetWindowState(FLAG_WINDOW_UNDECORATED);
                SetWindowSize(screenWidth, screenHeight);
                SetWindowPosition(0, 0);
            }
        }

        if (totalFrames > 0) {
            currentFrameTime += deltaTime;
            if (currentFrameTime >= frameDuration) {
                currentFrameTime -= frameDuration;
                currentFrameIndex++;
                if (currentFrameIndex > totalFrames) currentFrameIndex = 1;

                char framePath[512];
                snprintf(framePath, sizeof(framePath), "%sAssets/frames/frame_%04d.png", GetApplicationDirectory(), currentFrameIndex);
                
                Image img = LoadImage(framePath);
                if (img.data != nullptr) {
                    UnloadTexture(bgTexture);
                    bgTexture = LoadTextureFromImage(img);
                    UnloadImage(img);
                }
            }
        }

        if (gameState == 0) {
            float onlineScale = 0.8f;
            float onlineW = (btnOnline.width > 0) ? btnOnline.width * onlineScale : 250.0f;
            float onlineH = (btnOnline.height > 0) ? btnOnline.height * onlineScale : 100.0f;
            Rectangle onlineRect = { 30.0f, (float)currentH * 0.35f, onlineW, onlineH };

            float settingsScale = 0.8f;
            float settingsW = (btnSettings.width > 0) ? btnSettings.width * settingsScale : 80.0f;
            float settingsH = (btnSettings.height > 0) ? btnSettings.height * settingsScale : 80.0f;
            Rectangle settingsRect = { 30.0f, onlineRect.y + onlineRect.height + 20.0f, settingsW, settingsH };

            if (CheckCollisionPointRec(mousePos, onlineRect) && actionPressed) gameState = 1;
            if (CheckCollisionPointRec(mousePos, settingsRect) && actionPressed) isSettingsOpen = !isSettingsOpen;
        }

        if (gameState == 1) {
            float hostScale = 0.8f;
            float hostW = (btnHost.width > 0) ? btnHost.width * hostScale : 200.0f;
            float hostH = (btnHost.height > 0) ? btnHost.height * hostScale : 80.0f;
            Rectangle hostRect = { (float)currentW / 2.0f - hostW / 2.0f, (float)currentH * 0.25f, hostW, hostH };

            float joinScale = 0.8f;
            float joinW = (btnJoin.width > 0) ? btnJoin.width * joinScale : 200.0f;
            float joinH = (btnJoin.height > 0) ? btnJoin.height * joinScale : 80.0f;
            Rectangle joinRect = { (float)currentW / 2.0f - joinW / 2.0f, hostRect.y + hostRect.height + 15.0f, joinW, joinH };

            float codeScale = 0.8f;
            float codeW = (texCode.width > 0) ? texCode.width * codeScale : 250.0f;
            float codeH = (texCode.height > 0) ? texCode.height * codeScale : 80.0f;
            Rectangle codeRect = { (float)currentW / 2.0f - codeW / 2.0f, joinRect.y + joinRect.height + 15.0f, codeW, codeH };

            if (!showNotExistError) {
                if (CheckCollisionPointRec(mousePos, hostRect) && actionPressed) gameState = 2;
                if (actionPressed) isCodeBoxActive = CheckCollisionPointRec(mousePos, codeRect);

                if (isCodeBoxActive) {
                    int key = GetCharPressed();
                    while (key > 0) {
                        if ((key >= 32) && (key <= 125) && (letterCount < 6)) {
                            roomCode[letterCount] = (char)toupper(key);
                            roomCode[letterCount + 1] = '\0';
                            letterCount++;
                        }
                        key = GetCharPressed();
                    }
                    if (IsKeyPressed(KEY_BACKSPACE)) {
                        letterCount--;
                        if (letterCount < 0) letterCount = 0;
                        roomCode[letterCount] = '\0';
                    }
                    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                        showNotExistError = true;
                        isCodeBoxActive = false;
                    }
                }

                float goBackW = (btnGoBack.width > 0) ? btnGoBack.width * 0.8f : 80.0f;
                float goBackH = (btnGoBack.height > 0) ? btnGoBack.height * 0.8f : 80.0f;
                Rectangle goBackRect = { 15.0f, (float)currentH - goBackH - 15.0f, goBackW, goBackH };
                if (CheckCollisionPointRec(mousePos, goBackRect) && actionPressed) gameState = 0;
            } else {
                float errGoBackW = (btnGoBack.width > 0) ? btnGoBack.width * 0.6f : 60.0f;
                float errGoBackH = (btnGoBack.height > 0) ? btnGoBack.height * 0.6f : 60.0f;
                Rectangle errorGoBackRect = { (float)currentW / 2.0f - 250.0f + 15.0f, (float)currentH / 2.0f - 150.0f + 15.0f, errGoBackW, errGoBackH };
                if (CheckCollisionPointRec(mousePos, errorGoBackRect) && actionPressed) showNotExistError = false;
            }
        }

        if (gameState == 2) {
            float goBackScale = 0.8f;
            float goBackW = (btnGoBack.width > 0) ? btnGoBack.width * goBackScale : 80.0f;
            float goBackH = (btnGoBack.height > 0) ? btnGoBack.height * goBackScale : 80.0f;
            Rectangle hostGoBackRect = { 20.0f, (float)currentH - goBackH - 20.0f, goBackW, goBackH };

            if (CheckCollisionPointRec(mousePos, hostGoBackRect) && actionPressed) gameState = 1;

            float numSize = 45.0f;
            float startX = 600.0f;
            float playersY = 180.0f;
            float impostersY = 340.0f;
            float spacing = 10.0f;

            for (int i = 0; i < 7; i++) {
                Rectangle numRect = { startX + i * (numSize + spacing), playersY, numSize, numSize };
                if (CheckCollisionPointRec(mousePos, numRect) && actionPressed) selectedMaxPlayers = i + 4;
            }

            for (int i = 0; i < 3; i++) {
                Rectangle numRect = { startX + i * (numSize + spacing), impostersY, numSize, numSize };
                if (CheckCollisionPointRec(mousePos, numRect) && actionPressed) selectedImposters = i + 1;
            }

            float confirmHostScale = 0.8f;
            float confirmHostW = (btnHost.width > 0) ? btnHost.width * confirmHostScale : 200.0f;
            float confirmHostH = (btnHost.height > 0) ? btnHost.height * confirmHostScale : 80.0f;
            Rectangle confirmHostRect = { (float)currentW - confirmHostW - 20.0f, (float)currentH - confirmHostH - 20.0f, confirmHostW, confirmHostH };

            if (CheckCollisionPointRec(mousePos, confirmHostRect) && actionPressed) {
                gameState = 3;
                isColorMenuOpen = false;
                isPlayerMoving = false;
                walkFrameIndex = 0;
                walkAnimTimer = 0.0f;
                float playerDrawW = (playerFrameTextures[0].width > 0) ? playerFrameTextures[0].width * playerScale : 0.0f;
                float playerDrawH = (playerFrameTextures[0].height > 0) ? playerFrameTextures[0].height * playerScale : 0.0f;
                playerPos = {
                    lobbyImageRect.x + lobbyImageRect.width / 2.0f - playerDrawW / 2.0f,
                    lobbyImageRect.y + lobbyImageRect.height / 2.0f - playerDrawH / 2.0f
                };
            }
        }

        if (gameState == 3) {
            if (IsKeyPressed(KEY_R)) isColorMenuOpen = !isColorMenuOpen;

            if (isColorMenuOpen) {
                if (IsKeyPressed(KEY_ESCAPE)) isColorMenuOpen = false;

                float swatchSize = 80.0f, spacing = 15.0f;
                int cols = 6, rows = 2;
                float gridW = cols * swatchSize + (cols - 1) * spacing;
                float gridH = rows * swatchSize + (rows - 1) * spacing;
                float gridX = (float)currentW / 2.0f - gridW / 2.0f;
                float gridY = (float)currentH / 2.0f - gridH / 2.0f;

                for (int i = 0; i < colorPaletteCount; i++) {
                    int col = i % cols, row = i / cols;
                    Rectangle swatchRect = { gridX + col * (swatchSize + spacing), gridY + row * (swatchSize + spacing), swatchSize, swatchSize };
                    if (CheckCollisionPointRec(mousePos, swatchRect) && actionPressed) {
                        selectedPlayerColor = colorPalette[i].color;
                        ApplyPlayerColorAll(playerFrameImagesCPU, playerFrameTextures, PLAYER_FRAME_COUNT, selectedPlayerColor);
                        isColorMenuOpen = false;
                    }
                }
            } else {
                float moveX = 0.0f, moveY = 0.0f;
                if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) moveX += 1.0f;
                if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) moveX -= 1.0f;
                if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) moveY += 1.0f;
                if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) moveY -= 1.0f;

                if (actionDown) {
                    float playerCenterW = (playerFrameTextures[0].width > 0) ? playerFrameTextures[0].width * playerScale * 0.5f : 0.0f;
                    float playerCenterH = (playerFrameTextures[0].height > 0) ? playerFrameTextures[0].height * playerScale * 0.5f : 0.0f;
                    Vector2 pCenter = { playerPos.x + playerCenterW, playerPos.y + playerCenterH };
                    float dx = mousePos.x - pCenter.x;
                    float dy = mousePos.y - pCenter.y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist > 15.0f) {
                        moveX = dx / dist;
                        moveY = dy / dist;
                    }
                }

                isPlayerMoving = (moveX != 0.0f || moveY != 0.0f);

                if (isPlayerMoving) {
                    if (!actionDown) {
                        float len = sqrtf(moveX * moveX + moveY * moveY);
                        moveX /= len;
                        moveY /= len;
                    }

                    if (moveX > 0.0f) playerFacingLeft = false;
                    if (moveX < 0.0f) playerFacingLeft = true;

                    playerPos.x += moveX * playerSpeed * deltaTime;
                    playerPos.y += moveY * playerSpeed * deltaTime;

                    walkAnimTimer += deltaTime;
                    if (walkAnimTimer >= walkFrameDuration) {
                        walkAnimTimer -= walkFrameDuration;
                        walkFrameIndex = (walkFrameIndex + 1) % 12;
                    }
                } else {
                    walkFrameIndex = 0;
                    walkAnimTimer = 0.0f;
                }

                float playerDrawW = (playerFrameTextures[0].width > 0) ? playerFrameTextures[0].width * playerScale : 0.0f;
                float playerDrawH = (playerFrameTextures[0].height > 0) ? playerFrameTextures[0].height * playerScale : 0.0f;

                if (playerPos.x < lobbyImageRect.x) playerPos.x = lobbyImageRect.x;
                if (playerPos.y < lobbyImageRect.y) playerPos.y = lobbyImageRect.y;
                if (playerPos.x > lobbyImageRect.x + lobbyImageRect.width - playerDrawW) playerPos.x = lobbyImageRect.x + lobbyImageRect.width - playerDrawW;
                if (playerPos.y > lobbyImageRect.y + lobbyImageRect.height - playerDrawH) playerPos.y = lobbyImageRect.y + lobbyImageRect.height - playerDrawH;
            }
        }

        if (isSettingsOpen && gameState == 0) {
            menuScaleAnim += animSpeed * deltaTime;
            if (menuScaleAnim > 1.0f) menuScaleAnim = 1.0f;
        } else {
            menuScaleAnim -= animSpeed * deltaTime;
            if (menuScaleAnim < 0.0f) menuScaleAnim = 0.0f;
        }

        BeginDrawing();
            ClearBackground(BLACK);

            if (gameState != 3) {
                if (totalFrames > 0 && bgTexture.id > 0) {
                    DrawTexturePro(
                        bgTexture, 
                        Rectangle{ 0, 0, (float)bgTexture.width, (float)bgTexture.height },
                        Rectangle{ 0, 0, (float)currentW, (float)currentH },
                        Vector2{ 0, 0 }, 0.0f, WHITE
                    );
                } else {
                    for (int i = 0; i < STAR_COUNT; i++) {
                        DrawCircleV(Vector2{ stars[i].x, stars[i].y }, stars[i].size, stars[i].color);
                    }
                }
            }

            if (gameState == 0) {
                float onlineScale = 0.8f;
                float onlineW = (btnOnline.width > 0) ? btnOnline.width * onlineScale : 250.0f;
                float onlineH = (btnOnline.height > 0) ? btnOnline.height * onlineScale : 100.0f;
                Rectangle onlineRect = { 30.0f, (float)currentH * 0.35f, onlineW, onlineH };
                
                Texture2D onlineToDraw = (CheckCollisionPointRec(mousePos, onlineRect) && btnOnlineGreen.id > 0) ? btnOnlineGreen : btnOnline;
                if (onlineToDraw.id > 0) DrawTexturePro(onlineToDraw, Rectangle{ 0, 0, (float)onlineToDraw.width, (float)onlineToDraw.height }, onlineRect, Vector2{ 0, 0 }, 0.0f, WHITE);
                else {
                    DrawRectangleRec(onlineRect, DARKGRAY);
                    DrawText("ONLINE", (int)(onlineRect.x + 20), (int)(onlineRect.y + 30), 30, WHITE);
                }

                float settingsScale = 0.8f;
                float settingsW = (btnSettings.width > 0) ? btnSettings.width * settingsScale : 80.0f;
                float settingsH = (btnSettings.height > 0) ? btnSettings.height * settingsScale : 80.0f;
                Rectangle settingsRect = { 30.0f, onlineRect.y + onlineRect.height + 20.0f, settingsW, settingsH };
                if (btnSettings.id > 0) DrawTexturePro(btnSettings, Rectangle{ 0, 0, (float)btnSettings.width, (float)btnSettings.height }, settingsRect, Vector2{ 0, 0 }, 0.0f, CheckCollisionPointRec(mousePos, settingsRect) ? LIGHTGRAY : WHITE);
                else {
                    DrawRectangleRec(settingsRect, GRAY);
                    DrawText("*", (int)(settingsRect.x + 30), (int)(settingsRect.y + 20), 40, WHITE);
                }

                if (texLogo.id > 0) DrawTexturePro(texLogo, Rectangle{ 0, 0, (float)texLogo.width, (float)texLogo.height }, Rectangle{ (float)currentW / 2.0f - (texLogo.width * 0.8f) / 2.0f, (float)currentH * 0.03f, texLogo.width * 0.8f, texLogo.height * 0.8f }, Vector2{ 0, 0 }, 0.0f, WHITE);

                if (menuScaleAnim > 0.001f && texSettingsMenu.id > 0) {
                    float menuW = (float)texSettingsMenu.width;
                    float menuH = (float)texSettingsMenu.height;
                    Rectangle settingsMenuRect = { (float)currentW / 2.0f - ((menuW * menuScaleAnim) / 2.0f), (float)currentH / 2.0f - ((menuH * menuScaleAnim) / 2.0f), menuW * menuScaleAnim, menuH * menuScaleAnim };
                    Color menuTint = WHITE;
                    menuTint.a = (unsigned char)(255 * menuScaleAnim);
                    DrawTexturePro(texSettingsMenu, Rectangle{ 0, 0, menuW, menuH }, settingsMenuRect, Vector2{ 0, 0 }, 0.0f, menuTint);
                }
            }

            if (gameState == 1) {
                if (texLogo.id > 0) DrawTexturePro(texLogo, Rectangle{ 0, 0, (float)texLogo.width, (float)texLogo.height }, Rectangle{ 20.0f, 15.0f, texLogo.width * 0.45f, texLogo.height * 0.45f }, Vector2{ 0, 0 }, 0.0f, WHITE);

                float hostScale = 0.8f;
                Rectangle hostRect = { (float)currentW / 2.0f - (btnHost.width * hostScale) / 2.0f, (float)currentH * 0.25f, btnHost.width * hostScale, btnHost.height * hostScale };
                Texture2D hostToDraw = (CheckCollisionPointRec(mousePos, hostRect) && btnHostGreen.id > 0) ? btnHostGreen : btnHost;
                if (hostToDraw.id > 0) DrawTexturePro(hostToDraw, Rectangle{ 0, 0, (float)hostToDraw.width, (float)hostToDraw.height }, hostRect, Vector2{ 0, 0 }, 0.0f, WHITE);

                float joinScale = 0.8f;
                Rectangle joinRect = { (float)currentW / 2.0f - (btnJoin.width * joinScale) / 2.0f, hostRect.y + hostRect.height + 15.0f, btnJoin.width * joinScale, btnJoin.height * joinScale };
                Texture2D joinToDraw = (CheckCollisionPointRec(mousePos, joinRect) && btnJoinGreen.id > 0) ? btnJoinGreen : btnJoin;
                if (joinToDraw.id > 0) DrawTexturePro(joinToDraw, Rectangle{ 0, 0, (float)joinToDraw.width, (float)joinToDraw.height }, joinRect, Vector2{ 0, 0 }, 0.0f, WHITE);

                float codeScale = 0.8f;
                Rectangle codeRect = { (float)currentW / 2.0f - (texCode.width * codeScale) / 2.0f, joinRect.y + joinRect.height + 15.0f, texCode.width * codeScale, texCode.height * codeScale };
                Texture2D codeToDraw = ((CheckCollisionPointRec(mousePos, codeRect) || isCodeBoxActive) && texCodeGreen.id > 0) ? texCodeGreen : texCode;
                if (codeToDraw.id > 0) DrawTexturePro(codeToDraw, Rectangle{ 0, 0, (float)codeToDraw.width, (float)codeToDraw.height }, codeRect, Vector2{ 0, 0 }, 0.0f, WHITE);

                if (letterCount > 0) DrawText(roomCode, (int)(codeRect.x + codeRect.width / 2.0f - MeasureText(roomCode, 30) / 2.0f), (int)(codeRect.y + 20), 30, WHITE);

                float goBackW = btnGoBack.width * 0.8f;
                float goBackH = btnGoBack.height * 0.8f;
                Rectangle goBackRect = { 15.0f, (float)currentH - goBackH - 15.0f, goBackW, goBackH };
                if (btnGoBack.id > 0) DrawTexturePro(btnGoBack, Rectangle{ 0, 0, (float)btnGoBack.width, (float)btnGoBack.height }, goBackRect, Vector2{ 0, 0 }, 0.0f, CheckCollisionPointRec(mousePos, goBackRect) ? LIGHTGRAY : WHITE);
            }

            if (gameState == 2) {
                if (texLogo.id > 0) DrawTexturePro(texLogo, Rectangle{ 0, 0, (float)texLogo.width, (float)texLogo.height }, Rectangle{ 20.0f, 15.0f, texLogo.width * 0.45f, texLogo.height * 0.45f }, Vector2{ 0, 0 }, 0.0f, WHITE);

                Rectangle skeldRect = { 120.0f, 110.0f, 250.0f, 468.0f };
                if (texSkeldHost.id > 0) DrawTexturePro(texSkeldHost, Rectangle{ 0, 0, (float)texSkeldHost.width, (float)texSkeldHost.height }, skeldRect, Vector2{ 0, 0 }, 0.0f, WHITE);

                float numSize = 45.0f;
                float startX = 600.0f;
                float playersY = 180.0f;
                float impostersY = 340.0f;
                float spacing = 10.0f;

                if (texPlayersLabel.id > 0) {
                    float labelH = numSize;
                    float labelW = ((float)texPlayersLabel.width / texPlayersLabel.height) * labelH;
                    DrawTexturePro(texPlayersLabel, Rectangle{ 0, 0, (float)texPlayersLabel.width, (float)texPlayersLabel.height }, Rectangle{ startX - labelW - 15.0f, playersY, labelW, labelH }, Vector2{ 0, 0 }, 0.0f, WHITE);
                }

                for (int i = 0; i < 7; i++) {
                    int val = i + 4;
                    Rectangle numRect = { startX + i * (numSize + spacing), playersY, numSize, numSize };
                    Color tint = (selectedMaxPlayers == val) ? GREEN : (CheckCollisionPointRec(mousePos, numRect) ? LIGHTGRAY : WHITE);
                    if (texPlayerNum[i].id > 0) DrawTexturePro(texPlayerNum[i], Rectangle{ 0, 0, (float)texPlayerNum[i].width, (float)texPlayerNum[i].height }, numRect, Vector2{ 0, 0 }, 0.0f, tint);
                }

                if (texImpostersLabel.id > 0) {
                    float labelH = numSize;
                    float labelW = ((float)texImpostersLabel.width / texImpostersLabel.height) * labelH;
                    DrawTexturePro(texImpostersLabel, Rectangle{ 0, 0, (float)texImpostersLabel.width, (float)texImpostersLabel.height }, Rectangle{ startX - labelW - 15.0f, impostersY, labelW, labelH }, Vector2{ 0, 0 }, 0.0f, WHITE);
                }

                for (int i = 0; i < 3; i++) {
                    int val = i + 1;
                    Rectangle numRect = { startX + i * (numSize + spacing), impostersY, numSize, numSize };
                    Color tint = (selectedImposters == val) ? RED : (CheckCollisionPointRec(mousePos, numRect) ? LIGHTGRAY : WHITE);
                    if (texImposterNum[i].id > 0) DrawTexturePro(texImposterNum[i], Rectangle{ 0, 0, (float)texImposterNum[i].width, (float)texImposterNum[i].height }, numRect, Vector2{ 0, 0 }, 0.0f, tint);
                }

                float goBackW = btnGoBack.width * 0.8f;
                float goBackH = btnGoBack.height * 0.8f;
                Rectangle hostGoBackRect = { 20.0f, (float)currentH - goBackH - 20.0f, goBackW, goBackH };
                if (btnGoBack.id > 0) DrawTexturePro(btnGoBack, Rectangle{ 0, 0, (float)btnGoBack.width, (float)btnGoBack.height }, hostGoBackRect, Vector2{ 0, 0 }, 0.0f, CheckCollisionPointRec(mousePos, hostGoBackRect) ? LIGHTGRAY : WHITE);

                float confirmHostScale = 0.8f;
                float confirmHostW = (btnHost.width > 0) ? btnHost.width * confirmHostScale : 200.0f;
                float confirmHostH = (btnHost.height > 0) ? btnHost.height * confirmHostScale : 80.0f;
                Rectangle confirmHostRect = { (float)currentW - confirmHostW - 20.0f, (float)currentH - confirmHostH - 20.0f, confirmHostW, confirmHostH };
                
                Texture2D confirmHostToDraw = (CheckCollisionPointRec(mousePos, confirmHostRect) && btnHostGreen.id > 0) ? btnHostGreen : btnHost;
                if (confirmHostToDraw.id > 0) DrawTexturePro(confirmHostToDraw, Rectangle{ 0, 0, (float)confirmHostToDraw.width, (float)confirmHostToDraw.height }, confirmHostRect, Vector2{ 0, 0 }, 0.0f, WHITE);
            }

            if (gameState == 3) {
                if (texLobby.id > 0) {
                    DrawTexturePro(
                        texLobby,
                        Rectangle{ 0, 0, (float)texLobby.width, (float)texLobby.height },
                        lobbyImageRect,
                        Vector2{ 0, 0 }, 0.0f, WHITE
                    );
                }

                int frameToShow = isPlayerMoving ? (1 + walkFrameIndex) : 0;
                Texture2D texPlayerCurrent = playerFrameTextures[frameToShow];
                if (texPlayerCurrent.id > 0) {
                    float playerDrawW = texPlayerCurrent.width * playerScale;
                    float playerDrawH = texPlayerCurrent.height * playerScale;
                    float srcW = playerFacingLeft ? -(float)texPlayerCurrent.width : (float)texPlayerCurrent.width;
                    DrawTexturePro(
                        texPlayerCurrent,
                        Rectangle{ 0, 0, srcW, (float)texPlayerCurrent.height },
                        Rectangle{ playerPos.x, playerPos.y, playerDrawW, playerDrawH },
                        Vector2{ 0, 0 }, 0.0f, WHITE
                    );
                }

                if (isColorMenuOpen) {
                    DrawRectangle(0, 0, currentW, currentH, Fade(BLACK, 0.6f));

                    float swatchSize = 80.0f, spacing = 15.0f;
                    int cols = 6, rows = 2;
                    float gridW = cols * swatchSize + (cols - 1) * spacing;
                    float gridH = rows * swatchSize + (rows - 1) * spacing;
                    float gridX = (float)currentW / 2.0f - gridW / 2.0f;
                    float gridY = (float)currentH / 2.0f - gridH / 2.0f;

                    const char* title = "Choisis ta couleur (R pour fermer)";
                    DrawText(title, (int)((float)currentW / 2.0f - MeasureText(title, 28) / 2.0f), (int)(gridY - 50.0f), 28, WHITE);

                    for (int i = 0; i < colorPaletteCount; i++) {
                        int col = i % cols, row = i / cols;
                        Rectangle swatchRect = { gridX + col * (swatchSize + spacing), gridY + row * (swatchSize + spacing), swatchSize, swatchSize };
                        bool isHover = CheckCollisionPointRec(mousePos, swatchRect);
                        bool isSelected = (colorPalette[i].color.r == selectedPlayerColor.r && colorPalette[i].color.g == selectedPlayerColor.g && colorPalette[i].color.b == selectedPlayerColor.b);

                        DrawRectangleRec(swatchRect, colorPalette[i].color);
                        DrawRectangleLinesEx(swatchRect, (isSelected || isHover) ? 4.0f : 2.0f, (isSelected || isHover) ? WHITE : BLACK);
                    }
                }
            }

            DrawCircleV(mousePos, 4, WHITE);
            DrawCircleLines((int)mousePos.x, (int)mousePos.y, 5, BLACK);

        EndDrawing();
    }

    if (bgTexture.id > 0) UnloadTexture(bgTexture);
    UnloadTexture(btnOnline);
    UnloadTexture(btnOnlineGreen);
    UnloadTexture(btnSettings);
    UnloadTexture(texLogo);
    UnloadTexture(texSettingsMenu);
    UnloadTexture(texFb);
    UnloadTexture(btnHost);
    UnloadTexture(btnHostGreen);
    UnloadTexture(btnJoin);
    UnloadTexture(btnJoinGreen);
    UnloadTexture(btnGoBack);
    UnloadTexture(texCode);
    UnloadTexture(texCodeGreen);
    UnloadTexture(texDoesntExist);
    UnloadTexture(texSkeldHost);
    UnloadTexture(texLobby);
    for (int i = 0; i < PLAYER_FRAME_COUNT; i++) {
        if (playerFrameTextures[i].id > 0) UnloadTexture(playerFrameTextures[i]);
        if (playerFrameImagesCPU[i].data != nullptr) UnloadImage(playerFrameImagesCPU[i]);
    }

    UnloadTexture(texPlayersLabel);
    for (int i = 0; i < 7; i++) UnloadTexture(texPlayerNum[i]);

    UnloadTexture(texImpostersLabel);
    for (int i = 0; i < 3; i++) UnloadTexture(texImposterNum[i]);

    CloseWindow();
    return 0;
}
