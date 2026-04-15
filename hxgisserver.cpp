#include "hxgisserver.h"

HXGISServer::HXGISServer(const char *url, const char *root_path, const char *cache_path)
{
    m_handle = plugin_HXGISServer_create(url, root_path, cache_path);
}

HXGISServer::~HXGISServer()
{
    if (m_handle) {
        plugin_HXGISServer_destroy(m_handle);
        m_handle = nullptr;
    }
}

bool HXGISServer::isRunning() const
{
    return m_handle != nullptr;
}

const char *HXGISServer::version()
{
    return plugin_HXGISServer_version();
}
