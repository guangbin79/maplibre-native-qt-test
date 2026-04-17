/**
 * @file hxgisserver.cpp
 * @brief HXGISServer 实现 — 对 C 共享库的薄封装
 *
 * 本文件每个方法都直接转发到 plugin-HXGISServer.h 中的 C 函数，
 * 不添加额外逻辑，仅提供 RAII 生命周期管理和类型安全。
 */

#include "hxgisserver.h"

/**
 * @brief 构造函数：调用 C API 启动本地瓦片服务
 *
 * 底层 plugin_HXGISServer_create() 会：
 *   1. 在指定 url 上启动 HTTP 监听
 *   2. 以 root_path 为根目录初始化瓦片数据管理
 *   3. 返回不透明句柄（失败时返回 nullptr）
 */
HXGISServer::HXGISServer(const char *url, const char *root_path, const char *cache_path)
{
    m_handle = plugin_HXGISServer_create(url, root_path, cache_path);
}

/**
 * @brief 析构函数：释放 C 句柄，停止 HTTP 服务
 *
 * plugin_HXGISServer_destroy() 会关闭监听端口并清理资源。
 * 析构后 m_handle 置 nullptr，防止悬垂指针。
 */
HXGISServer::~HXGISServer()
{
    if (m_handle) {
        plugin_HXGISServer_destroy(m_handle);
        m_handle = nullptr;
    }
}

/**
 * @brief 服务状态检查 — 句柄非空即视为运行中
 */
bool HXGISServer::isRunning() const
{
    return m_handle != nullptr;
}

/**
 * @brief 版本号由底层库静态返回，不依赖实例状态，因此为 static 方法
 */
const char *HXGISServer::version()
{
    return plugin_HXGISServer_version();
}
