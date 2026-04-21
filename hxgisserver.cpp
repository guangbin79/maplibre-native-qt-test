/**
 * @file hxgisserver.cpp
 * @brief HXGISServer 实现 — 对 C 共享库的薄封装
 *
 * 本文件每个方法都直接转发到 plugin-HXGISServer.h 中的 C 函数，
 * 不添加额外逻辑，仅提供 RAII 生命周期管理和类型安全。
 *
 * @section lifecycle RAII 生命周期详解
 * 构造和析构遵循严格的资源管理顺序：
 *
 * 构造阶段：
 *   1. 成员初始化：m_handle 初始化为 nullptr（C++11 默认成员初始化）
 *   2. C API 调用：plugin_HXGISServer_create() 启动服务
 *   3. 结果存储：成功则保存句柄，失败则保持 nullptr
 *
 * 析构阶段：
 *   1. 空值检查：确保 m_handle 非空才执行释放
 *   2. C API 调用：plugin_HXGISServer_destroy() 停止服务并释放资源
 *   3. 置空处理：m_handle 设为 nullptr，防止悬垂指针
 *
 * @section error_handling 错误处理策略
 * 本封装采用"不抛异常"的错误处理策略：
 *   - 构造失败：通过 isRunning() 返回 false 通知调用方
 *   - 不捕获异常：C API 的错误在内部处理，不向上传播
 *   - 静默失败：析构时即使 C API 报错也继续执行，确保资源不泄漏
 *
 * 这种设计适合嵌入式/游戏场景，避免异常打断主流程。
 */

#include "hxgisserver.h"

/**
 * @brief 构造函数：调用 C API 启动本地瓦片服务
 *
 * 资源获取顺序（RAII 构造阶段）：
 *   1. m_handle 已由默认成员初始化置为 nullptr
 *   2. 调用 plugin_HXGISServer_create() 委托 C 库启动服务
 *   3. C 库内部执行：
 *      - 解析 url，创建 socket 并 bind/listen
 *      - 验证 root_path 存在且可读
 *      - 初始化瓦片缓存管理器
 *      - 启动 HTTP 服务线程
 *   4. 保存返回的句柄（成功）或 nullptr（失败）
 *
 * @note 不抛异常：即使 C 库内部出错，也通过返回 nullptr 静默处理
 * @note 参数直接透传：不对 url/root_path/cache_path 做修改或校验
 */
HXGISServer::HXGISServer(const char *url, const char *root_path, const char *cache_path)
{
    m_handle = plugin_HXGISServer_create(url, root_path, cache_path);
}

/**
 * @brief 析构函数：释放 C 句柄，停止 HTTP 服务
 *
 * 资源释放顺序（RAII 析构阶段）：
 *   1. 检查 m_handle 非空：避免对 nullptr 调用 C API
 *   2. 调用 plugin_HXGISServer_destroy()：
 *      - 停止 HTTP 监听，关闭 socket
 *      - 等待正在处理的请求完成或超时
 *      - 释放瓦片缓存内存
 *      - 销毁服务线程和同步对象
 *   3. m_handle 置 nullptr：标记资源已释放，防止重复释放
 *
 * @note 空值安全：若构造失败（m_handle 为 nullptr），析构不执行任何操作
 * @note 幂等性：即使 C API 内部出错，m_handle 仍会被置空，避免悬垂指针
 * @note 不抛异常：析构函数标记为 noexcept（隐式），确保异常安全
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
 *
 * 实现原理：
 *   - 仅检查 m_handle != nullptr，不涉及 C API 调用
 *   - 轻量级操作，无锁、无系统调用
 *
 * 状态语义：
 *   - true:  构造时 C API 返回有效句柄，服务正在运行
 *   - false: 构造失败（端口被占、路径无效等），或对象已析构
 *
 * @note 线程安全：只读访问 m_handle，可在多线程并发调用
 * @note 非实时性：返回结果反映调用时刻的状态，不保证后续仍有效
 */
bool HXGISServer::isRunning() const
{
    return m_handle != nullptr;
}

/**
 * @brief 获取版本号 — 委托给 C API 静态函数
 *
 * C API 调用：
 *   - plugin_HXGISServer_version() 返回静态字符串指针
 *   - 字符串存储在 C 库的数据段，程序生命周期内有效
 *   - 无需实例，不访问任何对象状态
 *
 * 设计为 static 的原因：
 *   - 版本号是全局属性，与具体实例无关
 *   - 允许在构造前检查版本：if (strcmp(HXGISServer::version(), "1.0") >= 0)
 *   - 符合 C++ 语义：不访问 this 指针的方法应标记为 static
 *
 * @note 线程安全：C 库保证返回的字符串指针始终有效，多线程并发安全
 * @note 内存管理：返回的指针由 C 库静态持有，调用方禁止 free/delete
 */
const char *HXGISServer::version()
{
    return plugin_HXGISServer_version();
}
