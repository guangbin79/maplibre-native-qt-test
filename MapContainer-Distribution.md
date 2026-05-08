# MapContainer 发布文件清单

本文档列出了将 `MapContainer` 组件发布给客户时需要包含的所有文件。

## 概述

`MapContainer` 是一个基于 QMapLibre SDK 封装的 Qt Widget 地图组件。客户只需要包含 `mapcontainer.h` 并链接相关库即可使用，但编译时需要以下全部源文件。

---

## 必需的头文件 (.h)

| 文件 | 说明 | 客户可见 |
|---|---|---|
| `mapcontainer.h` | **主接口类**，客户直接使用的唯一入口 | ✅ 是 |
| `mapannotation.h` | 标注数据结构 (`MapAnnotation`) | ✅ 是 |
| `maproutesegment.h` | 线路数据结构 (`MapRouteSegment`) | ✅ 是 |
| `locationindicatormanager.h` | 位置指示器管理器（含 `LocationMode` 枚举） | ✅ 是（需引用枚举） |
| `annotationmanager.h` | 标注管理器内部实现 | ❌ 否 |
| `routemanager.h` | 线路管理器内部实现 | ❌ 否 |
| `cameraanimationmath.h` | 动画数学工具 | ❌ 否 |
| `geojsonbuilder.h` | GeoJSON 构建器 | ❌ 否 |
| `routegeojsonbuilder.h` | 线路 GeoJSON 构建器 | ❌ 否 |

**总计：9 个头文件**

---

## 必需的实现文件 (.cpp)

| 文件 | 说明 |
|---|---|
| `mapcontainer.cpp` | MapContainer 主类实现 |
| `annotationmanager.cpp` | 标注管理器实现 |
| `routemanager.cpp` | 线路管理器实现 |
| `locationindicatormanager.cpp` | 位置指示器实现 |
| `geojsonbuilder.cpp` | GeoJSON 构建器实现 |
| `routegeojsonbuilder.cpp` | 线路 GeoJSON 构建器实现 |

**总计：6 个实现文件**

---

## 第三方依赖

客户需要在项目中链接以下库：

### CMake 配置示例

```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(QMapLibre REQUIRED COMPONENTS Widgets)

target_link_libraries(客户目标 PRIVATE
    QMapLibre::Widgets      # MapLibre Native Qt SDK
    Qt6::Widgets
    Qt6::OpenGLWidgets
)
```

### 依赖说明

| 依赖 | 用途 |
|---|---|
| **QMapLibre::Widgets** | MapLibre Native Qt SDK，提供底层地图渲染 |
| **Qt6::Widgets** | Qt Widgets 模块，基础 UI 类 |
| **Qt6::OpenGLWidgets** | OpenGL Widgets 模块，地图渲染后端 |

---

## 客户使用方式

### 1. 包含头文件

```cpp
#include "mapcontainer.h"
```

### 2. 创建地图实例

```cpp
MapContainer::MapConfig config;
config.styleUrl = "https://demotiles.maplibre.org/style.json";
config.defaultCoordinate = QMapLibre::Coordinate(39.9, 116.4);  // 北京
config.defaultZoom = 10.0;

MapContainer *map = new MapContainer(config, parentWidget);
layout->addWidget(map);
```

### 3. 常用操作示例

```cpp
// 设置中心点和缩放
map->setCenter(39.9042, 116.4074);
map->setZoom(12.0);

// 添加标注
QVector<MapAnnotation> annotations = {
    {"poi-001", 39.9042, 116.4074, "天安门", "marker"}
};
QMap<QString, QImage> icons;
icons["marker"] = QImage(":/icons/marker.png");
map->setAnnotations(annotations, icons);

// 添加线路
QVector<MapRouteSegment> segments = {
    {"seg-001", "route-A", {{39.9, 116.4}, {39.92, 116.38}}, QColor("#FF5722"), 4.0, false}
};
map->setRoutes(segments);

// 位置指示器（导航模式）
map->setLocationIcon(QImage(":/icons/location.png"));
map->setLocationMode(LocationIndicatorManager::LocationMode::Fixed);
map->setLocation(39.9050, 116.4080);
map->showLocation();

// 相机动画
map->animateTo(31.23, 121.47, 12.0, 0.0, 45.0, 2000);

// 连接信号
connect(map, &MapContainer::zoomChanged, this, [](double zoom) {
    qDebug() << "Zoom:" << zoom;
});
```

---

## 推荐的目录结构

交付给客户时，建议按以下结构组织：

```
mapcontainer-sdk/
├── include/                    # 头文件（客户需要引用）
│   ├── mapcontainer.h          # 主接口
│   ├── mapannotation.h         # 标注数据结构
│   ├── maproutesegment.h       # 线路数据结构
│   └── locationindicatormanager.h  # 位置模式枚举
├── src/                        # 实现文件（编译需要）
│   ├── mapcontainer.cpp
│   ├── annotationmanager.cpp
│   ├── annotationmanager.h
│   ├── routemanager.cpp
│   ├── routemanager.h
│   ├── locationindicatormanager.cpp
│   ├── locationindicatormanager.h
│   ├── geojsonbuilder.cpp
│   ├── geojsonbuilder.h
│   ├── routegeojsonbuilder.cpp
│   ├── routegeojsonbuilder.h
│   └── cameraanimationmath.h
├── examples/                   # 使用示例
│   └── basic_usage.cpp
├── MapContainer-Distribution.md  # 本文件
└── README.md                   # 快速开始指南
```

---

## 文件汇总

| 类别 | 数量 | 文件 |
|---|---|---|
| 头文件 (.h) | 9 | mapcontainer.h, mapannotation.h, maproutesegment.h, locationindicatormanager.h, annotationmanager.h, routemanager.h, cameraanimationmath.h, geojsonbuilder.h, routegeojsonbuilder.h |
| 实现文件 (.cpp) | 6 | mapcontainer.cpp, annotationmanager.cpp, routemanager.cpp, locationindicatormanager.cpp, geojsonbuilder.cpp, routegeojsonbuilder.cpp |
| **总计** | **15** | |

---

## 注意事项

1. **Qt 版本要求**：Qt 6.6+（Widgets, OpenGLWidgets）
2. **C++ 标准**：C++17 或更高
3. **线程安全**：所有 `MapContainer` 公共接口必须在主线程（UI 线程）中调用
4. **内部类不可直接使用**：`AnnotationManager`、`RouteManager` 等内部实现类不应被客户直接实例化或调用
5. **MapLibre SDK**：客户需要单独获取并配置 QMapLibre SDK

---

*生成日期：2026-05-08*
