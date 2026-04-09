/**
 * @file oled.c
 * @brief OLED12864 SSD1306 驱动（I2C）
 * @author zerobinhi
 * @date 2025-11-03
 *
 * 功能：
 *  - 单点绘制、线段、矩形、填充
 *  - 显示 ASCII 字符 / 字符串
 *  - 显示数字、浮点数
 *  - 显示 16x16 汉字（阴码、逆向、列行式）
 *  - 显示位图（BMP）
 *
 * color 约定：
 *  0 -> 正常显示（黑底白字）
 *  1 -> 反色显示（白底黑字）
 */
#include "oled.h"
#include <math.h>
#include <stdint.h>

/* SSD1306 控制字节（I2C）*/
#define SSD1306_CTRL_CMD 0x00
#define SSD1306_CTRL_DAT 0x40

/* OLED 显存：128列 × 8页，每页8行，共1024字节 */
static uint8_t OLED_GRAM[OLED_WIDTH][8];

/**
 * @brief 发送命令数组到 SSD1306
 */
static HAL_StatusTypeDef _oled_write_cmd(const uint8_t *cmd, size_t len)
{
    uint8_t buf[len + 1];
    buf[0] = SSD1306_CTRL_CMD;
    memcpy(buf + 1, cmd, len);
    return HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, buf, (uint16_t)(len + 1), 50);
}
/**
 * @brief 发送一页数据（128字节）
 */
static HAL_StatusTypeDef _oled_write_page(const uint8_t *data128)
{
    uint8_t buf[129];
    buf[0] = SSD1306_CTRL_DAT;
    memcpy(buf + 1, data128, 128);
    return HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, buf, 129, 50);
}

/**
 * @brief OLED 初始化
 */
HAL_StatusTypeDef OLED_Init()
{
    const uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
        0x40, 0x8D, 0x14, 0x20, 0x02, 0xA1, 0xC8,
        0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB,
        0x40, 0xA4, 0xA6, 0xAF};
    if (_oled_write_cmd(init_cmds, sizeof(init_cmds)) != HAL_OK)
        return HAL_ERROR;
    OLED_Clear(0);
    return OLED_Refresh();
}
/**
 * @brief 刷新显存到 OLED
 */
HAL_StatusTypeDef OLED_Refresh()
{
    uint8_t page_buf[128];

    for (uint8_t page = 0; page < 8; page++)
    {
        uint8_t cmd[3] = {0xB0 | page, 0x00, 0x10};
        if (_oled_write_cmd(cmd, 3) != HAL_OK)
            return HAL_ERROR;
        
        for (uint16_t col = 0; col < 128; col++)
            page_buf[col] = OLED_GRAM[col][page];

        if (_oled_write_page(page_buf) != HAL_OK)
            return HAL_ERROR;
    }
    return HAL_OK;
}
/**
 * @brief 清屏或填充
 */
void OLED_Clear(uint8_t color)
{
    memset(OLED_GRAM, color ? 0xFF : 0x00, sizeof(OLED_GRAM));
}
/**
 * @brief 设置对比度
 */
HAL_StatusTypeDef OLED_SetContrast(uint8_t contrast)
{
    uint8_t cmd[2] = {0x81, contrast};
    return _oled_write_cmd(cmd, 2);
}
/**
 * @brief 全屏反显
 */
HAL_StatusTypeDef OLED_InvertDisplay(bool invert)
{
    uint8_t cmd = invert ? 0xA7 : 0xA6;
    return _oled_write_cmd(&cmd, 1);
}
/**
 * @brief OLED 单点绘制（自动防越界）
 */
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return; // 超出屏幕范围直接忽略

    uint8_t page = y >> 3;
    uint8_t bit = y & 0x07;

    if (color)
        OLED_GRAM[x][page] |= (1 << bit);
    else
        OLED_GRAM[x][page] &= ~(1 << bit);
}

/**
 * @brief  绘制一条直线（使用 Bresenham 算法，适配无符号坐标）
 * @param  (x1, y1): 起点坐标（0~127, 0~63）
 * @param  (x2, y2): 终点坐标（0~127, 0~63）
 * @param  color: 颜色（1=亮，0=灭）
 * @note   坐标参数改为uint16_t，通过无符号数特性处理方向计算，增加坐标合法性检查
 */
void OLED_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color)
{
    // 如果线段完全在屏幕外（右或下），可以直接返回
    if ((x1 >= OLED_WIDTH && x2 >= OLED_WIDTH) ||
        (y1 >= OLED_HEIGHT && y2 >= OLED_HEIGHT) ||
        (x1 < 0 && x2 < 0) ||
        (y1 < 0 && y2 < 0))
        return;

    int16_t dx = abs((int16_t)x2 - (int16_t)x1);
    int16_t dy = abs((int16_t)y2 - (int16_t)y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;

    while (1)
    {
        OLED_DrawPoint(x1, y1, color);
        if (x1 == x2 && y1 == y2)
            break;
        int16_t e2 = err << 1;
        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y1 += sy;
        }
        // 超出范围提前退出（加速）
        if (x1 >= OLED_WIDTH || y1 >= OLED_HEIGHT || x1 < 0 || y1 < 0)
            break;
    }
}
/**
 * @brief  绘制矩形（空心）
 * @param  (x1, y1): 左上角坐标
 * @param  (x2, y2): 右下角坐标
 * @param  color: 颜色（1=亮，0=灭）
 */
void OLED_DrawRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    if (x1 > x2)
    {
        uint8_t t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y1 > y2)
    {
        uint8_t t = y1;
        y1 = y2;
        y2 = t;
    }

    if (x2 >= OLED_WIDTH)
        x2 = OLED_WIDTH - 1;
    if (y2 >= OLED_HEIGHT)
        y2 = OLED_HEIGHT - 1;

    OLED_DrawLine(x1, y1, x2, y1, color);
    OLED_DrawLine(x1, y2, x2, y2, color);
    OLED_DrawLine(x1, y1, x1, y2, color);
    OLED_DrawLine(x2, y1, x2, y2, color);
}
/**
 * @brief  绘制矩形（实心）
 * @param  (x1, y1): 左上角坐标
 * @param  (x2, y2): 右下角坐标
 * @param  color: 颜色（1=亮，0=灭）
 */
void OLED_FillRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    if (x1 > x2)
    {
        uint8_t t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y1 > y2)
    {
        uint8_t t = y1;
        y1 = y2;
        y2 = t;
    }

    if (x2 >= OLED_WIDTH)
        x2 = OLED_WIDTH - 1;
    if (y2 >= OLED_HEIGHT)
        y2 = OLED_HEIGHT - 1;

    for (uint8_t y = y1; y <= y2; y++)
        for (uint8_t x = x1; x <= x2; x++)
            OLED_DrawPoint(x, y, color);
}
/**
 * @brief 在指定位置显示单个 ASCII 字符
 *
 * @param x     起始横坐标（0~OLED_WIDTH-1）
 * @param y     起始纵坐标（0~OLED_HEIGHT-1）
 * @param chr   要显示的 ASCII 字符（范围：' ' ~ '~'）
 * @param size  字体大小（支持：12、16、24、32）
 * @param color 显示模式：0=正常，1=反色
 *
 * @note 若字符或坐标越界，则不执行任何操作。
 */
void OLED_ShowChar(uint8_t x, uint8_t y,
                   char chr, uint8_t size, uint8_t color)
{
    // 参数合法性检查
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;
    if (chr < ' ' || chr > '~')
        return;
    if (color > 1)
        return;

    const uint8_t *font = NULL;
    uint8_t width = 0, height = 0;

    switch (size)
    {
    case 12:
        font = c_chFont1206[chr - ' '];
        width = 6;
        height = 12;
        break;
    case 16:
        font = c_chFont1608[chr - ' '];
        width = 8;
        height = 16;
        break;
    case 24:
        font = c_chFont1612[chr - ' '];
        width = 12;
        height = 16;
        break;
    case 32:
        font = c_chFont3216[chr - ' '];
        width = 16;
        height = 32;
        break;
    default:
        return; // 字体尺寸不支持
    }

    // 防止超出屏幕边界
    if (x + width > OLED_WIDTH || y + height > OLED_HEIGHT)
        return;

    uint8_t pages = (height + 7) / 8;
    for (uint8_t i = 0; i < width; i++)
    {
        for (uint8_t page = 0; page < pages; page++)
        {
            uint8_t data = font[page * width + i];
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                uint8_t py = y + page * 8 + bit;
                if (py >= OLED_HEIGHT)
                    break;
                uint8_t pixel = (data & (1 << bit)) ? 1 : 0;
                if (color)
                    pixel = !pixel;
                OLED_DrawPoint(x + i, py, pixel);
            }
        }
    }
}
/**
 * @brief 在指定位置显示字符串
 *
 * @param x     起始横坐标
 * @param y     起始纵坐标
 * @param str   要显示的字符串（以 '\0' 结尾）
 * @param size  字体大小（支持：12、16、24、32）
 * @param color 显示模式：0=正常，1=反色
 *
 * @note 超出屏幕右边会自动换行；超出屏幕底部会停止显示。
 */
void OLED_ShowString(uint8_t x, uint8_t y,
                     const char *str, uint8_t size, uint8_t color)
{
    if (!str)
        return;
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;

    uint8_t cx = x, cy = y, charWidth = 0;

    switch (size)
    {
    case 12:
        charWidth = 6;
        break;
    case 16:
        charWidth = 8;
        break;
    case 24:
        charWidth = 12;
        break;
    case 32:
        charWidth = 16;
        break;
    default:
        return;
    }

    while (*str)
    {
        if (*str < ' ' || *str > '~')
        {
            str++;
            continue;
        }

        // 自动换行处理
        if (cx + charWidth > OLED_WIDTH)
        {
            cx = 0;
            cy += size;
            if (cy + size > OLED_HEIGHT)
                break;
        }

        OLED_ShowChar(cx, cy, *str, size, color);
        cx += charWidth;
        str++;
    }
}

/**
 * @brief 显示整数（带前导零，可显示负数）
 *
 * @param x     起始横坐标
 * @param y     起始纵坐标
 * @param num   要显示的数字（可负）
 * @param len   固定显示位数（不足前补0，符号不算在内）
 * @param size  字体大小
 * @param color 显示模式：0=正常，1=反色
 */
void OLED_ShowNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT || len == 0 || len > 10)
        return;

    char buf[18]; // 多预留几位，防止溢出
    uint8_t offset = 0;

    // 判断符号
    if (num < 0)
    {
        buf[0] = '-';
        offset = 1;
        num = -num;
    }

    // 生成格式字符串（例如 "%03lu"）
    char fmt[8];
    snprintf(fmt, sizeof(fmt), "%%0%ulu", len);

    snprintf(&buf[offset], sizeof(buf) - offset, fmt, (uint32_t)num);
    OLED_ShowString(x, y, buf, size, color);
}

/**
 * @brief 显示浮点数（带符号与小数位控制），无需printf浮点支持
 *
 * @param x        起始横坐标
 * @param y        起始纵坐标
 * @param num      浮点数（可带正负号）
 * @param int_len  整数部分位数（不包括符号）
 * @param dec_len  小数部分位数
 * @param size     字体大小
 * @param color    显示模式：0=正常，1=反色
 */
void OLED_ShowDecimal(uint8_t x, uint8_t y, float num,
                      uint8_t int_len, uint8_t dec_len,
                      uint8_t size, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;
    if (int_len == 0 || dec_len == 0 || dec_len > 6)
        return;

    char buf[24];
    uint8_t idx = 0;
    bool neg = false;

    if (num < 0.0f)
    {
        neg = true;
        num = -num;
    }

    uint32_t int_part = (uint32_t)num;
    float frac = num - (float)int_part;
    uint32_t frac_part = (uint32_t)(frac * powf(10, dec_len) + 0.5f);

    // 拼整数部分（带前导零）
    char int_buf[12];
    snprintf(int_buf, sizeof(int_buf), "%0*lu", int_len, (unsigned long)int_part);

    // 拼小数部分
    char frac_buf[12];
    snprintf(frac_buf, sizeof(frac_buf), "%0*lu", dec_len, (unsigned long)frac_part);

    // 组合结果
    if (neg)
        idx += snprintf(buf + idx, sizeof(buf) - idx, "-");

    snprintf(buf + idx, sizeof(buf) - idx, "%s.%s", int_buf, frac_buf);

    OLED_ShowString(x, y, buf, size, color);
}

/**
 * @brief 显示 16x16 汉字（阴码）
 *
 * @param x     起始横坐标（0~127）
 * @param y     起始纵坐标（0~63）
 * @param no    汉字在字库数组中的索引
 * @param color 显示模式：0=正常，1=反色
 *
 * @note 字库需为 16x16，列行式存储。
 */
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t no, uint8_t color)
{
    if (x + 16 > OLED_WIDTH || y + 16 > OLED_HEIGHT)
        return;

    const uint8_t *hz_up = Hzk[no * 2];
    const uint8_t *hz_down = Hzk[no * 2 + 1];

    for (uint8_t block = 0; block < 2; block++)
    {
        const uint8_t *dataPtr = (block == 0) ? hz_up : hz_down;
        for (uint8_t col = 0; col < 16; col++)
        {
            uint8_t data = dataPtr[col];
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                uint8_t py = y + block * 8 + bit;
                uint8_t pixel = (data & (1 << bit)) ? 1 : 0;
                if (color)
                    pixel = !pixel;
                OLED_DrawPoint(x + col, py, pixel);
            }
        }
    }
}

/**
 * @brief 显示位图（BMP 格式，列行式）
 *
 * @param x     起始横坐标
 * @param y     起始纵坐标
 * @param bmp   位图数据指针
 * @param w     位图宽度（像素）
 * @param h     位图高度（像素）
 * @param color 显示模式：0=正常，1=反色
 */
void OLED_DrawBitmap(uint8_t x, uint8_t y,
                     const uint8_t *bmp, uint8_t w, uint8_t h, uint8_t color)
{
    if (!bmp)
        return;
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;
    if (x + w > OLED_WIDTH || y + h > OLED_HEIGHT)
        return;

    uint8_t blockCnt = (h + 7) / 8;
    for (uint8_t block = 0; block < blockCnt; block++)
    {
        for (uint8_t col = 0; col < w; col++)
        {
            uint16_t idx = block * w + col;
            uint8_t data = bmp[idx];
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                uint8_t py = y + block * 8 + bit;
                if (py >= OLED_HEIGHT)
                    break;
                uint8_t pixel = (data & (1 << bit)) ? 1 : 0;
                if (color)
                    pixel = !pixel;
                OLED_DrawPoint(x + col, py, pixel);
            }
        }
    }
}