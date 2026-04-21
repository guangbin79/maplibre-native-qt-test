#include "scalebarwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <cmath>

/**
 * @file scalebarwidget.cpp
 * @brief 地图比例尺控件实现
 * 
 * 本文件实现了基于Web Mercator投影的比例尺计算和绘制逻辑。
 * 
 * 核心算法说明：
 * 
 * 1. Web Mercator投影米/像素计算
 *    
 *    公式：metersPerPixel = 156543.03392 * cos(latitude) / pow(2, zoom)
 *    
 *    其中156543.03392的来源：
 *    - 地球赤道周长 ≈ 40075 km = 40,075,000 米
 *    - Web Mercator将世界投影到256x256像素的正方形（zoom=0时）
 *    - 赤道处每像素代表的米数 = 40,075,000 / 256 ≈ 156,543.03 米/像素
 *    - 实际公式值156543.03392 = 40,075,017 / 256（更精确的赤道周长）
 *    
 *    cos(latitude)修正原理：
 *    - Web Mercator是等角投影，保持角度但会拉伸距离
 *    - 在纬度φ处，东西方向的距离被放大了 1/cos(φ) 倍
 *    - 因此实际地面距离 = 像素距离 * cos(φ) * 基础比例
 *    - 即：metersPerPixel(φ) = metersPerPixel(赤道) * cos(φ)
 *    
 *    pow(2, zoom)缩放因子：
 *    - zoom=0时，整个世界为256x256像素
 *    - 每增加1级zoom，地图在每个维度上放大2倍
 *    - zoom=n时，分辨率为 256 * 2^n 像素
 *    - 因此米/像素比例需要除以 2^zoom
 * 
 * 2. Nice-Distance算法（人类友好距离选择）
 *    
 *    为什么使用1-2-5序列：
 *    - 人类更容易理解和记忆1、2、5及其10的幂次倍数的数字
 *    - 例如：100m、200m、500m、1km、2km、5km等
 *    - 相比等差或等比序列，1-2-5序列在视觉上更有规律
 *    
 *    序列生成：1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, ...
 *    通项：d = k * 10^n，其中 k ∈ {1, 2, 5}，n为整数
 *    
 *    目标宽度100px的 rationale：
 *    - 100px在 typical UI 中是一个舒适的视觉宽度
 *    - 既不会太小难以阅读，也不会太大占用过多空间
 *    - 便于 mentally 估算其他距离（如"大约两个比例尺那么远"）
 *    
 *    选择逻辑：
 *    - 计算目标距离：targetMeters = metersPerPixel * 100
 *    - 从nice序列中找到第一个 >= targetMeters * 0.4 的值
 *    - 0.4因子确保比例尺不会太小（至少40px宽），保持可读性
 * 
 * 3. 距离格式化
 *    
 *    切换点1km的 rationale：
 *    - 小于1km时，用米表示更直观（如"500m"而非"0.5km"）
 *    - 大于等于1km时，用公里表示更简洁（如"2km"而非"2000m"）
 *    - 1km是常用的距离感知分界点
 */

ScaleBarWidget::ScaleBarWidget(QWidget *parent)
    : QWidget(parent)
    , m_niceDistances({1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000,
                       10000, 20000, 50000, 100000, 200000, 500000, 1000000})
{
    setFixedSize(150, 40);
}

void ScaleBarWidget::updateScale(double latitude, double zoomLevel)
{
    m_latitude = latitude;
    m_zoomLevel = zoomLevel;
    update();
}

void ScaleBarWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // ========== 步骤1：计算Web Mercator投影的米/像素比例 ==========
    // 
    // 156543.03392 = 地球赤道周长(约40075km) / 256像素 (zoom=0时的基础分辨率)
    // cos(latitude * π/180) 将纬度转换为弧度并计算余弦值，用于纬度修正
    // pow(2.0, m_zoomLevel) 计算2的zoom次方，表示当前zoom级别的分辨率倍数
    //
    // 示例计算（latitude=36.75°, zoom=8）：
    // cos(36.75°) ≈ 0.801
    // pow(2, 8) = 256
    // metersPerPixel = 156543.03392 * 0.801 / 256 ≈ 490 米/像素
    const double metersPerPixel = 156543.03392 *
        std::cos(m_latitude * M_PI / 180.0) / std::pow(2.0, m_zoomLevel);

    // ========== 步骤2：选择"nice"距离值 ==========
    //
    // 目标：找一个合适的地面距离，使得对应的像素宽度接近100px
    // 
    // targetMeters：如果比例尺宽度为100px，对应的地面距离
    // 计算公式：targetMeters = metersPerPixel * 100
    //
    // niceDistance选择逻辑：
    // - 遍历m_niceDistances序列（1, 2, 5, 10, 20, 50, ...）
    // - 找到第一个满足 d >= targetMeters * 0.4 的值
    // - 0.4因子确保最终比例尺宽度至少为40px（100px * 0.4），保证可读性
    // - 如果不满足条件的，使用序列最大值（1000000m = 1000km）
    //
    // 示例（metersPerPixel ≈ 490）：
    // targetMeters = 490 * 100 = 49000米 = 49km
    // targetMeters * 0.4 = 19600米
    // 序列中第一个 >= 19600的是20000
    // 因此niceDistance = 20000米 = 20km
    const double targetWidth = 100.0;
    const double targetMeters = metersPerPixel * targetWidth;
    double niceDistance = m_niceDistances.back();
    for (int d : m_niceDistances) {
        if (d >= targetMeters * 0.4) {
            niceDistance = d;
            break;
        }
    }

    // ========== 步骤3：计算比例尺条的像素宽度 ==========
    //
    // 根据选定的niceDistance反推需要的像素宽度
    // 公式：barWidth = niceDistance / metersPerPixel
    //
    // 接上例：barWidth = 20000 / 490 ≈ 40.8px
    // 这是一个合理的、人类友好的显示宽度
    const double barWidth = niceDistance / metersPerPixel;

    // ========== 步骤4：格式化距离文本 ==========
    //
    // 根据距离大小选择合适的单位：
    // - 距离 < 1km（1000m）：显示为"xxx m"，如"500 m"
    // - 距离 >= 1km：显示为"x km"，如"20 km"
    //
    // 这种切换避免了小数，保持显示简洁
    QString distanceText = niceDistance >= 1000
        ? QString("%1 km").arg(static_cast<int>(niceDistance / 1000))
        : QString("%1 m").arg(static_cast<int>(niceDistance));

    // ========== 步骤5：设置绘制参数 ==========
    QFont font = painter.font();
    font.setPixelSize(11);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::black);

    // ========== 步骤6：绘制距离标签（顶部左侧） ==========
    // 文本基线位置，从顶部向下偏移8像素
    const int textY = 8;
    painter.drawText(0, textY, distanceText);

    // ========== 步骤7：绘制比例尺条 ==========
    //
    // 布局参数：
    // - barY：比例尺条的Y坐标（在文本下方4像素）
    // - barH：比例尺条高度4像素（细线风格）
    // - capExtra：端帽向上下延伸的额外长度（各2像素，总共4像素）
    //
    // 绘制顺序：
    // 1. 水平主条（圆角矩形，半径2px）
    // 2. 左端帽（垂直矩形，标记起点）
    // 3. 右端帽（垂直矩形，标记终点）
    const int barY = textY + 4;
    const int barH = 4;
    const int capExtra = 4;

    // 水平主条：黑色填充，无描边，圆角半径2px
    painter.setBrush(Qt::black);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, barY, static_cast<int>(barWidth), barH, 2, 2);

    // 左端帽：2像素宽，高度为barH + capExtra，居中于主条
    // 位置：x=0, y=barY - capExtra/2
    painter.drawRect(0, barY - capExtra / 2, 2, barH + capExtra);

    // 右端帽：同上，位置在barWidth - 2处
    painter.drawRect(static_cast<int>(barWidth) - 2, barY - capExtra / 2, 2, barH + capExtra);

    // ========== 步骤8：绘制缩放级别指示器 ==========
    //
    // 在比例尺条右侧6像素处显示当前zoom级别
    // 格式："Z" + 整数zoom值，如"Z8"
    // 这帮助开发者/用户了解当前地图状态
    painter.setPen(Qt::black);
    QString zoomText = QString("Z%1").arg(static_cast<int>(m_zoomLevel));
    painter.drawText(static_cast<int>(barWidth) + 6, textY, zoomText);
}
