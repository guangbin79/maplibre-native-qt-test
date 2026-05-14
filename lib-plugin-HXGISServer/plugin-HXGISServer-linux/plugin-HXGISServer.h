#ifndef PPT_PLUGIN_HXGISSERVER_H
#define PPT_PLUGIN_HXGISSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct plugin_HXGISServer_ plugin_HXGISServer;

const char * plugin_HXGISServer_version();

/*
 * Environment variables consumed by the plugin (set before create):
 *   HX_GIS_SERVER_URL  - HTTP listen address  (required)
 *   HX_GIS_DATA_PATH   - GIS data root dir    (required)
 *   HX_GIS_CACHE_PATH  - cache directory       (optional, NULL to disable)
 *   HX_LOG_PATH        - log file directory    (optional)
 */
plugin_HXGISServer * plugin_HXGISServer_create(const char *http_url, const char *root_path, const char *cache_path);
void plugin_HXGISServer_destroy(plugin_HXGISServer *handler);

#ifdef __cplusplus
}
#endif

#endif //PPT_PLUGIN_HXGISSERVER_H
