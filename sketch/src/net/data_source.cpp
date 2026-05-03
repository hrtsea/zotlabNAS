// ZotLab NAS Monitor - Data Source Factory Implementation
#include "data_source.h"
#include "api_client.h"
#include "serial_client.h"
#include "synology_client.h"
#include "qnap_client.h"
#include "truenas_client.h"
#include "netdata_client.h"
#include "snmp_client.h"
#include "mock_client.h"
#include "fnos_client.h"
#include "unraid_client.h"

// ============================================================================
// NasTypeConfig 默认值表
// ============================================================================
static const NasTypeConfig NAS_TYPE_CONFIGS[] = {
    // {type, default_ip, port, default_user, need_pass, need_apiurl, need_snmp, need_serial}
    {NAS_SYNOLOGY,     "192.168.1.100", (uint16_t)5000,   "admin",  true,  true,  false, false},
    {NAS_QNAP,         "192.168.1.100", (uint16_t)8080,   "admin",  true,  true,  false, false},
    {NAS_TRUENAS,      "192.168.1.100", (uint16_t)80,     "root",   true,  true,  false, false},
    {NAS_FNOS,         "192.168.1.100", (uint16_t)3000,   nullptr,  false, true,  false, false},
    {NAS_UNRAID,       "192.168.1.100", (uint16_t)80,     "root",   true,  false, false, false},
    {NET_LINUX_HTTP,   "192.168.1.100", (uint16_t)8099,  nullptr,  false, false, false, false},
    {NET_LINUX_SERIAL, "/dev/ttyUSB0",  (uint16_t)115200, nullptr,  false, false, false, true},
    {NET_NETDATA,      "192.168.1.100", (uint16_t)19999, nullptr,  false, true,  false, false},
    {NET_SNMP,         "192.168.1.100", (uint16_t)161,    nullptr,  false, false, true,  false},
    {NET_WINDOWS,      "192.168.1.100", (uint16_t)0,      "admin",  true,  false, false, false},
    {NAS_MOCK,         "",              (uint16_t)0,        nullptr,  false, false, false, false},
};
static const int NAS_TYPE_CONFIGS_COUNT = sizeof(NAS_TYPE_CONFIGS) / sizeof(NAS_TYPE_CONFIGS[0]);

// 根据枚举类型查找配置
NasTypeConfig NasTypeConfig::getDefaults(NasType type) {
    for (int i = 0; i < NAS_TYPE_CONFIGS_COUNT; i++) {
        if (NAS_TYPE_CONFIGS[i].type == type) {
            return NAS_TYPE_CONFIGS[i];
        }
    }
    return {};  // 返回空结构
}

// 根据字符串ID查找配置
NasTypeConfig NasTypeConfig::getDefaults(const char* type_id) {
    NasType type = nasTypeFromString(type_id);
    return getDefaults(type);
}

// 默认disconnect实现（子类可覆写）
void DataSource::disconnect() {
    Serial.println(F("[DataSource] Default disconnect called"));
    // 子类应该覆写此方法以进行特定的清理工作
}

static struct {
    const char* id;
    DataSource* (*create)();
} factory_table[] = {
    {"linux_http",   []() -> DataSource* { 
        auto* client = new ApiClient(); 
        client->setType(NET_LINUX_HTTP); 
        return client; 
    }},
    {"linux_serial", []() -> DataSource* { return new SerialClient(); }},
    {"windows",      []() -> DataSource* { 
        auto* client = new ApiClient(); 
        client->setType(NET_WINDOWS); 
        return client; 
    }},
    {"synology",     []() -> DataSource* { return new SynologyClient(); }},
    {"qnap",         []() -> DataSource* { return new QnapClient(); }},
    {"truenas",      []() -> DataSource* { return new TrueNASClient(); }},
    {"fnos",         []() -> DataSource* { return new FnosDataSource(); }},
    {"unraid",       []() -> DataSource* { return new UnraidDataSource(); }},
    {"netdata",      []() -> DataSource* { return new NetdataClient(); }},
    {"snmp",         []() -> DataSource* { return new SnmpClient(); }},
    {"mock",         []() -> DataSource* { return new MockDataSource(); }},
};
static const int factory_count = sizeof(factory_table) / sizeof(factory_table[0]);

DataSource* createDataSource(const char* nas_type_id) {
    for (int i = 0; i < factory_count; i++) {
        if (strcmp(factory_table[i].id, nas_type_id) == 0) {
            return factory_table[i].create();
        }
    }
    Serial.printf("[DataSource] Unknown type: %s\n", nas_type_id);
    return nullptr;
}

const char* getDisplayTypeName(const char* nas_type_id) {
    for (int i = 0; i < DATA_TYPE_COUNT; i++) {
        if (strcmp(NAS_TYPES[i].id, nas_type_id) == 0) {
            return NAS_TYPES[i].display_name;
        }
    }
    return "Unknown";
}

NasType nasTypeFromString(const char* nas_type_id) {
    if (strcmp(nas_type_id, "synology") == 0) return NAS_SYNOLOGY;
    if (strcmp(nas_type_id, "qnap") == 0) return NAS_QNAP;
    if (strcmp(nas_type_id, "truenas") == 0) return NAS_TRUENAS;
    if (strcmp(nas_type_id, "fnos") == 0) return NAS_FNOS;
    if (strcmp(nas_type_id, "unraid") == 0) return NAS_UNRAID;
    if (strcmp(nas_type_id, "netdata") == 0) return NET_NETDATA;
    if (strcmp(nas_type_id, "snmp") == 0) return NET_SNMP;
    if (strcmp(nas_type_id, "linux_http") == 0) return NET_LINUX_HTTP;
    if (strcmp(nas_type_id, "linux_serial") == 0) return NET_LINUX_SERIAL;
    if (strcmp(nas_type_id, "windows") == 0) return NET_WINDOWS;
    if (strcmp(nas_type_id, "mock") == 0) return NAS_MOCK;
    return NET_LINUX_HTTP;
}

const char* nasTypeToString(NasType type) {
    switch (type) {
        case NAS_SYNOLOGY: return "synology";
        case NAS_QNAP: return "qnap";
        case NAS_TRUENAS: return "truenas";
        case NAS_FNOS: return "fnos";
        case NAS_UNRAID: return "unraid";
        case NET_NETDATA: return "netdata";
        case NET_SNMP: return "snmp";
        case NET_LINUX_HTTP: return "linux_http";
        case NET_LINUX_SERIAL: return "linux_serial";
        case NET_WINDOWS: return "windows";
        case NAS_MOCK: return "mock";
        default: return "linux_http";
    }
}
