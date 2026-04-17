/**
 * @file hxgisserver.h
 * @brief HXGISServer — 本地 GIS 瓦片服务的 C++ RAII 封装
 *
 * HXGISServer 对底层 C 共享库 libplugin-HXGISServer.so 提供面向对象的封装：
 *   - 构造时调用 plugin_HXGISServer_create() 启动本地 HTTP 瓦片服务
 *   - 析构时调用 plugin_HXGISServer_destroy() 自动停止服务并释放资源
 *   - 禁止拷贝/赋值，确保底层 C 句柄的唯一所有权
 *
 * 底层 C API 定义见 plugin-HXGISServer.h，该库提供：
 *   - 本地矢量瓦片服务（MVT 格式）
 *   - MapLibre Style JSON 端点
 *   - 离线地图数据管理
 *
 * 典型用法：
 *   HXGISServer server("127.0.0.1:4943", "/path/to/map_data");
 *   if (!server.isRunning()) { ... }  // 启动失败处理
 */

#pragma once

#include "plugin-HXGISServer.h"

class HXGISServer
{
public:
    /**
     * @brief 启动 GIS 服务
     * @param url       监听地址，格式 "host:port"，如 "127.0.0.1:4943"
     * @param root_path 地图数据根目录，用于存放/查找瓦片缓存和样式文件
     * @param cache_path 可选的独立缓存目录，为 nullptr 时使用 root_path 下的默认位置
     */
    HXGISServer(const char *url, const char *root_path, const char *cache_path = nullptr);

    /**
     * @brief 析构时自动停止服务并释放 C 句柄
     */
    ~HXGISServer();

    // 禁止拷贝和赋值 — 底层 C 句柄不可共享，保证 RAII 所有权唯一
    HXGISServer(const HXGISServer &) = delete;
    HXGISServer &operator=(const HXGISServer &) = delete;

    /**
     * @brief 检查服务是否成功启动
     * @return true 表示 m_handle 非空，服务正在运行
     */
    bool isRunning() const;

    /**
     * @brief 获取 GIS Server 版本号
     * @return 静态字符串，由底层库内部管理生命周期
     */
    static const char *version();

private:
    // 底层 C API 的不透明句柄，由 plugin_HXGISServer_create() 创建
    plugin_HXGISServer *m_handle = nullptr;
};
