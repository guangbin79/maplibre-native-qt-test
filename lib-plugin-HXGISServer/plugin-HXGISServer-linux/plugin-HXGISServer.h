#ifndef PPT_PLUGIN_HXGISSERVER_H
#define PPT_PLUGIN_HXGISSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct plugin_HXGISServer_ plugin_HXGISServer;

const char * plugin_HXGISServer_version();
plugin_HXGISServer * plugin_HXGISServer_create(const char *http_url, const char *root_path, const char *cache_path);
void plugin_HXGISServer_destroy(plugin_HXGISServer *handler);

#ifdef __cplusplus
}
#endif

#endif //PPT_PLUGIN_HXGISSERVER_H
