#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#ifndef WIN32
#include <dlfcn.h>
#endif

#include "plugin-HXGISServer.h"
#include "hx-plugin.h"

static void test_direct_api(const char *url, const char *root_path, const char *cache_path)
{
    std::cout << "=== 方式1: 直接调用 plugin_HXGISServer_* 接口 ===" << std::endl;

    const char *ver = plugin_HXGISServer_version();
    std::cout << "[版本] " << (ver ? ver : "(null)") << std::endl;

    std::cout << "[创建] url=" << url
              << " root_path=" << root_path
              << " cache_path=" << (cache_path ? cache_path : "(null)")
              << std::endl;

    auto *handle = plugin_HXGISServer_create(url, root_path, cache_path);
    if (!handle) {
        std::cerr << "[错误] plugin_HXGISServer_create 返回 nullptr" << std::endl;
        return;
    }
    std::cout << "[创建] 成功, handle=" << handle << std::endl;

    std::cout << std::endl;
    std::cout << "GIS 服务器已启动，可用 curl 测试以下接口：" << std::endl;
    std::cout << "  瓦片服务:  curl http://" << url << "/gisserver/mbtiles/z/x/y/data.mbtiles" << std::endl;
    std::cout << "  元数据:    curl http://" << url << "/gisserver/mbtiles/metadata/data.mbtiles" << std::endl;
    std::cout << "  Bundle:    curl http://" << url << "/gisserver/bundle/z/x/y/version/data" << std::endl;
    std::cout << "  GeoJSON:   curl http://" << url << "/gisserver/geojsonvt/start?data=xxx" << std::endl;
    std::cout << "  GEDB:      curl http://" << url << "/gisserver/tianmu/z/x/y/version/data" << std::endl;
    std::cout << "  GeoPkg:    curl http://" << url << "/gisserver/geopackage/z/x/y/version/data" << std::endl;
    std::cout << "  Vtzero:    curl http://" << url << "/gisserver/vtzero/create" << std::endl;
    std::cout << "  静态文件:  curl http://" << url << "/style.json" << std::endl;
    std::cout << std::endl;
    std::cout << "按 Enter 键销毁并退出..." << std::endl;

    getchar();

    plugin_HXGISServer_destroy(handle);
    std::cout << "[销毁] 完成" << std::endl;
}

static void test_plugin_api(const char *url, const char *root_path, const char *cache_path,
                            const char *plugin_path, const char *data_path)
{
    std::cout << "=== 方式2: 通过 hx_plugin_* 标准插件接口 ===" << std::endl;

#ifdef WIN32
    _putenv_s("HX_GIS_SERVER_URL", url);
    _putenv_s("HX_GIS_MAP_PATH", root_path);
    if (cache_path) {
        _putenv_s("HX_GIS_MAP_CACHE_PATH", cache_path);
    } else {
        _putenv_s("HX_GIS_MAP_CACHE_PATH", "");
    }
    _putenv_s("HX_NAVI_LOG_PATH", data_path);
#else
    setenv("HX_GIS_SERVER_URL", url, 1);
    setenv("HX_GIS_MAP_PATH", root_path, 1);
    if (cache_path) {
        setenv("HX_GIS_MAP_CACHE_PATH", cache_path, 1);
    } else {
        unsetenv("HX_GIS_MAP_CACHE_PATH");
    }
    setenv("HX_NAVI_LOG_PATH", data_path, 1);
#endif

    const char *ver = plugin_HXGISServer_version();
    std::cout << "[版本] " << (ver ? ver : "(null)") << std::endl;

    void *plugin = hx_plugin_create(plugin_path, data_path);
    if (!plugin) {
        std::cerr << "[错误] hx_plugin_create 返回 nullptr"
                  << " (HX_GIS_SERVER_URL 或 HX_GIS_MAP_PATH 未设置)" << std::endl;
        return;
    }
    std::cout << "[创建] 成功, plugin=" << plugin << std::endl;

    int size = 0;
    void *result = hx_plugin_do(plugin, "test_command", &size);
    std::cout << "[do] command=\"test_command\" result=" << result
              << " size=" << size << " (接口未实现，预期 nullptr)" << std::endl;

    std::cout << std::endl;
    std::cout << "GIS 服务器已启动，按 Enter 键销毁并退出..." << std::endl;
    getchar();

    hx_plugin_destroy(plugin);
    std::cout << "[销毁] 完成" << std::endl;
}

#ifndef WIN32
using fn_version_t        = const char *(*)();
using fn_create_t         = plugin_HXGISServer *(*)(const char *, const char *, const char *);
using fn_destroy_t        = void (*)(plugin_HXGISServer *);
using fn_plugin_create_t  = void *(*)(const char *, const char *);
using fn_plugin_do_t      = void *(*)(void *, const char *, int *);
using fn_plugin_destroy_t = void (*)(void *);

static void test_dlopen(const char *so_path, const char *url, const char *root_path,
                        const char *cache_path)
{
    std::cout << "=== 方式3: dlopen 动态加载 " << so_path << " ===" << std::endl;

    void *dl = dlopen(so_path, RTLD_NOW);
    if (!dl) {
        std::cerr << "[错误] dlopen 失败: " << dlerror() << std::endl;
        return;
    }

    auto fn_version        = (fn_version_t)dlsym(dl, "plugin_HXGISServer_version");
    auto fn_create         = (fn_create_t)dlsym(dl, "plugin_HXGISServer_create");
    auto fn_destroy        = (fn_destroy_t)dlsym(dl, "plugin_HXGISServer_destroy");
    auto fn_plugin_create  = (fn_plugin_create_t)dlsym(dl, "hx_plugin_create");
    auto fn_plugin_do      = (fn_plugin_do_t)dlsym(dl, "hx_plugin_do");
    auto fn_plugin_destroy = (fn_plugin_destroy_t)dlsym(dl, "hx_plugin_destroy");

    std::cout << "[符号] version=" << (void *)fn_version
              << " create=" << (void *)fn_create
              << " destroy=" << (void *)fn_destroy
              << std::endl;
    std::cout << "[符号] plugin_create=" << (void *)fn_plugin_create
              << " plugin_do=" << (void *)fn_plugin_do
              << " plugin_destroy=" << (void *)fn_plugin_destroy
              << std::endl;

    if (!fn_version || !fn_create || !fn_destroy) {
        std::cerr << "[错误] 核心符号未找到" << std::endl;
        dlclose(dl);
        return;
    }

    std::cout << "[版本] " << fn_version() << std::endl;

    auto *handle = fn_create(url, root_path, cache_path);
    if (!handle) {
        std::cerr << "[错误] create 返回 nullptr" << std::endl;
        dlclose(dl);
        return;
    }
    std::cout << "[创建] 成功, handle=" << handle << std::endl;

    std::cout << std::endl;
    std::cout << "GIS 服务器已启动，按 Enter 键销毁并退出..." << std::endl;
    getchar();

    fn_destroy(handle);
    std::cout << "[销毁] 完成" << std::endl;

    dlclose(dl);
}
#endif

static void print_usage(const char *prog)
{
    std::cout << "用法:" << std::endl;
    std::cout << "  " << prog << " <url> <root_path> [cache_path] [mode]" << std::endl;
    std::cout << std::endl;
    std::cout << "参数:" << std::endl;
    std::cout << "  url        HTTP 监听地址，如 0.0.0.0:8080" << std::endl;
    std::cout << "  root_path  地图数据根目录" << std::endl;
    std::cout << "  cache_path 缓存目录（可选，默认为空）" << std::endl;
    std::cout << "  mode       测试模式（可选，默认 1）" << std::endl;
    std::cout << "             1 = 直接链接 API" << std::endl;
    std::cout << "             2 = 标准插件接口（环境变量）" << std::endl;
    std::cout << "             3 = dlopen 动态加载" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << prog << " 0.0.0.0:8080 /data/maps" << std::endl;
    std::cout << "  " << prog << " 0.0.0.0:8080 /data/maps /tmp/cache 2" << std::endl;
    std::cout << "  " << prog << " 0.0.0.0:8080 /data/maps '' 3" << std::endl;
}

int main(int argc, char *argv[])
{
    const char *url        = "0.0.0.0:8080";
    const char *root_path  = "/tmp/hx_gis_test";
    const char *cache_path = nullptr;
    int mode = 1;

    if (argc >= 3) {
        url       = argv[1];
        root_path = argv[2];
    }
    if (argc >= 4) {
        cache_path = argv[3];
        if (strcmp(cache_path, "") == 0) cache_path = nullptr;
    }
    if (argc >= 5) {
        mode = atoi(argv[4]);
    }

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }

    std::cout << "=== plugin-HXGISServer Demo ===" << std::endl;
    std::cout << "url       : " << url << std::endl;
    std::cout << "root_path : " << root_path << std::endl;
    std::cout << "cache_path: " << (cache_path ? cache_path : "(null)") << std::endl;
    std::cout << "mode      : " << mode << std::endl;
    std::cout << std::endl;

    switch (mode) {
        case 1:
            test_direct_api(url, root_path, cache_path);
            break;
        case 2:
            test_plugin_api(url, root_path, cache_path, root_path, root_path);
            break;
        case 3:
#ifndef WIN32
            test_dlopen("./libplugin-HXGISServer.so", url, root_path, cache_path);
#else
            std::cerr << "模式3 (dlopen) 在 Windows 下不可用" << std::endl;
            return 1;
#endif
            break;
        default:
            std::cerr << "未知模式: " << mode << " (可选 1/2/3)" << std::endl;
            return 1;
    }

    return 0;
}
