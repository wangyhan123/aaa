#include <graphics.h>
#include <conio.h>
#include <time.h>
#include <vector>
#include <string>
#include <Windows.h>
#include <cmath>
#include <algorithm>

using namespace std;

COLORREF MyHSVtoRGB(float H, float S, float V) {
    float R, G, B;
    float h = H / 60.0f;
    int i = static_cast<int>(floor(h));
    float f = h - i;
    float p = V * (1 - S);
    float q = V * (1 - S * f);
    float t = V * (1 - S * (1 - f));

    switch (i) {
    case 0: R = V; G = t; B = p; break;
    case 1: R = q; G = V; B = p; break;
    case 2: R = p; G = V; B = t; break;
    case 3: R = p; G = q; B = V; break;
    case 4: R = t; G = p; B = V; break;
    case 5: R = V; G = p; B = q; break;
    default: R = G = B = 0;
    }

    return RGB(
        static_cast<int>(R * 255),
        static_cast<int>(G * 255),
        static_cast<int>(B * 255)
    );
}
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int BLOCK_SIZE = 30;
const int BOARD_OFFSET_X = 50;
const int BOARD_OFFSET_Y = 50;
const int SHAPES[7][4][4] = {
    {
        {0,0,0,0},
        {1,1,1,1},
        {0,0,0,0},
        {0,0,0,0}
    },
    {
        {1,0,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    {
        {0,0,1,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    {
        {0,1,1,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    {
        {0,1,1,0},
        {1,1,0,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    {
        {0,1,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    {
        {1,1,0,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    }
};
const COLORREF COLORS[7] = {
CYAN,
BLUE,
RGB(255, 165, 0),
YELLOW,
    GREEN,
    MAGENTA,
    RED
};

// 粒子效果结构
struct Particle {
    int x, y;
    float vx, vy;
    int life;
    int maxLife;
    COLORREF color;
    int size;
};

class Tetromino {
public:
    int shape[4][4];
    int type;
    int x, y;
    COLORREF color;

    Tetromino() {
        reset();
    }

    void reset() {
        type = rand() % 7;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                shape[i][j] = SHAPES[type][i][j];
            }
        }
        color = COLORS[type];
        x = BOARD_WIDTH / 2 - 2;
        y = 0;
    }

    void rotate() {
        int temp[4][4];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                temp[i][j] = shape[i][j];
            }
        }
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                shape[j][3 - i] = temp[i][j];
            }
        }
    }

    void draw(int offsetX = 0, int offsetY = 0) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (shape[i][j]) {
                    int posX = BOARD_OFFSET_X + (x + j) * BLOCK_SIZE + offsetX;
                    int posY = BOARD_OFFSET_Y + (y + i) * BLOCK_SIZE + offsetY;

                    // 绘制3D效果底部阴影
                    setfillcolor(RGB(
                        max(0, GetRValue(color) - 50),
                        max(0, GetGValue(color) - 50),
                        max(0, GetBValue(color) - 50)
                    ));
                    solidrectangle(posX, posY + 3, posX + BLOCK_SIZE - 1, posY + BLOCK_SIZE - 1 + 3);
                    for (int py = 0; py < BLOCK_SIZE; py++) {
                        int highlight = min(255, 40 - py * 2);
                        setlinecolor(RGB(
                            min(255, GetRValue(color) + highlight),
                            min(255, GetGValue(color) + highlight),
                            min(255, GetBValue(color) + highlight)
                        ));
                        line(posX, posY + py, posX + BLOCK_SIZE - 1, posY + py);
                    }
                    setlinecolor(WHITE);
                    rectangle(posX, posY, posX + BLOCK_SIZE - 1, posY + BLOCK_SIZE - 1);
                }
            }
        }
    }
};

class Game {
public:
    int board[BOARD_HEIGHT][BOARD_WIDTH];
    Tetromino current, next;
    int score;
    int level;
    int linesCleared;
    bool gameOver;
    clock_t lastFallTime;
    vector<Particle> particles;
    vector<int> clearedLines;
    clock_t clearAnimationTime;
    bool animatingClear;
    bool mute;
    bool showSpeedUp;
    clock_t speedUpStartTime;
    int lastLevel;

    Game() : animatingClear(false), mute(false), showSpeedUp(false), lastLevel(1) {
        init();
    }

    void init() {
        for (int i = 0; i < BOARD_HEIGHT; i++) {
            for (int j = 0; j < BOARD_WIDTH; j++) {
                board[i][j] = -1;
            }
        }
        score = 0;
        level = 1;
        linesCleared = 0;
        gameOver = false;
        animatingClear = false;
        particles.clear();
        clearedLines.clear();
        showSpeedUp = false;
        lastLevel = 1;
        current.reset();
        next.reset();
        lastFallTime = clock();
    }
    void createParticles(int posX, int posY, COLORREF color, int count = 15) {
        for (int i = 0; i < count; i++) {
            Particle p;
            p.x = posX + rand() % (BLOCK_SIZE - 8) + 4;
            p.y = posY + rand() % (BLOCK_SIZE - 8) + 4;
            p.vx = (rand() % 10 - 5) / 5.0f;
            p.vy = (rand() % 5 - 5) / 3.0f;
            p.life = rand() % 30 + 10;
            p.maxLife = p.life;
            p.color = color;
            p.size = rand() % 4 + 1;
            particles.push_back(p);
        }
    }
    void updateParticles() {
        for (auto it = particles.begin(); it != particles.end();) {
            it->x += it->vx;
            it->y += it->vy;
            it->vy += 0.05f;
            it->life--;

            if (it->life <= 0) {
                it = particles.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    void drawParticles() {
        for (auto& p : particles) {
            float alpha = (float)p.life / p.maxLife;
            setfillcolor(RGB(
                GetRValue(p.color),
                GetGValue(p.color),
                GetBValue(p.color)
            ));
            setlinecolor(WHITE);
            fillcircle(p.x, p.y, p.size);
        }
    }

    bool isValidMove(int newX, int newY, int newShape[4][4]) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (newShape[i][j]) {
                    int boardX = newX + j;
                    int boardY = newY + i;
                    if (boardX < 0 || boardX >= BOARD_WIDTH || boardY >= BOARD_HEIGHT) {
                        return false;
                    }
                    if (boardY >= 0 && board[boardY][boardX] != -1) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void mergeTetromino() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (current.shape[i][j]) {
                    int boardX = current.x + j;
                    int boardY = current.y + i;

                    if (boardY >= 0) {
                        board[boardY][boardX] = current.type;
                        int posX = BOARD_OFFSET_X + boardX * BLOCK_SIZE;
                        int posY = BOARD_OFFSET_Y + boardY * BLOCK_SIZE;
                        createParticles(posX, posY, current.color);
                    }
                }
            }
        }
    }

    void clearLines() {
        clearedLines.clear();

        for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
            bool lineFull = true;
            for (int j = 0; j < BOARD_WIDTH; j++) {
                if (board[i][j] == -1) {
                    lineFull = false;
                    break;
                }
            }

            if (lineFull) {
                clearedLines.push_back(i);
            }
        }
        if (!clearedLines.empty()) {
            animatingClear = true;
            clearAnimationTime = clock();
        }
        if (!clearedLines.empty()) {
            linesCleared += clearedLines.size();
            score += clearedLines.size() * clearedLines.size() * 100 * level;
            int newLevel = linesCleared / 10 + 1;
            if (newLevel != level && newLevel > 1) {
                showSpeedUp = true;
                speedUpStartTime = clock();
            }
            level = newLevel;
        }
    }
    void drawBackground() {
        for (int i = 0; i < 700; i++) {
            float p = (float)i / 700;
            int r = 20 + p * 30;
            int g = 20 + p * 50;
            int b = 40 + p * 70;
            setlinecolor(RGB(r, g, b));
            line(0, i, 800, i);
        }
        setfillcolor(WHITE);
        for (int i = 0; i < 200; i++) {
            int x = rand() % 800;
            int y = rand() % 700;
            int size = rand() % 2 + 1;
            solidcircle(x, y, size);
        }
        setlinecolor(RGB(40, 40, 60));
        for (int y = BOARD_OFFSET_Y; y <= BOARD_OFFSET_Y + BOARD_HEIGHT * BLOCK_SIZE; y += BLOCK_SIZE) {
            line(BOARD_OFFSET_X, y, BOARD_OFFSET_X + BOARD_WIDTH * BLOCK_SIZE, y);
        }
        for (int x = BOARD_OFFSET_X; x <= BOARD_OFFSET_X + BOARD_WIDTH * BLOCK_SIZE; x += BLOCK_SIZE) {
            line(x, BOARD_OFFSET_Y, x, BOARD_OFFSET_Y + BOARD_HEIGHT * BLOCK_SIZE);
        }
    }

    void drawBoard() {

        static IMAGE buffer(800, 700);
        SetWorkingImage(&buffer);
        drawBackground();
        setlinecolor(RGB(100, 150, 200));
        setlinestyle(PS_SOLID, 3);
        rectangle(BOARD_OFFSET_X - 7, BOARD_OFFSET_Y - 7,
            BOARD_OFFSET_X + BOARD_WIDTH * BLOCK_SIZE + 7,
            BOARD_OFFSET_Y + BOARD_HEIGHT * BLOCK_SIZE + 7);
        setlinestyle(PS_SOLID, 1);
        setfillcolor(RGB(20, 20, 30));
        solidrectangle(BOARD_OFFSET_X, BOARD_OFFSET_Y,
            BOARD_OFFSET_X + BOARD_WIDTH * BLOCK_SIZE,
            BOARD_OFFSET_Y + BOARD_HEIGHT * BLOCK_SIZE);
        for (int i = 0; i < BOARD_HEIGHT; i++) {
            for (int j = 0; j < BOARD_WIDTH; j++) {
                int posX = BOARD_OFFSET_X + j * BLOCK_SIZE;
                int posY = BOARD_OFFSET_Y + i * BLOCK_SIZE;
                setlinecolor(RGB(60, 80, 120));
                rectangle(posX, posY, posX + BLOCK_SIZE - 1, posY + BLOCK_SIZE - 1);
                if (board[i][j] != -1) {
                    COLORREF color = COLORS[board[i][j]];
                    for (int py = 0; py < BLOCK_SIZE; py++) {
                        int highlight = min(255, 40 - py * 2);
                        setlinecolor(RGB(
                            min(255, GetRValue(color) + highlight),
                            min(255, GetGValue(color) + highlight),
                            min(255, GetBValue(color) + highlight)
                        ));
                        line(posX, posY + py, posX + BLOCK_SIZE - 1, posY + py);
                    }
                    setlinecolor(WHITE);
                    rectangle(posX, posY, posX + BLOCK_SIZE - 1, posY + BLOCK_SIZE - 1);
                }
            }
        }
        if (animatingClear) {
            double elapsed = (clock() - clearAnimationTime) % 1000;
            bool flash = (elapsed < 200 || (elapsed > 400 && elapsed < 600) || (elapsed > 800));

            for (int row : clearedLines) {
                setfillcolor(flash ? RGB(255, 255, 200) : RGB(200, 200, 255));
                solidrectangle(BOARD_OFFSET_X, BOARD_OFFSET_Y + row * BLOCK_SIZE,
                    BOARD_OFFSET_X + BOARD_WIDTH * BLOCK_SIZE,
                    BOARD_OFFSET_Y + (row + 1) * BLOCK_SIZE);
            }
            if (clock() - clearAnimationTime > 1000) {
                animatingClear = false;
                for (int row : clearedLines) {
                    for (int k = row; k > 0; k--) {
                        for (int j = 0; j < BOARD_WIDTH; j++) {
                            board[k][j] = board[k - 1][j];
                        }
                    }
                    for (int j = 0; j < BOARD_WIDTH; j++) {
                        board[0][j] = -1;
                    }
                }
                clearedLines.clear();
            }
        }
        current.draw();
        drawParticles();
        setfillcolor(RGB(30, 30, 50));
        solidroundrect(430, 40, 710, 280, 10, 10);
        setlinecolor(RGB(100, 150, 200));
        roundrect(430, 40, 710, 280, 10, 10);
        settextcolor(WHITE);
        setbkmode(TRANSPARENT);
        settextstyle(24, 0, _T("Consolas"));
        const int NEXT_TEXT_X = 520;
        const int NEXT_TEXT_Y = 60;
        settextcolor(BLACK);
        outtextxy(NEXT_TEXT_X + 2, NEXT_TEXT_Y + 2, _T("Next"));
        settextcolor(YELLOW);
        outtextxy(NEXT_TEXT_X, NEXT_TEXT_Y, _T("Next"));
        const int PREVIEW_AREA_X1 = 470;
        const int PREVIEW_AREA_Y1 = 85;
        const int PREVIEW_AREA_X2 = 670;
        const int PREVIEW_AREA_Y2 = 135;

        setfillcolor(RGB(50, 50, 80));
        solidrectangle(PREVIEW_AREA_X1, PREVIEW_AREA_Y1, PREVIEW_AREA_X2, PREVIEW_AREA_Y2);
        const int BLOCK_PREVIEW_SIZE = 20;
        const int PREVIEW_CENTER_X = 570;
        const int PREVIEW_CENTER_Y = 110;
        int minX = 4, maxX = 0, minY = 4, maxY = 0;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (next.shape[i][j]) {
                    if (j < minX) minX = j;
                    if (j > maxX) maxX = j;
                    if (i < minY) minY = i;
                    if (i > maxY) maxY = i;
                }
            }
        }
        int blockWidth = (maxX - minX + 1) * BLOCK_PREVIEW_SIZE;
        int blockHeight = (maxY - minY + 1) * BLOCK_PREVIEW_SIZE;
        int offsetX = PREVIEW_CENTER_X - blockWidth / 2 - minX * BLOCK_PREVIEW_SIZE;
        int offsetY = PREVIEW_CENTER_Y - blockHeight / 2 - minY * BLOCK_PREVIEW_SIZE;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (next.shape[i][j]) {
                    int posX = offsetX + j * BLOCK_PREVIEW_SIZE;
                    int posY = offsetY + i * BLOCK_PREVIEW_SIZE;
                    setfillcolor(RGB(
                        max(0, GetRValue(next.color) - 30),
                        max(0, GetGValue(next.color) - 30),
                        max(0, GetBValue(next.color) - 30)
                    ));
                    solidrectangle(posX + 2, posY + 2, posX + BLOCK_PREVIEW_SIZE, posY + BLOCK_PREVIEW_SIZE);
                    setfillcolor(next.color);
                    solidrectangle(posX, posY, posX + BLOCK_PREVIEW_SIZE - 2, posY + BLOCK_PREVIEW_SIZE - 2);
                    setlinecolor(WHITE);
                    rectangle(posX, posY, posX + BLOCK_PREVIEW_SIZE - 2, posY + BLOCK_PREVIEW_SIZE - 2);
                }
            }
        }

        const int SCORE_PANEL_Y = 150;
        settextstyle(24, 0, _T("Consolas"));
        settextcolor(BLACK);
        outtextxy(462, SCORE_PANEL_Y + 1, _T("Score:"));
        settextcolor(WHITE);
        outtextxy(460, SCORE_PANEL_Y, _T("Score:"));
        settextcolor(BLACK);
        outtextxy(462, SCORE_PANEL_Y + 31, _T("Level:"));
        settextcolor(WHITE);
        outtextxy(460, SCORE_PANEL_Y + 30, _T("Level:"));
        settextcolor(BLACK);
        outtextxy(462, SCORE_PANEL_Y + 61, _T("Lines:"));
        settextcolor(WHITE);
        outtextxy(460, SCORE_PANEL_Y + 60, _T("Lines:"));
        settextcolor(YELLOW);
        char scoreStr[20];
        sprintf_s(scoreStr, "%d", score);
        outtextxy(570, SCORE_PANEL_Y, s2ws(scoreStr).c_str());
        char levelStr[20];
        sprintf_s(levelStr, "%d", level);
        outtextxy(570, SCORE_PANEL_Y + 30, s2ws(levelStr).c_str());
        char linesStr[20];
        sprintf_s(linesStr, "%d", linesCleared);
        outtextxy(570, SCORE_PANEL_Y + 60, s2ws(linesStr).c_str());
        if (showSpeedUp) {
            clock_t currentTime = clock();
            if (currentTime - speedUpStartTime < 5000) {
                LOGFONT originalFont;
                gettextstyle(&originalFont);
                settextstyle(60, 0, _T("Arial Rounded MT Bold"));
                setbkmode(TRANSPARENT);
                const wchar_t* speedUpText = _T("SPEED UP!");
                int textWidth = textwidth(speedUpText);
                int textHeight = textheight(speedUpText);
                int x = (800 - textWidth) / 2;
                int y = 500;
                settextcolor(RGB(50, 100, 200));
                for (int i = 0; i < 8; i++) {
                    outtextxy(x - 1, y - 1, speedUpText);
                    outtextxy(x + 1, y - 1, speedUpText);
                    outtextxy(x - 1, y + 1, speedUpText);
                    outtextxy(x + 1, y + 1, speedUpText);
                }

                for (int py = 0; py < textHeight; py++) {
                    float progress = (float)py / textHeight;
                    settextcolor(MyHSVtoRGB(progress * 60.0f + 200.0f, 0.8f, 0.9f));
                    outtextxy(x, y + py, speedUpText);
                }
                settextcolor(WHITE);
                outtextxy(x, y, speedUpText);
                settextstyle(&originalFont);
            }
            else {
                showSpeedUp = false;
            }
        }
        if (gameOver) {
            settextcolor(RED);
            settextstyle(48, 0, _T("Consolas"));
            outtextxy(150, 280, _T("GAME OVER"));

            settextstyle(24, 0, _T("Consolas"));
            outtextxy(150, 340, _T("Press R to restart"));
        }
        settextcolor(RGB(200, 220, 255));
        settextstyle(16, 0, _T("Consolas"));
        outtextxy(50, 650, _T("操作:"));
        outtextxy(150, 650, _T("移动：← →"));
        outtextxy(250, 650, _T("旋转：↑"));
        outtextxy(350, 650, _T("加速降落：↓"));
        outtextxy(450, 650, _T("一键下落：空格"));
        setfillcolor(mute ? RGB(200, 100, 100) : RGB(100, 200, 100));
        solidroundrect(720, 45, 770, 95, 8, 8);
        setlinecolor(RGB(200, 220, 255));
        setlinestyle(PS_SOLID, 2);
        roundrect(720, 45, 770, 95, 8, 8);
        settextcolor(WHITE);
        settextstyle(20, 0, _T("Webdings"));
        outtextxy(735, 65, mute ? _T("q") : _T("Q"));
        SetWorkingImage(NULL);
        putimage(0, 0, &buffer);
    }

    void update() {
        if (gameOver) return;
        updateParticles();


        if (showSpeedUp) {
            clock_t currentTime = clock();
            if (currentTime - speedUpStartTime > 5000) {
                showSpeedUp = false;
            }
            else {
                return;
            }
        }

        clock_t currentTime = clock();
        double fallInterval = 1000.0 / (level * 0.8 + 1);
        if (currentTime - lastFallTime > fallInterval) {
            if (isValidMove(current.x, current.y + 1, current.shape)) {
                current.y++;
            }
            else {
                mergeTetromino();
                clearLines();
                current = next;
                next.reset();
                if (!isValidMove(current.x, current.y, current.shape)) {
                    gameOver = true;
                }
            }
            lastFallTime = currentTime;
        }
    }

    void moveLeft() {
        if (isValidMove(current.x - 1, current.y, current.shape)) {
            current.x--;
        }
    }

    void moveRight() {
        if (isValidMove(current.x + 1, current.y, current.shape)) {
            current.x++;
        }
    }

    void rotate() {
        int temp[4][4];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                temp[i][j] = current.shape[i][j];
            }
        }

        current.rotate();
        if (!isValidMove(current.x, current.y, current.shape)) {
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    current.shape[i][j] = temp[i][j];
                }
            }
        }
    }

    void hardDrop() {
        int dropCount = 0;
        while (isValidMove(current.x, current.y + 1, current.shape)) {
            current.y++;
            dropCount++;
        }
        score += dropCount;
    }

    wstring s2ws(const string& s) {
        int len;
        int slength = (int)s.length() + 1;
        len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
        wchar_t* buf = new wchar_t[len];
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
        wstring r(buf);
        delete[] buf;
        return r;
    }
};
void drawMatryoshka(int x, int y, int size) {

    setfillcolor(RGB(255, 100, 100));
    solidcircle(x, y, size / 2);
    setfillcolor(RGB(250, 250, 200));
    solidcircle(x, y - size / 3, size / 3);
    setfillcolor(BLACK);
    solidcircle(x - size / 8, y - size / 3, size / 20);
    solidcircle(x + size / 8, y - size / 3, size / 20);
    for (int i = -size / 16; i <= size / 16; i++) {
        solidcircle(x + i, y - size / 4, size / 60);
    }
    setfillcolor(GREEN);
    for (int i = -size / 4; i <= size / 4; i += size / 8) {
        solidcircle(x + i, y + size / 8, size / 16);
    }
    setfillcolor(RGB(255, 200, 200));
    solidrectangle(x - size / 3, y - size / 2, x + size / 3, y - size / 3);
    setfillcolor(GREEN);
    solidcircle(x - size / 4, y - size / 2 + size / 10, size / 12);
    solidcircle(x + size / 4, y - size / 2 + size / 10, size / 12);
    solidcircle(x, y - size / 2 + size / 10, size / 12);
}
void drawStartScreen(bool startButtonHovered) {
    static IMAGE buffer(800, 700);
    SetWorkingImage(&buffer);
    for (int i = 0; i < 700; i++) {
        float p = (float)i / 700;
        int r = 20 + p * 40;
        int g = 10 + p * 30;
        int b = 50 + p * 100;
        setlinecolor(RGB(r, g, b));
        line(0, i, 800, i);
    }
    static float rotation = 0;
    rotation += 0.01;
    if (rotation > 6.28) rotation = 0;

    for (int i = 0; i < 8; i++) {
        double angle = i * 45 * 3.14159 / 180.0 + rotation;
        int size = 100;
        int centerX = 400 + static_cast<int>(cos(angle) * 200);
        int centerY = 300 + static_cast<int>(sin(angle) * 200);
        setfillcolor(MyHSVtoRGB((i * 45 + rotation * 180 / 3.14), 0.7, 0.7));
        solidcircle(centerX, centerY, size / 2);
    }
    drawMatryoshka(200, 500, 120);
    drawMatryoshka(600, 200, 80);
    drawMatryoshka(300, 200, 60);
    settextcolor(RGB(100, 0, 0));
    setbkmode(TRANSPARENT);
    settextstyle(76, 0, _T("Arial"));
    outtextxy(203, 153, L"俄罗斯方块");

    settextcolor(YELLOW);
    outtextxy(200, 150, L"俄罗斯方块");
    setfillcolor(startButtonHovered ? RGB(70, 100, 200) : RGB(40, 70, 150));
    solidroundrect(300, 350, 500, 400, 10, 10);

    setlinecolor(WHITE);
    setlinestyle(PS_SOLID, 2);
    roundrect(300, 350, 500, 400, 10, 10);

    settextcolor(WHITE);
    settextstyle(36, 0, _T("Arial"));
    outtextxy(350, 360, L"开始游戏");
    if (startButtonHovered) {
        setlinecolor(RGB(150, 180, 255));
        for (int i = 0; i < 3; i++) {
            roundrect(300 - i, 350 - i, 500 + i, 400 + i, 10, 10);
        }
    }
    settextcolor(RGB(230, 230, 150));
    settextstyle(16, 0, _T("Arial"));
    outtextxy(600, 670, L"作者：王语菡,魏宣亦");

    outtextxy(50, 670, L"按 ESC 退出游戏");
    setlinecolor(RGB(255, 215, 0));
    for (int i = 0; i < 5; i++) {
        rectangle(0 + i * 2, 0 + i * 2, 800 - i * 2, 700 - i * 2);
    }
    SetWorkingImage(NULL);
    putimage(0, 0, &buffer);
}
void drawCountdownScreen(int countdown) {
    static IMAGE buffer(800, 700);
    SetWorkingImage(&buffer);
    setfillcolor(RGB(20, 30, 50));
    solidrectangle(0, 0, 800, 700);
    setfillcolor(WHITE);
    for (int i = 0; i < 300; i++) {
        solidcircle(rand() % 800, rand() % 700, rand() % 2 + 1);
    }
    settextcolor(WHITE);
    setbkmode(TRANSPARENT);
    settextstyle(150, 0, _T("Arial"));

    wchar_t countStr[2];
    countStr[0] = L'0' + countdown;
    countStr[1] = L'\0';
    settextcolor(RGB(80, 120, 200));
    outtextxy(322, 252, countStr);
    settextcolor(WHITE);
    outtextxy(320, 250, countStr);
    SetWorkingImage(NULL);
    putimage(0, 0, &buffer);
}

int main() {
    initgraph(800, 700);
    srand((unsigned int)time(NULL));
    BeginBatchDraw();
    bool showStartScreen = true;
    bool startButtonHovered = false;

    while (showStartScreen) {

        MOUSEMSG mouseMsg;
        if (MouseHit()) {
            mouseMsg = GetMouseMsg();
            if (mouseMsg.x >= 300 && mouseMsg.x <= 500 &&
                mouseMsg.y >= 350 && mouseMsg.y <= 400) {
                startButtonHovered = true;


                if (mouseMsg.uMsg == WM_LBUTTONDOWN) {
                    showStartScreen = false;
                }
            }
            else {
                startButtonHovered = false;
            }
        }


        drawStartScreen(startButtonHovered);
        if (_kbhit()) {
            int key = _getch();
            if (key == 27) { // ESC键
                EndBatchDraw();
                closegraph();
                return 0;
            }
        }


        FlushBatchDraw();
        Sleep(20);
    }
    bool showCountdown = true;
    int countdown = 3;
    clock_t lastCountTime = clock();

    while (showCountdown) {

        drawCountdownScreen(countdown);
        FlushBatchDraw();
        if (clock() - lastCountTime > 1000) {
            countdown--;
            lastCountTime = clock();

            if (countdown <= 0) {
                showCountdown = false;
            }
        }


        if (_kbhit()) {
            int key = _getch();
            if (key == 27) {
                EndBatchDraw();
                closegraph();
                return 0;
            }
        }
        Sleep(20);
    }

    Game game;
    clock_t lastFrameTime = clock();
    const double frameRate = 60.0;
    const double frameDelay = 1000.0 / frameRate;
    while (true) {

        while (_kbhit()) {
            int key = _getch();
            if (key == 0 || key == 224) {
                key = _getch();
                switch (key) {
                case 75: game.moveLeft(); break;
                case 77: game.moveRight(); break;
                case 72: game.rotate(); break;
                case 80:
                    if (game.isValidMove(game.current.x, game.current.y + 1, game.current.shape)) {
                        game.current.y++;
                    }
                    break;
                }
            }
            else {

                switch (toupper(key)) {
                case 'A': game.moveLeft(); break;
                case 'D': game.moveRight(); break;
                case 'W': game.rotate(); break;
                case 'S':
                    if (game.isValidMove(game.current.x, game.current.y + 1, game.current.shape)) {
                        game.current.y++;
                    }
                    break;
                case ' ': game.hardDrop(); break;
                case 'R':
                    if (game.gameOver) {
                        game.init();
                    }
                    break;
                case 'M':
                    game.mute = !game.mute;
                    break;
                case 27:
                    EndBatchDraw();
                    closegraph();
                    return 0;
                }
            }
        }

        game.update();
        game.drawBoard();
        FlushBatchDraw();
        clock_t currentTime = clock();
        double elapsed = currentTime - lastFrameTime;
        if (elapsed < frameDelay) {
            Sleep(frameDelay - elapsed);
        }
        lastFrameTime = currentTime;
    }
    EndBatchDraw();
    closegraph();
    return 0;
}