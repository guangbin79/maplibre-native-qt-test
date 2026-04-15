#pragma once

#include "plugin-HXGISServer.h"

class HXGISServer
{
public:
    HXGISServer(const char *url, const char *root_path, const char *cache_path = nullptr);
    ~HXGISServer();

    HXGISServer(const HXGISServer &) = delete;
    HXGISServer &operator=(const HXGISServer &) = delete;

    bool isRunning() const;
    static const char *version();

private:
    plugin_HXGISServer *m_handle = nullptr;
};
