# MapLibre Qt Widgets 离线地图 Demo 集成指南

## 概述

本文档指导如何使用 `MapContainer`、`HXGISServer`、`ControlPanelWidget`、`ScaleBarWidget` 四个核心组件，构建一个支持离线地图浏览、交互控制和比例尺显示的完整 Demo 程序。

## 组件介绍

### 1. MapContainer — 地图显示容器

**文件**: `mapcontainer.h` / `mapcontainer.cpp`

**功能**:
- 基于 QMapLibre 的跨平台地图渲染
- 支持鼠标拖拽、滚轮缩放
- 支持多点触控手势（单指平移、双指缩放/旋转）
- 提供地图状态管理（中心点、缩放、旋转、倾斜）

**关键 API**:
```cpp
// 配置结构体
struct MapConfig {
    QString styleUrl;                    // 地图样式 URL
    QMapLibre::Coordinate defaultCoordinate;  // 默认中心点 [纬度, 经度]
    double defaultZoom = 10.0;           // 默认缩放级别
};

// 构造函数
explicit MapContainer(const MapConfig &config, QWidget *parent = nullptr);

// 状态控制
void setCenter(double latitude, double longitude);
void setZoom(double zoom);
void setBearing(double bearing);  // 旋转角度
void setPitch(double pitch);      // 倾斜角度
void setStyle(const QString &styleUrl);

// 信号
void zoomChanged(double zoom);
void centerChanged(double latitude, double longitude);
void bearingChanged(double bearing);
void pitchChanged(double pitch);
```

---

### 2. HXGISServer — 本地瓦片服务

**文件**: `hxgisserver.h` / `hxgisserver.cpp`

**功能**:
- 启动本地 HTTP 瓦片服务器
- 提供离线矢量瓦片数据（MVT 格式）
- 提供 MapLibre Style JSON 端点
- RAII 模式自动管理资源生命周期

**关键 API**:
```cpp
// 构造函数 - 启动服务
// address: 监听地址和端口，如 "127.0.0.1:4943"
// rootPath: 地图数据目录路径
// cachePath: 缓存目录路径（可选）
HXGISServer(const QString &address, 
            const QString &rootPath,
            const QString &cachePath = QString());

// 检查服务是否运行正常
bool isRunning() const;

// 获取库版本（静态方法）
static QString version();
```

**本地服务 URL 格式**:
```
http://127.0.0.1:4943/styles/day/style.json     # 日间样式
http://127.0.0.1:4943/styles/night/style.json   # 夜间样式
http://127.0.0.1:4943/tiles/{z}/{x}/{y}.mvt     # 矢量瓦片
```

---

### 3. ControlPanelWidget — 控制面板

**文件**: `controlpanelwidget.h` / `controlpanelwidget.cpp`

**功能**:
- 三个垂直滑块控制地图视角
- 缩放 (Zoom): 0-20
- 旋转 (Bearing): 0°-360°
- 倾斜 (Tilt): 0°-60°
- 双向绑定：滑块控制地图，地图状态反馈到滑块

**关键 API**:
```cpp
// 构造函数
explicit ControlPanelWidget(QWidget *parent = nullptr);

// 信号 - 用户操作滑块时触发
void zoomChanged(double zoom);
void bearingChanged(double bearing);
void tiltChanged(double tilt);

// 槽函数 - 接收地图状态更新
void setZoomValue(double zoom);
void setBearingValue(double bearing);
void setTiltValue(double tilt);
```

---

### 4. ScaleBarWidget — 比例尺

**文件**: `scalebarwidget.h` / `scalebarwidget.cpp`

**功能**:
- 在地图上显示当前缩放级别的距离比例尺
- 基于 Web Mercator 投影计算实际距离
- 使用 "nice-distance" 算法选择人类友好的刻度值
- 自动适应不同缩放级别和纬度

**关键 API**:
```cpp
// 构造函数
explicit ScaleBarWidget(QWidget *parent = nullptr);

// 更新比例尺 - 当地图状态变化时调用
void updateScale(double latitude, double zoomLevel);
```

**比例尺计算原理**:
```
米/像素 = 156543.03392 * cos(纬度) / 2^zoom

其中 156543.03392 = 赤道周长(40075km) / 360° / 256像素
```

---

## 完整 Demo 实现

### 项目结构

```
offline-map-demo/
├── CMakeLists.txt
├── main.cpp
├── mainwindow.h
├── mainwindow.cpp
├── mapcontainer.h / .cpp          # 地图容器
├── hxgisserver.h / .cpp           # 本地服务
├── controlpanelwidget.h / .cpp    # 控制面板
├── scalebarwidget.h / .cpp        # 比例尺
└── map_data/                      # 离线地图数据目录
    ├── styles/
    │   ├── day/
    │   │   └── style.json
    │   └── night/
    │       └── style.json
    └── tiles/                     # MVT 瓦片数据
```

---

### 1. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(OfflineMapDemo VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# 配置 MapLibre SDK 路径
set(MAPLIBRE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native-qt_v3.0.0_Qt6.6.3_Linux")
list(APPEND CMAKE_PREFIX_PATH "${MAPLIBRE_SDK_DIR}/lib64/cmake/QMapLibre")

# 查找依赖
find_package(QMapLibre REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# 源文件
set(SOURCES
    main.cpp
    mainwindow.cpp
    mapcontainer.cpp
    hxgisserver.cpp
    controlpanelwidget.cpp
    scalebarwidget.cpp
)

set(HEADERS
    mainwindow.h
    mapcontainer.h
    hxgisserver.h
    controlpanelwidget.h
    scalebarwidget.h
)

# 构建目标
qt_add_executable(offline-map-demo
    ${SOURCES}
    ${HEADERS}
)

# 链接库
target_link_libraries(offline-map-demo PRIVATE
    QMapLibre::Widgets
    Qt6::Widgets
)

# 复制地图数据到构建目录
add_custom_command(TARGET offline-map-demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/map_data
    $<TARGET_FILE_DIR:offline-map-demo>/map_data
)
```

---

### 2. mainwindow.h

```cpp
#pragma once
#include <QMainWindow>

class MapContainer;
class HXGISServer;
class ControlPanelWidget;
class ScaleBarWidget;

/**
 * @brief 主窗口 - 整合所有组件的离线地图浏览器
 * 
 * 组件布局：
 * - 中央：MapContainer（地图显示）
 * - 右侧：ControlPanelWidget（控制面板）
 * - 左下角：ScaleBarWidget（比例尺，叠加在地图上）
 * - 后台：HXGISServer（本地瓦片服务）
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onMapZoomChanged(double zoom);
    void onMapBearingChanged(double bearing);
    void onMapPitchChanged(double pitch);

private:
    void setupUI();
    void setupConnections();
    bool startLocalServer();

    MapContainer *m_mapContainer = nullptr;
    HXGISServer *m_gisServer = nullptr;
    ControlPanelWidget *m_controlPanel = nullptr;
    ScaleBarWidget *m_scaleBar = nullptr;
};
```

---

### 3. mainwindow.cpp

```cpp
#include "mainwindow.h"
#include "mapcontainer.h"
#include "hxgisserver.h"
#include "controlpanelwidget.h"
#include "scalebarwidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QDebug>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 1. 启动本地瓦片服务
    if (!startLocalServer()) {
        QMessageBox::critical(this, "错误", "无法启动本地地图服务");
        return;
    }

    // 2. 设置 UI
    setupUI();

    // 3. 连接信号槽
    setupConnections();
}

MainWindow::~MainWindow() = default;

bool MainWindow::startLocalServer()
{
    // 获取地图数据目录路径
    QString mapDataPath = QDir::currentPath() + "/map_data";
    
    // 创建 HXGISServer 实例（RAII 自动管理）
    m_gisServer = new HXGISServer("127.0.0.1:4943", mapDataPath);
    
    if (!m_gisServer->isRunning()) {
        qCritical() << "本地服务启动失败，请检查：";
        qCritical() << "  - 端口 4943 是否被占用";
        qCritical() << "  - 地图数据目录是否存在:" << mapDataPath;
        return false;
    }
    
    qDebug() << "本地地图服务已启动：http://127.0.0.1:4943";
    qDebug() << "服务版本：" << HXGISServer::version();
    return true;
}

void MainWindow::setupUI()
{
    // 创建中央控件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 主布局：水平布局（地图 + 控制面板）
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ===== 左侧：地图容器 =====
    MapContainer::MapConfig config;
    // 使用本地服务样式
    config.styleUrl = "http://127.0.0.1:4943/styles/day/style.json";
    // 默认显示北京
    config.defaultCoordinate = QMapLibre::Coordinate(39.9, 116.4);
    config.defaultZoom = 10.0;

    m_mapContainer = new MapContainer(config, centralWidget);
    mainLayout->addWidget(m_mapContainer, 1);  // stretch = 1，占据剩余空间

    // ===== 右侧：控制面板 =====
    m_controlPanel = new ControlPanelWidget(centralWidget);
    m_controlPanel->setFixedWidth(80);  // 固定宽度
    m_controlPanel->setMaximumHeight(400);
    mainLayout->addWidget(m_controlPanel, 0);

    // ===== 比例尺（叠加在地图上）=====
    m_scaleBar = new ScaleBarWidget(m_mapContainer);
    m_scaleBar->setGeometry(16, height() - 56, 150, 40);
    m_scaleBar->updateScale(39.9, 10.0);  // 初始比例尺

    // 窗口设置
    setWindowTitle("离线地图浏览器 - MapLibre Qt Demo");
    resize(1024, 768);
}

void MainWindow::setupConnections()
{
    // ===== 控制面板 → 地图 =====
    // 用户拖动滑块时更新地图
    connect(m_controlPanel, &ControlPanelWidget::zoomChanged,
            m_mapContainer, &MapContainer::setZoom);
    
    connect(m_controlPanel, &ControlPanelWidget::bearingChanged,
            m_mapContainer, &MapContainer::setBearing);
    
    connect(m_controlPanel, &ControlPanelWidget::tiltChanged,
            m_mapContainer, &MapContainer::setPitch);

    // ===== 地图 → 控制面板 =====
    // 地图状态变化时更新滑块（双向绑定）
    connect(m_mapContainer, &MapContainer::zoomChanged,
            this, &MainWindow::onMapZoomChanged);
    
    connect(m_mapContainer, &MapContainer::bearingChanged,
            this, &MainWindow::onMapBearingChanged);
    
    connect(m_mapContainer, &MapContainer::pitchChanged,
            this, &MainWindow::onMapPitchChanged);

    // ===== 地图 → 比例尺 =====
    // 缩放级别变化时更新比例尺
    connect(m_mapContainer, &MapContainer::zoomChanged,
            [this](double zoom) {
                // 获取当前中心点纬度（简化处理，使用默认值）
                double latitude = 39.9;  // 实际应从地图获取
                m_scaleBar->updateScale(latitude, zoom);
            });
}

void MainWindow::onMapZoomChanged(double zoom)
{
    m_controlPanel->setZoomValue(zoom);
}

void MainWindow::onMapBearingChanged(double bearing)
{
    m_controlPanel->setBearingValue(bearing);
}

void MainWindow::onMapPitchChanged(double pitch)
{
    m_controlPanel->setTiltValue(pitch);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // 比例尺保持在左下角
    if (m_scaleBar) {
        m_scaleBar->setGeometry(16, height() - 56, 150, 40);
    }
}
```

---

### 4. main.cpp

```cpp
#include "mainwindow.h"
#include <QApplication>
#include <QDir>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用信息
    app.setApplicationName("OfflineMapDemo");
    app.setOrganizationName("YourOrganization");

    // Linux 平台：设置 Qt 插件路径（MapLibre 需要）
#ifdef Q_OS_LINUX
    QString pluginPath = QDir::currentPath() + 
        "/maplibre-native-qt_v3.0.0_Qt6.6.3_Linux/plugins";
    QCoreApplication::addLibraryPath(pluginPath);
    qDebug() << "Qt Plugin Path:" << pluginPath;
#endif

    // 检查地图数据目录
    QString mapDataPath = QDir::currentPath() + "/map_data";
    if (!QDir(mapDataPath).exists()) {
        qWarning() << "警告：地图数据目录不存在:" << mapDataPath;
        qWarning() << "请确保 map_data/ 目录包含 styles/ 和 tiles/ 子目录";
    }

    // 创建并显示主窗口
    MainWindow window;
    window.show();

    return app.exec();
}
```

---

## 数据准备

### 地图数据目录结构

```
map_data/
├── styles/
│   ├── day/
│   │   └── style.json          # 日间地图样式
│   └── night/
│       └── style.json          # 夜间地图样式
└── tiles/
    ├── 0/
    │   ├── 0/
    │   │   └── 0.mvt           # 0/0/0 瓦片
    │   └── 1/
    │       └── 0.mvt           # 0/1/0 瓦片
    ├── 1/
    │   └── ...
    └── ...
```

### style.json 示例

```json
{
  "version": 8,
  "name": "Day Style",
  "sources": {
    "local-tiles": {
      "type": "vector",
      "tiles": ["http://127.0.0.1:4943/tiles/{z}/{x}/{y}.mvt"],
      "minzoom": 0,
      "maxzoom": 14
    }
  },
  "layers": [
    {
      "id": "background",
      "type": "background",
      "paint": {
        "background-color": "#f8f4f0"
      }
    },
    {
      "id": "water",
      "type": "fill",
      "source": "local-tiles",
      "source-layer": "water",
      "paint": {
        "fill-color": "#a0c8f0"
      }
    },
    {
      "id": "roads",
      "type": "line",
      "source": "local-tiles",
      "source-layer": "roads",
      "paint": {
        "line-color": "#ffffff",
        "line-width": 2
      }
    }
  ]
}
```

---

## 构建与运行

### Linux (x86_64)

```bash
# 1. 配置
cmake -B build \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/6.6.3/gcc_64;${PWD}/maplibre-native-qt_v3.0.0_Qt6.6.3_Linux"

# 2. 构建
cmake --build build -j$(nproc)

# 3. 准备数据
mkdir -p build/map_data
# 复制你的地图数据到 build/map_data/

# 4. 运行
cd build
export QT_PLUGIN_PATH="${PWD}/../maplibre-native-qt_v3.0.0_Qt6.6.3_Linux/plugins:$QT_PLUGIN_PATH"
./offline-map-demo
```

### Android (arm64-v8a)

```bash
# 1. 配置 Android 构建
cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/6.6.3/android_arm64_v8a"

# 2. 构建
cmake --build build-android

# 3. 部署到设备
adb install build-android/android-build/OfflineMapDemo.apk
```

---

## 常见问题

### Q: 地图显示空白？

**检查清单**:
1. HXGISServer 是否成功启动？检查 `isRunning()` 返回值
2. 样式 URL 是否正确？尝试在浏览器访问 `http://127.0.0.1:4943/styles/day/style.json`
3. 地图数据目录是否存在？检查 `map_data/` 路径
4. Qt 插件路径是否设置正确？（Linux 平台）

### Q: 控制面板滑块不响应？

**检查信号连接**:
```cpp
// 确保双向绑定正确
connect(controlPanel, &ControlPanelWidget::zoomChanged, 
        mapContainer, &MapContainer::setZoom);
connect(mapContainer, &MapContainer::zoomChanged, 
        controlPanel, &ControlPanelWidget::setZoomValue);
```

### Q: 比例尺显示不正确？

**检查纬度值**：比例尺计算需要当前地图中心纬度。确保调用 `updateScale(latitude, zoom)` 时传入正确的纬度值。

### Q: 触控手势不工作？

**检查**:
1. 确保运行在有触控支持的设备上
2. MapContainer 已启用触控事件处理（内部已实现）
3. 桌面设备上触控事件会映射为鼠标事件

---

## 扩展功能

### 添加样式切换

```cpp
// 在 MainWindow 中添加样式切换按钮
void MainWindow::switchStyle(bool nightMode) {
    QString styleUrl = nightMode 
        ? "http://127.0.0.1:4943/styles/night/style.json"
        : "http://127.0.0.1:4943/styles/day/style.json";
    m_mapContainer->setStyle(styleUrl);
}
```

### 添加位置标记

```cpp
// 在地图中心添加标记（需要 QMapLibre::Map 的 Annotation API）
void MainWindow::addMarker(double lat, double lon) {
    auto *map = m_mapContainer->map();
    QMapLibre::SymbolAnnotation annotation;
    annotation.geometry = QMapLibre::Coordinate(lat, lon);
    annotation.icon = "marker-icon";
    map->addAnnotation(annotation);
}
```

### 保存/恢复地图状态

```cpp
// 保存状态
void MainWindow::saveState() {
    QSettings settings;
    settings.setValue("map/center", QVariant::fromValue(m_mapContainer->center()));
    settings.setValue("map/zoom", m_mapContainer->zoom());
    settings.setValue("map/bearing", m_mapContainer->bearing());
    settings.setValue("map/pitch", m_mapContainer->pitch());
}

// 恢复状态
void MainWindow::restoreState() {
    QSettings settings;
    auto center = settings.value("map/center").value<QMapLibre::Coordinate>();
    m_mapContainer->setCenter(center.first, center.second);
    m_mapContainer->setZoom(settings.value("map/zoom").toDouble());
}
```

---

## 参考

- [MapLibre Native Qt 文档](https://maplibre.org/maplibre-native-qt/docs/)
- [MapLibre Style 规范](https://maplibre.org/maplibre-style-spec/)
- [Web Mercator 投影](https://en.wikipedia.org/wiki/Web_Mercator_projection)
