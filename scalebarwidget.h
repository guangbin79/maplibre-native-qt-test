#pragma once
#include <QWidget>
#include <vector>

/**
 * @file scalebarwidget.h
 * @brief 地图比例尺控件
 * 
 * 本控件用于在地图上显示当前缩放级别下的距离比例尺。
 * 
 * 核心原理：
 * - 基于Web Mercator投影计算当前纬度和缩放级别下的米/像素比例
 * - 使用"nice-distance"算法选择人类友好的距离间隔（1-2-5序列）
 * - 绘制水平比例尺条，显示对应的实际地面距离
 * 
 * 比例尺计算考虑因素：
 * 1. 纬度修正：Web Mercator投影在极地有畸变，需用cos(latitude)修正
 * 2. 缩放级别：每增加一级zoom，像素分辨率翻倍
 * 3. 视觉优化：目标宽度约100px，选择最接近的友好距离值
 */

class ScaleBarWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父控件指针
     */
    explicit ScaleBarWidget(QWidget *parent = nullptr);

public slots:
    /**
     * @brief 更新比例尺显示
     * 
     * 根据当前地图中心纬度和缩放级别重新计算并更新比例尺。
     * 
     * @param latitude 地图中心纬度（度），范围[-90, 90]
     *               用于Web Mercator投影的纬度修正计算
     * @param zoomLevel 地图缩放级别，通常范围[0, 20]
     *                  0级时整个世界地图为256x256像素
     *                  每增加1级，像素分辨率翻倍
     */
    void updateScale(double latitude, double zoomLevel);

protected:
    /**
     * @brief 绘制事件处理
     * 
     * 绘制流程：
     * 1. 计算Web Mercator投影下的米/像素比例
     *    - 公式：156543.03392 * cos(lat) / 2^zoom
     *    - 156543.03392 = 赤道周长(40075km) / 360度 / 256像素
     * 
     * 2. 选择"nice"距离值
     *    - 目标宽度100px，计算对应的地面距离
     *    - 从1-2-5序列中选择不小于目标距离40%的最小值
     * 
     * 3. 格式化距离文本
     *    - < 1km：显示为"xxx m"
     *    - >= 1km：显示为"x km"
     * 
     * 4. 绘制比例尺图形
     *    - 顶部：距离数值标签
     *    - 中部：水平比例尺条（圆角矩形）
     *    - 两端：垂直端帽标记
     *    - 右侧：当前缩放级别指示器
     * 
     * @param event 绘制事件对象（未使用但需保留参数）
     */
    void paintEvent(QPaintEvent *event) override;

private:
    double m_latitude = 36.75;    ///< 当前纬度（度），默认36.75°
    double m_zoomLevel = 8.0;     ///< 当前缩放级别，默认8级
    std::vector<int> m_niceDistances;  ///< 友好距离序列（1-2-5序列，单位：米）
};
