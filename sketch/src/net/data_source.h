// ZotLab NAS Monitor - Data Source Abstraction Layer
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "nas_data.h"
#include "config.h"

// NAS类型配置结构体（用于UI默认值和字段可见性）
struct NasTypeConfig {
    NasType type;              // NAS类型枚举
    const char* default_ip;     // 默认IP地址
    uint16_t default_port;     // 默认端口
    const char* default_user;   // 默认用户名（可为nullptr）
    bool need_password;        // 是否需要密码
    bool need_apiurl;          // 是否需要API URL
    bool need_snmp;            // 是否需要SNMP配置（community/version）
    bool need_serial;          // 是否需要串口配置（device/baud）
    
    // 静态方法：根据类型获取默认值（不需实例化）
    static NasTypeConfig getDefaults(NasType type);
    static NasTypeConfig getDefaults(const char* type_id);
};

class DataSource {
public:
    virtual ~DataSource() = default;
    virtual bool init(Preferences& prefs) = 0;
    virtual bool connect() = 0;
    virtual void disconnect();  // 优雅关闭连接
    virtual bool poll() = 0;
    virtual bool isConnected() = 0;
    virtual const NasData& getData() = 0;
    virtual const char* getTypeName() = 0;
    virtual const char* getConnIcon() = 0;
    
    // 获取该类型的配置信息（用于UI默认值）
    virtual NasTypeConfig getConfig() = 0;
};

struct NasTypeEntry {
    const char* id;             // 字符串ID
    const char* display_name;   // 显示名称
    NasType nas_type_enum;      // NasType枚举值
    bool implemented;           // 是否已实现
    // 注意：图标由UI层定义，避免LVGL依赖
};

static const NasTypeEntry NAS_TYPES[] = {
    {"synology",     "Synology DSM",   NAS_SYNOLOGY,    true},
    {"qnap",         "QNAP QTS",       NAS_QNAP,        true},
    {"truenas",      "TrueNAS",        NAS_TRUENAS,     true},
    {"fnos",         "FNOS",           NAS_FNOS,        true},
    {"unraid",       "Unraid",         NAS_UNRAID,      true},
    {"netdata",      "Netdata",        NET_NETDATA,     true},
    {"snmp",         "SNMP",           NET_SNMP,        true},
    {"linux_http",   "Linux (HTTP)",   NET_LINUX_HTTP,  true},
    {"linux_serial", "Linux (Serial)", NET_LINUX_SERIAL,true},
    {"windows",      "Windows",        NET_WINDOWS,     true},
    {"mock",         "Mock (测试)",    NAS_MOCK,        true},
};

static const int DATA_TYPE_COUNT = sizeof(NAS_TYPES) / sizeof(NAS_TYPES[0]);

DataSource* createDataSource(const char* nas_type_id);
const char* getDisplayTypeName(const char* nas_type_id);
NasType nasTypeFromString(const char* nas_type_id);
const char* nasTypeToString(NasType type);

// 全局数据源实例（由 appStart() 初始化）
extern DataSource* g_data_source;

// 安全切换数据源（先 disconnect + delete 旧实例，再创建新实例）
// 返回 true 表示切换成功
bool switchDataSource(const char* nas_type_id);

// C 代码可用的包装函数
#ifdef __cplusplus
extern "C" {
#endif
    bool data_source_is_connected(void);
    const char* data_source_get_type_name(void);
    float data_source_get_rx_speed_mbps(void);  // 返回 MB/s
    float data_source_get_tx_speed_mbps(void);  // 返回 MB/s
    bool data_source_switch(const char* nas_type_id);  // C 包装函数
#ifdef __cplusplus
}
#endif
