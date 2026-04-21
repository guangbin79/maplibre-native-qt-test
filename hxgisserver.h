/**
 * @file hxgisserver.h
 * @brief HXGISServer — 本地 GIS 瓦片服务的 C++ RAII 封装
 *
 * @section overview 类概述
 * HXGISServer 对底层 C 共享库 libplugin-HXGISServer.so 提供面向对象的封装，
 * 采用 RAII（Resource Acquisition Is Initialization）模式管理底层 C 资源：
 *   - 构造时调用 plugin_HXGISServer_create() 启动本地 HTTP 瓦片服务
 *   - 析构时调用 plugin_HXGISServer_destroy() 自动停止服务并释放资源
 *   - 禁止拷贝/赋值，确保底层 C 句柄的唯一所有权
 *
 * @section raii RAII 模式说明
 * 本类遵循 RAII 原则：
 *   - 资源获取（服务启动）与对象构造绑定
 *   - 资源释放（服务停止）与对象析构绑定
 *   - 异常安全：即使构造后发生异常，析构函数仍会正确释放已分配的资源
 *   - 所有权语义：m_handle 为独占所有权，不可共享或转移
 *
 * 底层 C API 定义见 plugin-HXGISServer.h，该库提供：
 *   - 本地矢量瓦片服务（MVT 格式）
 *   - MapLibre Style JSON 端点
 *   - 离线地图数据管理
 *
 * @section usage 典型用法
 * @code{.cpp}
 *   // 基本用法：启动服务并检查状态
 *   HXGISServer server("127.0.0.1:4943", "/path/to/map_data");
 *   if (!server.isRunning()) {
 *       std::cerr << "服务启动失败" << std::endl;
 *       return -1;
 *   }
 *
 *   // 使用独立缓存目录
 *   HXGISServer server2("0.0.0.0:8080", "/data/maps", "/tmp/cache");
 *
 *   // 获取版本号（无需实例）
 *   std::cout << HXGISServer::version() << std::endl;
 * @endcode
 *
 * @section errors 常见错误场景
 * | 错误类型 | 原因 | 表现 |
 * |---------|------|------|
 * | 端口被占用 | 指定 port 已被其他进程使用 | 构造后 isRunning() 返回 false |
 * | 权限不足 | 低端口（<1024）需要 root 权限 | 构造后 isRunning() 返回 false |
 * | 路径不存在 | root_path 或 cache_path 无效 | 构造后 isRunning() 返回 false |
 * | 库加载失败 | libplugin-HXGISServer.so 缺失或损坏 | 程序启动时动态链接错误 |
 *
 * @section thread_safety 线程安全
 * - 构造/析构：非线程安全，不得在多个线程同时构造或析构同一实例
 * - isRunning()：只读操作，可在多线程并发调用
 * - version()：线程安全，返回静态字符串
 * - 底层 C 库的线程安全策略请参考 plugin-HXGISServer.h 文档
 */

#pragma once

#include "plugin-HXGISServer.h"

class HXGISServer
{
public:
    /**
     * @brief 构造并启动 GIS 服务
     * @param url        监听地址，格式 "host:port"
     *                   - host: 绑定 IP，如 "127.0.0.1"（仅本地）或 "0.0.0.0"（所有接口）
     *                   - port: 监听端口，如 4943；注意低端口（<1024）需要管理员权限
     * @param root_path  地图数据根目录，用于存放/查找瓦片缓存和样式文件
     *                   - 必须存在且可读
     *                   - 建议绝对路径，避免工作目录变化导致路径失效
     * @param cache_path 可选的独立缓存目录
     *                   - 为 nullptr 时使用 root_path 下的默认缓存位置
     *                   - 用于分离数据目录和缓存目录，便于清理和管理
     *
     * @note 构造不抛异常，启动失败通过 isRunning() 检查
     * @note url 格式错误、端口被占用、路径不存在均会导致启动失败
     */
    HXGISServer(const char *url, const char *root_path, const char *cache_path = nullptr);

    /**
     * @brief 析构时自动停止服务并释放 C 句柄
     *
     * 调用 plugin_HXGISServer_destroy() 完成以下操作：
     *   - 关闭 HTTP 监听端口
     *   - 释放瓦片缓存和内存资源
     *   - 清理底层线程和连接
     *
     * @note 若构造时启动失败（m_handle 为 nullptr），析构不执行任何操作
     * @note 析构后对象不可再用，但可安全重新构造新实例
     */
    ~HXGISServer();

    // 禁止拷贝和赋值 — 底层 C 句柄不可共享，保证 RAII 所有权唯一
    HXGISServer(const HXGISServer &) = delete;
    HXGISServer &operator=(const HXGISServer &) = delete;

    /**
     * @brief 检查服务是否成功启动
     * @return true  服务正在运行，m_handle 非空，HTTP 端口已监听
     * @return false 启动失败，m_handle 为 nullptr
     *
     * @section failure_scenarios 启动失败常见原因
     * - 端口已被其他进程占用（如另一个 HXGISServer 实例或系统服务）
     * - 绑定地址无效（如格式错误、主机名无法解析）
     * - 权限不足（绑定低端口 <1024 需要 root/Administrator）
     * - root_path 不存在或不可读
     * - 缓存目录创建失败（磁盘满或权限不足）
     * - 底层 C 库内部初始化错误（内存分配失败等）
     *
     * @note 应在构造后立即调用检查
     * @note 线程安全：只读操作，可多线程并发调用
     */
    bool isRunning() const;

    /**
     * @brief 获取 GIS Server 版本号
     * @return 版本字符串，格式如 "1.2.3"
     *
     * @note 静态方法，无需实例即可调用：HXGISServer::version()
     * @note 返回的字符串生命周期由底层库静态管理，调用方无需释放
     * @note 即使未构造实例也可调用，用于启动前版本检查
     * @note 线程安全：可并发调用
     */
    static const char *version();

private:
    /**
     * @brief 底层 C API 的不透明句柄
     *
     * 由 plugin_HXGISServer_create() 创建，plugin_HXGISServer_destroy() 销毁。
     * 遵循独占所有权语义：
     *   - 每个 HXGISServer 实例拥有唯一的 m_handle
     *   - 禁止拷贝/赋值防止句柄重复释放
     *   - 构造失败时为 nullptr，isRunning() 据此判断状态
     */
    plugin_HXGISServer *m_handle = nullptr;
};
