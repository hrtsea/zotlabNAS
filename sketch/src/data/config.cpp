// ZotLab NAS Monitor - NVS Configuration Implementation
#include "config.h"

AppConfig g_config;
ConfigBackup g_config_backup;  // 全局配置备份

static Preferences prefs;

void config_load() {
    // Initialize g_config with defaults FIRST
    memset(&g_config, 0, sizeof(AppConfig));
    strlcpy(g_config.nas_type, "mock", sizeof(g_config.nas_type));  // Default to Mock for testing
    g_config.nas_port = 0;
    g_config.poll_sec = DEFAULT_POLL_SEC;
    g_config.rotation_angle = 0;  // Default 0°
    g_config.brightness = 128;
    g_config.autodim = true;
    g_config.timezone = 8;
    g_config.serial_baud = DEFAULT_SERIAL_BAUD;
    g_config.snmp_ver = 2;  // Default SNMP v2c
    g_config.sata_disk_count = DEFAULT_SATA_DISK_COUNT;  // 默认 SATA 硬盘数
    g_config.m2_disk_count = DEFAULT_M2_DISK_COUNT;      // 默认 M.2 硬盘数
    g_config.auto_cycle_enabled = false;                 // 默认禁用自动循环
    g_config.auto_cycle_interval_sec = 10;               // 默认10秒间隔
    g_config.fan.enabled = FAN_ENABLED;
    g_config.fan.mode = FAN_MODE_MANUAL;
    g_config.fan.manual_pwm_pct = 50;
    g_config.fan.temp_source = TEMP_MAX_CPU_SYS;
    g_config.fan.hysteresis = FAN_DEFAULT_HYSTERESIS;
    g_config.fan.min_change_pct = FAN_DEFAULT_MIN_PWM;
    g_config.fan.min_pwm_pct = FAN_DEFAULT_MIN_PWM;
    g_config.fan.emergency_temp = FAN_DEFAULT_EMERGENCY;
    g_config.fan.ramp_time_ms = FAN_DEFAULT_RAMP_MS;
    g_config.fan.stall_detect_sec = FAN_DEFAULT_STALL_SEC;
    memcpy(g_config.fan.curve, DEFAULT_FAN_CURVE, sizeof(FanCurvePoint) * FAN_CURVE_POINTS);
    
    // Try to load from NVS (may fail on first boot)
    bool ret = prefs.begin(NVS_NAMESPACE, true); // read-only
    if (!ret) {
        Serial.println(F("[Config] NVS open failed, using defaults"));
        return;  // Use defaults initialized above
    }

    // WiFi
    prefs.getString(NVS_WIFI_SSID, g_config.ssid, sizeof(g_config.ssid));
    prefs.getString(NVS_WIFI_PASS, g_config.wifipass, sizeof(g_config.wifipass));

    // NAS
    prefs.getString(NVS_NAS_TYPE, g_config.nas_type, sizeof(g_config.nas_type));
    if (strlen(g_config.nas_type) == 0) {
        strlcpy(g_config.nas_type, "mock", sizeof(g_config.nas_type)); // Default to Mock
    }
    prefs.getString(NVS_NAS_IP, g_config.nas_ip, sizeof(g_config.nas_ip));
    g_config.nas_port = prefs.getUShort(NVS_NAS_PORT, 0);
    prefs.getString(NVS_NAS_USER, g_config.nas_user, sizeof(g_config.nas_user));
    prefs.getString(NVS_NAS_PASS, g_config.nas_pass, sizeof(g_config.nas_pass));
    g_config.nas_https = prefs.getBool(NVS_NAS_HTTPS, false);
    prefs.getString(NVS_SNMP_COMM, g_config.snmp_comm, sizeof(g_config.snmp_comm));
    g_config.snmp_ver = prefs.getUChar(NVS_SNMP_VER, 2);
    g_config.serial_baud = prefs.getUInt(NVS_SERIAL_BAUD, DEFAULT_SERIAL_BAUD);

    // Polling & Display
    g_config.poll_sec = prefs.getUChar(NVS_POLL_SEC, DEFAULT_POLL_SEC);
    g_config.rotation_angle = prefs.getUChar(NVS_ROTATION_ANGLE, 0);
    g_config.brightness = prefs.getUChar(NVS_BRIGHTNESS, 128);
    g_config.autodim = prefs.getBool(NVS_AUTODIM, true);
    g_config.timezone = (int8_t)prefs.getChar(NVS_TIMEZONE, 8);
    g_config.sata_disk_count = prefs.getUChar(NVS_SATA_DISK_COUNT, DEFAULT_SATA_DISK_COUNT);
    g_config.m2_disk_count = prefs.getUChar(NVS_M2_DISK_COUNT, DEFAULT_M2_DISK_COUNT);
    
    // Weather
    prefs.getString(NVS_WEATHER_API_KEY, g_config.weather_api_key, sizeof(g_config.weather_api_key));
    prefs.getString(NVS_WEATHER_CITY, g_config.weather_city, sizeof(g_config.weather_city));
    
    // Auto Cycle
    g_config.auto_cycle_enabled = prefs.getBool(NVS_AUTO_CYCLE_ENABLE, false);
    g_config.auto_cycle_interval_sec = prefs.getUChar(NVS_AUTO_CYCLE_INTERVAL, 10);

    // Fan
    g_config.fan.enabled = prefs.getBool(NVS_FAN_ENABLE, FAN_ENABLED);
    g_config.fan.mode = (FanMode)prefs.getUChar(NVS_FAN_MODE, 0);
    g_config.fan.manual_pwm_pct = prefs.getUChar(NVS_FAN_MANUAL, 50);
    g_config.fan.temp_source = (TempSource)prefs.getUChar(NVS_FAN_TSRC, TEMP_MAX_CPU_SYS);
    g_config.fan.hysteresis = prefs.getUChar(NVS_FAN_HYST, FAN_DEFAULT_HYSTERESIS);
    g_config.fan.min_pwm_pct = prefs.getUChar(NVS_FAN_MIN, FAN_DEFAULT_MIN_PWM);
    g_config.fan.emergency_temp = (int16_t)prefs.getShort(NVS_FAN_EMERG, FAN_DEFAULT_EMERGENCY);
    g_config.fan.ramp_time_ms = prefs.getUShort(NVS_FAN_RAMP, FAN_DEFAULT_RAMP_MS);
    g_config.fan.stall_detect_sec = prefs.getUChar(NVS_FAN_STALL, FAN_DEFAULT_STALL_SEC);

    // Fan curve (stored as blob)
    size_t sz = prefs.getBytes(NVS_FAN_CURVE, g_config.fan.curve,
                               sizeof(FanCurvePoint) * FAN_CURVE_POINTS);
    if (sz != sizeof(FanCurvePoint) * FAN_CURVE_POINTS) {
        memcpy(g_config.fan.curve, DEFAULT_FAN_CURVE,
               sizeof(FanCurvePoint) * FAN_CURVE_POINTS);
    }

    prefs.end();
}

void config_save() {
    prefs.begin(NVS_NAMESPACE, false);

    prefs.putString(NVS_WIFI_SSID, g_config.ssid);
    prefs.putString(NVS_WIFI_PASS, g_config.wifipass);
    prefs.putString(NVS_NAS_TYPE, g_config.nas_type);
    prefs.putString(NVS_NAS_IP, g_config.nas_ip);
    prefs.putUShort(NVS_NAS_PORT, g_config.nas_port);
    prefs.putString(NVS_NAS_USER, g_config.nas_user);
    prefs.putString(NVS_NAS_PASS, g_config.nas_pass);
    prefs.putBool(NVS_NAS_HTTPS, g_config.nas_https);
    prefs.putString(NVS_SNMP_COMM, g_config.snmp_comm);
    prefs.putUChar(NVS_SNMP_VER, g_config.snmp_ver);
    prefs.putUInt(NVS_SERIAL_BAUD, g_config.serial_baud);
    prefs.putUChar(NVS_POLL_SEC, g_config.poll_sec);
    prefs.putUChar(NVS_ROTATION_ANGLE, g_config.rotation_angle);
    prefs.putUChar(NVS_BRIGHTNESS, g_config.brightness);
    prefs.putBool(NVS_AUTODIM, g_config.autodim);
    prefs.putChar(NVS_TIMEZONE, (char)g_config.timezone);
    prefs.putUChar(NVS_SATA_DISK_COUNT, g_config.sata_disk_count);
    prefs.putUChar(NVS_M2_DISK_COUNT, g_config.m2_disk_count);
    
    // Weather
    prefs.putString(NVS_WEATHER_API_KEY, g_config.weather_api_key);
    prefs.putString(NVS_WEATHER_CITY, g_config.weather_city);
    
    // Auto Cycle
    prefs.putBool(NVS_AUTO_CYCLE_ENABLE, g_config.auto_cycle_enabled);
    prefs.putUChar(NVS_AUTO_CYCLE_INTERVAL, g_config.auto_cycle_interval_sec);

    // Fan
    prefs.putBool(NVS_FAN_ENABLE, g_config.fan.enabled);
    prefs.putUChar(NVS_FAN_MODE, (uint8_t)g_config.fan.mode);
    prefs.putUChar(NVS_FAN_MANUAL, g_config.fan.manual_pwm_pct);
    prefs.putUChar(NVS_FAN_TSRC, (uint8_t)g_config.fan.temp_source);
    prefs.putUChar(NVS_FAN_HYST, g_config.fan.hysteresis);
    prefs.putUChar(NVS_FAN_MIN, g_config.fan.min_pwm_pct);
    prefs.putShort(NVS_FAN_EMERG, (int16_t)g_config.fan.emergency_temp);
    prefs.putUShort(NVS_FAN_RAMP, g_config.fan.ramp_time_ms);
    prefs.putUChar(NVS_FAN_STALL, g_config.fan.stall_detect_sec);
    prefs.putBytes(NVS_FAN_CURVE, g_config.fan.curve,
                   sizeof(FanCurvePoint) * FAN_CURVE_POINTS);

    prefs.end();
}

void config_save_wifi(const char* ssid, const char* pass) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString(NVS_WIFI_SSID, ssid);
    if (pass && strlen(pass) > 0) prefs.putString(NVS_WIFI_PASS, pass);
    prefs.end();
    strlcpy(g_config.ssid, ssid, sizeof(g_config.ssid));
    if (pass && strlen(pass) > 0) strlcpy(g_config.wifipass, pass, sizeof(g_config.wifipass));
}

void config_save_nas(const char* type, const char* ip, uint16_t port,
                     const char* user, const char* pass, bool https) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString(NVS_NAS_TYPE, type);
    prefs.putString(NVS_NAS_IP, ip);
    prefs.putUShort(NVS_NAS_PORT, port);
    prefs.putString(NVS_NAS_USER, user);
    prefs.putString(NVS_NAS_PASS, pass);
    prefs.putBool(NVS_NAS_HTTPS, https);
    prefs.end();

    // 更新全局配置（但不立即触发数据源切换）
    // main.cpp 的 loop() 会检测到变化并处理
    strlcpy(g_config.nas_type, type, sizeof(g_config.nas_type));
    strlcpy(g_config.nas_ip, ip, sizeof(g_config.nas_ip));
    g_config.nas_port = port;
    strlcpy(g_config.nas_user, user, sizeof(g_config.nas_user));
    strlcpy(g_config.nas_pass, pass, sizeof(g_config.nas_pass));
    g_config.nas_https = https;
    
    // 强制延迟一下，让LVGL事件处理完成
    delay(10);
}

void config_save_display(uint8_t rotation_angle, uint8_t brightness, bool autodim) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_ROTATION_ANGLE, rotation_angle);
    prefs.putUChar(NVS_BRIGHTNESS, brightness);
    prefs.putBool(NVS_AUTODIM, autodim);
    prefs.end();

    g_config.rotation_angle = rotation_angle;
    g_config.brightness = brightness;
    g_config.autodim = autodim;
}

void config_save_fan(const FanConfig& fan) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool(NVS_FAN_ENABLE, fan.enabled);
    prefs.putUChar(NVS_FAN_MODE, (uint8_t)fan.mode);
    prefs.putUChar(NVS_FAN_MANUAL, fan.manual_pwm_pct);
    prefs.putUChar(NVS_FAN_TSRC, (uint8_t)fan.temp_source);
    prefs.putUChar(NVS_FAN_HYST, fan.hysteresis);
    prefs.putUChar(NVS_FAN_MIN, fan.min_pwm_pct);
    prefs.putShort(NVS_FAN_EMERG, (int16_t)fan.emergency_temp);
    prefs.putUShort(NVS_FAN_RAMP, fan.ramp_time_ms);
    prefs.putUChar(NVS_FAN_STALL, fan.stall_detect_sec);
    prefs.putBytes(NVS_FAN_CURVE, fan.curve, sizeof(FanCurvePoint) * FAN_CURVE_POINTS);
    prefs.end();

    g_config.fan = fan;
}

void config_save_disk_config(uint8_t sata_count, uint8_t m2_count) {
    // 只限制总数不超过 MAX_DISKS
    if (sata_count + m2_count > MAX_DISKS) {
        // 按比例缩减
        uint16_t total = sata_count + m2_count;
        sata_count = (uint8_t)((uint32_t)sata_count * MAX_DISKS / total);
        m2_count = MAX_DISKS - sata_count;
    }
    
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_SATA_DISK_COUNT, sata_count);
    prefs.putUChar(NVS_M2_DISK_COUNT, m2_count);
    prefs.end();
    
    g_config.sata_disk_count = sata_count;
    g_config.m2_disk_count = m2_count;
}

void config_reset() {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
    config_load();
}

// 保存当前配置到备份
void config_save_backup() {
    strlcpy(g_config_backup.nas_type, g_config.nas_type, sizeof(g_config_backup.nas_type));
    strlcpy(g_config_backup.nas_ip, g_config.nas_ip, sizeof(g_config_backup.nas_ip));
    g_config_backup.nas_port = g_config.nas_port;
    strlcpy(g_config_backup.nas_user, g_config.nas_user, sizeof(g_config_backup.nas_user));
    strlcpy(g_config_backup.nas_pass, g_config.nas_pass, sizeof(g_config_backup.nas_pass));
    g_config_backup.nas_https = g_config.nas_https;
    strlcpy(g_config_backup.snmp_comm, g_config.snmp_comm, sizeof(g_config_backup.snmp_comm));
    g_config_backup.snmp_ver = g_config.snmp_ver;
    g_config_backup.serial_baud = g_config.serial_baud;
    
    Serial.println(F("[Config] Backup saved"));
}

// 从备份恢复配置
void config_restore_backup() {
    strlcpy(g_config.nas_type, g_config_backup.nas_type, sizeof(g_config.nas_type));
    strlcpy(g_config.nas_ip, g_config_backup.nas_ip, sizeof(g_config.nas_ip));
    g_config.nas_port = g_config_backup.nas_port;
    strlcpy(g_config.nas_user, g_config_backup.nas_user, sizeof(g_config.nas_user));
    strlcpy(g_config.nas_pass, g_config_backup.nas_pass, sizeof(g_config.nas_pass));
    g_config.nas_https = g_config_backup.nas_https;
    strlcpy(g_config.snmp_comm, g_config_backup.snmp_comm, sizeof(g_config.snmp_comm));
    g_config.snmp_ver = g_config_backup.snmp_ver;
    g_config.serial_baud = g_config_backup.serial_baud;
    
    Serial.println(F("[Config] Restored from backup"));
}
