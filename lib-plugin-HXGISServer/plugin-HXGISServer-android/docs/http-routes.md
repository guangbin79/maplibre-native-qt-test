# HTTP 路由文档

> 插件内嵌 mongoose HTTP 服务器，监听地址由环境变量 `HX_GIS_SERVER_URL` 指定。
> 源码位置: `plugin-HXGISServer/src/plugin-HXGISServer.cpp` → `http_server_callback()`

## 请求处理流程

路由按以下顺序依次匹配（首次命中即处理，不再继续）：

```
HTTP Request
  │
  ├─ 1. /gisserver/mapserv              ──→ WMS 地图服务
  ├─ 2. /gisserver/tianmu/*/*/*/*/*.*    ──→ 天目栅格瓦片（GEDB）
  ├─ 3. /gisserver/geopackage/*/*/*/*/*.* ──→ GeoPackage 矢量查询
  ├─ 4. /gisserver/bundle/*/*/*/*/*.*    ──→ Bundle 打包瓦片
  ├─ 5. /gisserver/mbtiles/metadata/*.* ──→ MBTiles 元数据
  ├─ 6. /gisserver/mbtiles/*/*/*/*.*    ──→ MBTiles 瓦片
  ├─ 7.  /gisserver/vtzero/create       ──→ 内存点集：创建
  ├─ 8.  /gisserver/vtzero/restore/*/*  ──→ 内存点集：插入
  ├─ 9.  /gisserver/vtzero/clear        ──→ 内存点集：清空
  ├─ 10. /gisserver/vtzero/*/*/*        ──→ 内存点集：瓦片查询
  ├─ 11. /gisserver/geojsonvt/*/*/*/.*mvt ──→ GeoJSON 矢量瓦片
  ├─ 12. /gisserver/geojsonvt/start     ──→ GeoJSON：启动
  ├─ 13. /gisserver/geojsonvt/stop      ──→ GeoJSON：停止
  ├─ 14. ?schema=<protocol>             ──→ 协议替换
  └─ 15. 其他                            ──→ 静态文件服务（兜底）
```

> **路径匹配规则**: 路由 2-13 使用 `std::regex` 正则匹配；路由 1 使用 `mg_http_match_uri` 精确匹配；
> 路由 14 检查 query string 中是否存在 `schema` 参数；路由 15 为静态文件兜底。

---

## GIS 数据路由

### 1. WMS 地图服务

```
GET /gisserver/mapserv?maptype=<type>&mapfile=<file>
```

| 参数 | 位置 | 必需 | 说明 |
|------|------|------|------|
| maptype | query | 是 | WMS 服务类型（WMS/WFS 等），作为 `SERVICE` 参数 |
| mapfile | query | 是 | mapfile 名称，拼接为 `{root_path}/{mapfile}.map`（若不以 `/` 开头）或 `{root_path}{mapfile}.map`（若以 `/` 开头） |

**处理逻辑**: 基于 MapServer CGI，构造标准 WMS QUERY_STRING 后调用 `mapserv_main()` 渲染地图图像。固定参数：`VERSION=1.3.0`, `REQUEST=GetMAP`, `BBOX=-180,-90,180,90`, `CRS=CRS:84`, `width=600`, `height=300`。

**响应**: 通过 stdout 重定向到连接 fd 输出渲染结果。

**错误**: 缺少参数时返回 `200` + `mapserver error!`。

---

### 2. 天目栅格瓦片（GEDB）

```
GET /gisserver/tianmu/{z}/{x}/{y}/{version}/{data}
```

| 段 | 说明 |
|----|------|
| z | 缩放级别（整数） |
| x | 列号（整数） |
| y | 行号（整数） |
| version | 版本标识（字符串） |
| data | 数据路径（相对于 `HX_GIS_MAP_PATH`，可含子目录和文件名，如 `dir/Imagedb/1`） |

**正则**: `^/gisserver/tianmu/*/*/*/*/*.*`

**处理逻辑**:
1. 拼接完整路径: `{root_path}/{data}/`（若 URI 不以 `/` 结尾则追加 `/`）
2. 调用 `hx_gedb::hx_datasource_gedb_LoadImageData(x, y, z, version, db_path, byteArray, count)`

**响应**:
- `200` + 二进制瓦片数据（Content-Type 自动检测：png/jpg/bmp）
- `200` + `tianmuset get tiles none!`（无数据时）

---

### 3. GeoPackage 矢量查询

```
GET /gisserver/geopackage/{z}/{x}/{y}/{version}/{data}
```

| 段 | 说明 |
|----|------|
| z | 查询参数1（整数，除以 10000 后作为经度） |
| x | 查询参数2（整数，除以 10000 后作为纬度） |
| y | 查询参数3（整数，除以 10000 后作为经度） |
| version | 查询参数4（整数，除以 10000 后作为纬度） |
| data | GeoPackage 文件路径（相对于 `HX_GIS_MAP_PATH`） |

> **注意**: 源码注释为 `minX/minY/maxX/maxY`，但变量名实际为 z/x/y/version。
> 四个参数均除以 10000.0 后传入 `QueryFeaturesByRect(db_path, z/10000, x/10000, y/10000, version/10000)`。

**正则**: `^/gisserver/geopackage/*/*/*/*/*.*`

**处理逻辑**:
1. 拼接完整路径: `{root_path}/{data}`（若 URI 以 `/` 结尾则去掉末尾 `/`）
2. 调用 `m_geoPackageReader->QueryFeaturesByRect(db_path, zz/10000, xx/10000, yy/10000, vv/10000)`

**响应**: `200` + JSON（`Content-Type: application/json; charset=utf-8`）

---

### 4. Bundle 打包瓦片

```
GET /gisserver/bundle/{z}/{x}/{y}/{version}/{data}
```

| 段 | 说明 |
|----|------|
| z | 缩放级别（整数） |
| x | 列号（整数） |
| y | 行号（整数） |
| version | 版本号（整数） |
| data | 数据路径（相对于 `HX_GIS_MAP_PATH`） |

**正则**: `^/gisserver/bundle/*/*/*/*/*.*`

**处理逻辑**:
1. 拼接完整路径: `{root_path}/{data}/`（若 URI 不以 `/` 结尾则追加 `/`）
2. 调用 `hx_bundle::hx_datasource_bundle_LoadImageData(x, y, z, version, db_path, byteArray, count, err_msg)`
3. 支持 V1/V2 两种 Bundle 打包格式

**响应**:
- `200` + 二进制瓦片数据（Content-Type 自动检测）
- `200` + 错误信息字符串（无数据时，Content-Type 由自动检测决定）

---

### 5. MBTiles 元数据

```
GET /gisserver/mbtiles/metadata/{data}
```

| 段 | 说明 |
|----|------|
| data | MBTiles 文件路径（相对于 `HX_GIS_MAP_PATH`，必须以 `.mbtiles` 结尾） |

**正则**: `^/gisserver/mbtiles/metadata/*.*`

**处理逻辑**:
1. 拼接完整路径: `{root_path}/{data}`
2. 验证扩展名为 `.mbtiles`
3. 打开 SQLite 数据库，查询 `metadata` 表:
   ```sql
   SELECT name, value FROM metadata WHERE name != 'json'
   ```
4. 将结果组装为 JSON 对象返回

**响应**:
- `200` + JSON 文本（`Content-Type: text/plain`），格式如 `{"name":"xxx","format":"png","bounds":"minLon,minLat,maxLon,maxLat",...}`
- `204 No Content`（无数据或文件不存在）

---

### 6. MBTiles 瓦片

```
GET /gisserver/mbtiles/{z}/{x}/{y}/{data}
```

| 段 | 说明 |
|----|------|
| z | 缩放级别（整数） |
| x | 列号（TMS 坐标系，整数） |
| y | 行号（TMS 坐标系，整数） |
| data | 数据路径（相对于 `HX_GIS_MAP_PATH`，可含子目录） |

**正则**: `^/gisserver/mbtiles/*/*/*/*.*`

**坐标转换**: URL 中的 `y` 为 TMS 坐标（原点左下角），查询时转换为 MBTiles 内部的 `tile_row`（原点左上角）:
```
tile_row = 2^z - 1 - y
```

**SQL 查询**:
```sql
SELECT tile_data FROM tiles
WHERE zoom_level = {z} AND tile_column = {x} AND tile_row = {ex_y}
```

同时查询格式信息:
```sql
SELECT value FROM metadata WHERE name = 'format'
```

**两种访问模式**:

| 模式 | 条件 | 行为 |
|------|------|------|
| 单文件 | `data` 以 `.mbtiles` 结尾 | 直接打开该文件查瓦片 |
| 目录聚合 | `data` 为目录路径 | 遍历目录下所有 `.mbtiles` 文件，通过 `metadata.bounds` 判断哪个文件包含请求坐标 |

**目录聚合缓存机制**:
- 首次访问目录时，打开目录下所有 `.mbtiles` 文件并缓存 DB 句柄到 `m_mbtiles` 列表
- 记录上次命中的 DB 句柄 `m_last_mbtiles_db`，下次优先尝试
- 相同目录路径不会重新扫描

**响应**:
- `200` + 二进制瓦片数据（Content-Type 自动检测：png/jpg/bmp，无法识别时按 `application/x-protobuf` + `gzip` 编码处理）
- `204 No Content` + `tileset get tiles none!`（无数据）
- `404 Not Found` + `mbtiles server error!`（data 为空时）

**请求示例**:
```bash
# 单文件
curl http://0.0.0.0:8080/gisserver/mbtiles/3/4/2/tiles/country.mbtiles

# 目录聚合
curl http://0.0.0.0:8080/gisserver/mbtiles/3/4/2/tiles/

# 元数据
curl http://0.0.0.0:8080/gisserver/mbtiles/metadata/tiles/country.mbtiles
```

---

### 7. 内存点集 - 创建

```
GET /gisserver/vtzero/create
```

**正则**: `^/gisserver/vtzero/create`

**处理逻辑**: 调用 `create_vtzero_database()` 创建内存 SQLite 数据库（`points` 表含 `latitude`, `longitude` 字段）。

**响应**: `200` + `vtzero database create success!` 或 `vtzero database create failed!`

---

### 8. 内存点集 - 插入数据

```
GET /gisserver/vtzero/restore/{x}/{y}
```

| 段 | 说明 |
|----|------|
| x | 经度（整数） |
| y | 纬度（整数） |

**正则**: `^/gisserver/vtzero/restore/.*/.*`

**处理逻辑**: 执行 SQL 插入点坐标:
```sql
INSERT INTO points (latitude, longitude) VALUES ({x}, {y})
```

**响应**:
- `200` + `insert data success!`
- `200` + `insert data failed!`（SQL 执行失败）
- `200` + `vtzero database not create!`（未先调用 create）

---

### 9. 内存点集 - 清空

```
GET /gisserver/vtzero/clear
```

**正则**: `^/gisserver/vtzero/clear`

**处理逻辑**: 执行 SQL 清空所有点:
```sql
DELETE FROM points
```

**响应**:
- `200` + `clear data success!` 或 `clear data failed!`
- `200` + `no database!`（未先调用 create）

---

### 10. 内存点集 - 瓦片查询

```
GET /gisserver/vtzero/{z}/{x}/{y}
```

| 段 | 说明 |
|----|------|
| z | 缩放级别 |
| x | 列号 |
| y | 行号 |

**正则**: `^/gisserver/vtzero/.*/.*/.*`

**处理逻辑**:
1. 将 xyz 转换为经纬度范围: `xyz2lnglat(x, y, z, &min_lng, &min_lat, &max_lng, &max_lat)`
2. 调用 `hx_datasource_vtzero_LoadData(min_lng, min_lat, max_lng, max_lat, db, data)` 查询范围内的点集

**响应**: `200` + MVT 矢量瓦片数据（`Content-Type: text/plain`）

---

### 11. GeoJSON 矢量瓦片

```
GET /gisserver/geojsonvt/{z}/{x}/{y}/{name}.mvt
```

| 段 | 说明 |
|----|------|
| z | 缩放级别 |
| x | 列号 |
| y | 行号 |
| name.mvt | 输出文件名（固定 `.mvt` 后缀） |

**正则**: `^/gisserver/geojsonvt/*/*/*/.*mvt`

**处理逻辑**:
1. 调用 `_hx_gis_server::transform_form_db(db, geojson, count, x, y, z)` 从 SQLite 数据库查询并渲染矢量瓦片
2. 失败时调用 `hx_geojsonvt_LoadEmptyImageData` 返回空瓦片
3. 需要先调用 `/gisserver/geojsonvt/start` 加载数据

**额外响应头**:
```
Cache-Control: public, max-age=1, must-revalidate
Last-Modified: <当前时间>
Etag: <MD5(response)>
```

**响应**: `200` + MVT 二进制数据（`Content-Type: application/octet-stream`）

---

### 12. GeoJSON - 启动

```
GET /gisserver/geojsonvt/start?data=<db_path>
```

| 参数 | 位置 | 必需 | 说明 |
|------|------|------|------|
| data | query | 是 | SQLite 数据库路径（GeoJSON 数据源） |

**正则**: `^/gisserver/geojsonvt/start`

**处理逻辑**: 调用 `gis_server->open_db(data)` 打开 SQLite 数据库。

**响应**: `200` + JSON（`Content-Type: text/plain`），格式如 `{"connect": 1, "status": "success"}` 或 `{"connect": -1, "status": "failed"}`

---

### 13. GeoJSON - 停止

```
GET /gisserver/geojsonvt/stop
```

**正则**: `^/gisserver/geojsonvt/stop`

**处理逻辑**: 调用 `gis_server->close_db()` 关闭 SQLite 数据库并释放资源。

**响应**: `200` + JSON，格式如 `{"disconnect": 1, "status": "success"}` 或 `{"disconnect": -1, "status": "failed"}`

---

## 协议替换

```
GET <任意文件路径>?schema=<protocol>
```

**匹配条件**: query string 中存在 `schema` 参数（无其他路由匹配时）。

**处理逻辑**:
1. 读取 `{root_path}{uri}` 对应的文件
2. 将文件内容中所有 `{protocol}://` 替换为实际请求地址

**替换目标地址**（按优先级）:
1. 从请求头提取: `X-Forwarded-Proto` + (`Host` 或 `X-Forwarded-Host`) → 如 `https://example.com`
2. 回退到服务器监听地址: `HX_GIS_SERVER_URL` → 如 `http://0.0.0.0:8080`

**响应**:
- `200` + 替换后的文件内容（`Content-Type: application/json`）
- `404` + `File not found`（文件不存在）
- `500` + `Memory allocation failed`

**示例**:
```
GET /data/style.json?schema=hxmap
→ 文件中 hxmap://path/to/resource 替换为 http://0.0.0.0:8080/path/to/resource
```

---

## 静态文件服务

以上路由均未匹配时，以 `HX_GIS_MAP_PATH` 为根目录提供静态文件服务（`mg_http_serve_dir`）。

支持自动内容类型检测:
- `image/png`（魔数 `0x89 0x50`）
- `image/jpeg`（魔数 `0xFF 0xD8`）
- `image/bmp`（魔数 `0x42 0x4D`）
- `text/plain`（null 数据时）
- `application/octet-stream` + `Content-Encoding: gzip`（其他）

---

## 公共响应头

所有响应统一附加以下安全头:

```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: "POST, GET, OPTIONS, DELETE"
Access-Control-Allow-Credentials: false
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
Cross-Origin-Resource-Policy: cross-origin
```

---

## 路由匹配顺序总结

| # | 路由 | 匹配方式 | 源码行 |
|---|------|----------|--------|
| 1 | `/gisserver/mapserv` | `mg_http_match_uri` 精确匹配 | L634 |
| 2 | `/gisserver/tianmu/*/*/*/*/*.*` | `std::regex` | L697 |
| 3 | `/gisserver/geopackage/*/*/*/*/*.*` | `std::regex` | L801 |
| 4 | `/gisserver/bundle/*/*/*/*/*.*` | `std::regex` | L903 |
| 5 | `/gisserver/mbtiles/metadata/*.*` | `std::regex` | L1016 |
| 6 | `/gisserver/mbtiles/*/*/*/*.*` | `std::regex` | L1052 |
| 7 | `/gisserver/vtzero/create` | `std::regex` | L1194 |
| 8 | `/gisserver/vtzero/restore/.*/.*` | `std::regex` | L1217 |
| 9 | `/gisserver/vtzero/clear` | `std::regex` | L1269 |
| 10 | `/gisserver/vtzero/.*/.*/.*` | `std::regex` | L1302 |
| 11 | `/gisserver/geojsonvt/*/*/*/.*mvt` | `std::regex` | L1359 |
| 12 | `/gisserver/geojsonvt/start` | `std::regex` | L1458 |
| 13 | `/gisserver/geojsonvt/stop` | `std::regex` | L1501 |
| 14 | `?schema=<protocol>` | query 参数检查 | L1533 |
| 15 | 其他 | `mg_http_serve_dir` 静态文件 | L1554 |
