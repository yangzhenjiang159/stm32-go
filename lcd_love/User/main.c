/*
 * 尚硅谷 STM32F103ZET6 + FSMC 液晶屏
 * 功能：轮播显示 "老婆我爱你!"（跳动爱心 + 逐字出现 + 左右滑动 + 配色轮播）
 *
 * 已通过分步验证（四角定位 + 单个汉字均正常）的确定配置：
 *   1. FSMC 时序：ADDSET=15、DATAST=255（本板 HCLK=72MHz，教程按 36MHz 的时序过快）
 *   2. 初始化序列：ILI9488 风格（含 F7h Adjust Control 3）
 *   3. 像素格式：3Ah = 0x55（16bit/像素，每像素 1 次 16 位写）
 *   4. 扫描方向：36h = 0x08；分辨率 320x480
 *   5. 复位脚：PG15
 *   6. 行列地址(2Ah/2Bh)：8 位字节流写法（教程写法）
 */
#include "lcd.h"
#include "delay.h"
#include "love_font.h"
#include "heart_table.h"

#define CH_SIZE  32
#define STR_LEN  LOVE_FONT_COUNT
#define TEXT_W   (STR_LEN * CH_SIZE)
#define TEXT_X0  ((LCD_W - TEXT_W) / 2)
#define TEXT_Y   330
#define TEXT_BAND_H 40

#define HEART_CX (LCD_W / 2)
#define HEART_CY 170

/* 各轮配色：{前景色, 背景色} */
static const uint16_t g_palette[][2] = {
    {RED,     BLACK},
    {YELLOW,  BLUE},
    {WHITE,   RED},
    {CYAN,    BLACK},
    {GREEN,   BLACK},
    {BLUE,    YELLOW},
    {MAGENTA, BLACK},
    {BLACK,   YELLOW},
};
#define PALETTE_N (sizeof(g_palette) / sizeof(g_palette[0]))

/* ---------- 1. 屏幕初始化 ---------- */

static void FSMC_SetSlowTiming(void)
{
    FSMC_Bank1->BTCR[7] &= ~((uint32_t)0xF | ((uint32_t)0xFF << 8));
    FSMC_Bank1->BTCR[7] |= (uint32_t)15 | ((uint32_t)255 << 8);
}

static void LCD_InitScreen(void)
{
    FSMC_Init();
    FSMC_SetSlowTiming();
    LCD_Reset();
    LCD_BGOn();

    LCD_WriteCmd(0xF7);
    LCD_WriteData(0xA9);
    LCD_WriteData(0x51);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x82);

    LCD_WriteCmd(0xC0);
    LCD_WriteData(0x11);
    LCD_WriteData(0x09);

    LCD_WriteCmd(0xC1);
    LCD_WriteData(0x41);
    LCD_WriteData(0x00);

    LCD_WriteCmd(0xC5);
    LCD_WriteData(0x00);
    LCD_WriteData(0x0A);
    LCD_WriteData(0x80);

    LCD_WriteCmd(0xB1);
    LCD_WriteData(0xB0);
    LCD_WriteData(0x11);

    LCD_WriteCmd(0xB4);
    LCD_WriteData(0x02);

    LCD_WriteCmd(0xB6);
    LCD_WriteData(0x02);
    LCD_WriteData(0x22);

    LCD_WriteCmd(0xB7);
    LCD_WriteData(0xC6);

    LCD_WriteCmd(0xBE);
    LCD_WriteData(0x00);
    LCD_WriteData(0x04);

    LCD_WriteCmd(0xE0);
    LCD_WriteData(0x00);
    LCD_WriteData(0x07);
    LCD_WriteData(0x10);
    LCD_WriteData(0x09);
    LCD_WriteData(0x17);
    LCD_WriteData(0x0B);
    LCD_WriteData(0x41);
    LCD_WriteData(0x89);
    LCD_WriteData(0x4B);
    LCD_WriteData(0x0A);
    LCD_WriteData(0x0C);
    LCD_WriteData(0x0E);
    LCD_WriteData(0x18);
    LCD_WriteData(0x1B);
    LCD_WriteData(0x0F);

    LCD_WriteCmd(0xE1);
    LCD_WriteData(0x00);
    LCD_WriteData(0x17);
    LCD_WriteData(0x1A);
    LCD_WriteData(0x04);
    LCD_WriteData(0x0E);
    LCD_WriteData(0x06);
    LCD_WriteData(0x2F);
    LCD_WriteData(0x45);
    LCD_WriteData(0x43);
    LCD_WriteData(0x02);
    LCD_WriteData(0x0A);
    LCD_WriteData(0x09);
    LCD_WriteData(0x32);
    LCD_WriteData(0x36);
    LCD_WriteData(0x0F);

    LCD_WriteCmd(0x3A);          /* 16bit/像素 */
    LCD_WriteData(0x55);

    LCD_WriteCmd(0x36);          /* 扫描方向 */
    LCD_WriteData(0x08);

    LCD_WriteCmd(0x11);
    Delay_ms(120);
    LCD_WriteCmd(0x29);
    Delay_ms(50);
}

/* ---------- 2. 窗口设置（8 位字节流写法，已验证） ---------- */

static void SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    LCD_WriteCmd(0x2A);
    LCD_WriteData(x >> 8);
    LCD_WriteData(x & 0xFF);
    LCD_WriteData((x + w - 1) >> 8);
    LCD_WriteData((x + w - 1) & 0xFF);
    LCD_WriteCmd(0x2B);
    LCD_WriteData(y >> 8);
    LCD_WriteData(y & 0xFF);
    LCD_WriteData((y + h - 1) >> 8);
    LCD_WriteData((y + h - 1) & 0xFF);
    LCD_WriteCmd(0x2C);
}

/* ---------- 3. 绘图 ---------- */

static void FillArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint32_t i, n = (uint32_t)w * h;
    if (w == 0 || h == 0)
    {
        return;
    }
    SetWindow(x, y, w, h);
    for (i = 0; i < n; i++)
    {
        LCD_WriteData(color);
    }
}

static void LCD_ClearAll24(uint16_t color)
{
    FillArea(0, 0, LCD_W, LCD_H, color);
}

/*
 * 圆润心形：使用参数方程轮廓点表 + 扫描线填充
 *   轮廓由参数方程 x = 16sin³t、y = 13cos t - 5cos2t - 2cos3t - cos4t 生成，
 *   轮廓是光滑曲线，顶部双凸、底部收成尖角，没有折角。
 *   r = 心形半宽（像素）；心形以 (cx, cy) 为中心。
 */
static void DrawHeart(int16_t cx, int16_t cy, int16_t r, uint16_t color)
{
    const float yTop = HEART_Y_TOP;
    const float yBot = HEART_Y_BOT;
    int16_t rows = (int16_t)((yTop - yBot) * (float)r);
    int16_t top = (int16_t)((float)cy - (yTop - yBot) * (float)r * 0.5f);

    for (int16_t row = 0; row < rows; row++)
    {
        float yu = yTop - (float)row / (float)r;
        int16_t py = (int16_t)(top + row);
        float xs[6];
        int16_t n = 0;
        int16_t i;

        if (py < 0 || py >= LCD_H)
        {
            continue;
        }

        /* 求轮廓与当前水平线的所有交点 */
        for (i = 0; i < HEART_N; i++)
        {
            int16_t j = (int16_t)((i + 1) % HEART_N);
            float y1 = g_heartY[i];
            float y2 = g_heartY[j];

            if ((y1 <= yu && yu < y2) || (y2 <= yu && yu < y1))
            {
                float t = (yu - y1) / (y2 - y1);
                if (n < 6)
                {
                    xs[n++] = g_heartX[i] + t * (g_heartX[j] - g_heartX[i]);
                }
            }
        }

        /* 交点排序 */
        for (i = 0; i < n; i++)
        {
            for (int16_t k = i + 1; k < n; k++)
            {
                if (xs[k] < xs[i])
                {
                    float tmp = xs[i];
                    xs[i] = xs[k];
                    xs[k] = tmp;
                }
            }
        }

        /* 成对填充（顶部凹陷处会有两段） */
        for (i = 0; i + 1 < n; i += 2)
        {
            int16_t x1 = (int16_t)((float)cx + xs[i] * (float)r);
            int16_t x2 = (int16_t)((float)cx + xs[i + 1] * (float)r);

            if (x2 < x1)
            {
                continue;
            }
            if (x1 < 0)
            {
                x1 = 0;
            }
            if (x2 >= LCD_W)
            {
                x2 = LCD_W - 1;
            }
            FillArea((uint16_t)x1, (uint16_t)py, (uint16_t)(x2 - x1 + 1), 1, color);
        }
    }
}

static void DrawLoveChar(uint16_t x, uint16_t y, uint8_t index,
                         uint16_t fColor, uint16_t bColor)
{
    SetWindow(x, y, CH_SIZE, CH_SIZE);
    for (uint16_t i = 0; i < 128; i++)
    {
        uint8_t tempByte = love_font[index][i];
        for (uint8_t j = 0; j < 8; j++)
        {
            LCD_WriteData((tempByte & 0x01) ? fColor : bColor);
            tempByte >>= 1;
        }
    }
}

static void DrawLoveString(uint16_t x, uint16_t y, uint16_t fColor, uint16_t bColor)
{
    uint8_t i;
    for (i = 0; i < STR_LEN; i++)
    {
        DrawLoveChar((uint16_t)(x + i * CH_SIZE), y, i, fColor, bColor);
    }
}

/* ---------- 4. 主程序 ---------- */

int main(void)
{
    uint8_t round = 0;

    LCD_InitScreen();

    /* 上电自检：全屏红 → 绿 → 蓝 */
    LCD_ClearAll24(RED);
    Delay_ms(700);
    LCD_ClearAll24(GREEN);
    Delay_ms(700);
    LCD_ClearAll24(BLUE);
    Delay_ms(700);
    LCD_ClearAll24(BLACK);

    while (1)
    {
        uint16_t fColor = g_palette[round][0];
        uint16_t bColor = g_palette[round][1];
        uint8_t i, k;
        int16_t off;

        LCD_ClearAll24(bColor);

        /* 1. 爱心跳动 3 次 */
        for (k = 0; k < 3; k++)
        {
            DrawHeart(HEART_CX, HEART_CY, 38, fColor);
            Delay_ms(200);
            DrawHeart(HEART_CX, HEART_CY, 46, bColor);
            DrawHeart(HEART_CX, HEART_CY, 33, fColor);
            Delay_ms(200);
            DrawHeart(HEART_CX, HEART_CY, 46, bColor);
        }
        DrawHeart(HEART_CX, HEART_CY, 38, fColor);

        /* 2. 逐字出现 */
        for (i = 0; i < STR_LEN; i++)
        {
            DrawLoveChar((uint16_t)(TEXT_X0 + i * CH_SIZE), TEXT_Y, i, fColor, bColor);
            Delay_ms(220);
        }
        Delay_ms(1200);

        /* 3. 整句左右滑动两个来回 */
        for (k = 0; k < 2; k++)
        {
            for (off = 0; off <= 60; off += 4)
            {
                FillArea(0, TEXT_Y - 4, LCD_W, TEXT_BAND_H, bColor);
                DrawLoveString((uint16_t)(TEXT_X0 + off), TEXT_Y, fColor, bColor);
                Delay_ms(22);
            }
            for (off = 60; off >= 0; off -= 4)
            {
                FillArea(0, TEXT_Y - 4, LCD_W, TEXT_BAND_H, bColor);
                DrawLoveString((uint16_t)(TEXT_X0 + off), TEXT_Y, fColor, bColor);
                Delay_ms(22);
            }
        }
        Delay_ms(600);

        round++;
        if (round >= PALETTE_N)
        {
            round = 0;
        }
    }
}
