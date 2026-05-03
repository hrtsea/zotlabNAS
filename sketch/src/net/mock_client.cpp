// ZotLab NAS Monitor - Mock Data Source for Testing
#include "mock_client.h"
#include "config.h"
#include "nas_data.h"
#include <math.h>
#include <Arduino.h>

MockDataSource::MockDataSource() {
    memset(&data, 0, sizeof(data));
    last_poll_ms = 0;
}

MockDataSource::~MockDataSource() {}

// 生成模拟数据
void MockDataSource::generateMockData() {
    static uint32_t counter = 0;
    counter++;
    
    // 系统信息
    strncpy(data.system.hostname, "ZotLab-NAS-Mock", sizeof(data.system.hostname));
    strncpy(data.system.model, "Mock NAS DS920+", sizeof(data.system.model));
    data.system.uptime_s = counter * 100;
    data.system.temp_cpu = 42 + (counter % 10);
    data.system.temp_sys = 38 + (counter % 8);
    data.system.cpu_pct = 25.0f + (counter % 30);
    data.system.ram_total_mb = 8192;
    data.system.ram_used_mb = 3276 + (counter % 500);
    data.system.ram_pct = (data.system.ram_used_mb * 100.0f) / data.system.ram_total_mb;
    
    // CPU核心信息（4核心）
    data.system.cpu_core_count = 4;
    for (int i = 0; i < 4; i++) {
        data.system.cpu_cores[i] = 20.0f + ((counter + i * 10) % 40);
    }
    
    // 负载平均值（1/5/15分钟）
    data.system.load_avg[0] = 0.5f + (counter % 20) / 10.0f;
    data.system.load_avg[1] = 0.8f;
    data.system.load_avg[2] = 1.2f;
    
    // RAM详细信息
    data.system.ram_free_mb = data.system.ram_total_mb - data.system.ram_used_mb;
    data.system.ram_cached_mb = 512;
    data.system.swap_total_mb = 2048;
    data.system.swap_used_mb = 128;
    
    // 风扇信息 (独立的mock数据，模拟真实风扇控制)
    // 模拟温度变化引起的风扇转速波动
    float simulated_temp = 45.0f + sin(counter * 0.1f) * 15.0f;  // 30-60C 正弦波
    data.fan.ctrl_temp = (int16_t)simulated_temp;
    
    // 根据温度模拟PWM (使用简单的线性曲线)
    uint8_t simulated_pwm = 0;
    if (simulated_temp < 35) {
        simulated_pwm = 20;  // 最低20%
    } else if (simulated_temp < 50) {
        simulated_pwm = 20 + (uint8_t)((simulated_temp - 35) * 2.0f);  // 20-50%
    } else if (simulated_temp < 65) {
        simulated_pwm = 50 + (uint8_t)((simulated_temp - 50) * 2.0f);  // 50-80%
    } else {
        simulated_pwm = 80 + (uint8_t)((simulated_temp - 65) * 1.33f);  // 80-100%
    }
    if (simulated_pwm > 100) simulated_pwm = 100;
    
    data.fan.pwm_pct = simulated_pwm;
    
    // 根据PWM模拟RPM (假设风扇范围: 800-3000 RPM)
    data.fan.rpm = 800 + (uint32_t)(simulated_pwm * 22.0f);
    
    // 添加一些随机波动
    data.fan.rpm += (counter % 50) - 25;  // ±25 RPM波动
    data.fan.pwm_pct += (counter % 3) - 1;  // ±1%波动
    
    data.fan.enabled = true;
    
    // 模拟堵转报警 (极少发生)
    data.fan.stall_alarm = (counter % 500 == 0);  // 每500次触发一次
    
    // 磁盘信息（6块 SATA + 2块 M.2 = 8块磁盘配置）
    // 实际在线：3 SATA + 1 M.2 = 4块（HEALTH_OK）
    // 损坏：2 SATA（HEALTH_CRITICAL）
    // 离线：1 SATA + 1 M.2（不在 data.disks 中）
    data.disk_count = 6;  // 6 块硬盘有数据（4 在线 + 2 损坏）
    
    // SATA 硬盘 1-3：在线且健康
    for (int i = 0; i < 3; i++) {
        snprintf(data.disks[i].name, sizeof(data.disks[i].name), "Disk %d", i+1);
        snprintf(data.disks[i].model_name, sizeof(data.disks[i].model_name), "WD RED %dTB", 4+i);
        snprintf(data.disks[i].mount, sizeof(data.disks[i].mount), "/dev/sd%c", 'a'+i);
        data.disks[i].size_gb = 4000 + (i * 1000);
        data.disks[i].used_gb = 2000 + (counter % 1000) + (i * 200);
        data.disks[i].used_pct = (data.disks[i].used_gb * 100) / data.disks[i].size_gb;
        data.disks[i].temp = 35 + (counter % 8) + i;
        data.disks[i].health = HEALTH_OK;  // 健康
        
        // SATA 硬盘读写速度（模拟波动）
        data.disks[i].read_kbps = 100000 + (counter % 50000) + (i * 10000);  // 100-160 MB/s
        data.disks[i].write_kbps = 80000 + (counter % 40000) + (i * 8000);   // 80-128 MB/s
    }
    
    // SATA 硬盘 4-5：损坏
    for (int i = 3; i < 5; i++) {
        snprintf(data.disks[i].name, sizeof(data.disks[i].name), "Disk %d", i+1);
        snprintf(data.disks[i].model_name, sizeof(data.disks[i].model_name), "WD RED %dTB", 4+i);
        snprintf(data.disks[i].mount, sizeof(data.disks[i].mount), "/dev/sd%c", 'a'+i);
        data.disks[i].size_gb = 4000 + (i * 1000);
        data.disks[i].used_gb = 0;  // 损坏无法读取
        data.disks[i].used_pct = 0;
        data.disks[i].temp = 0;  // 无温度数据
        data.disks[i].health = HEALTH_CRITICAL;  // 严重故障
        data.disks[i].read_kbps = 0;  // 损坏无法读写
        data.disks[i].write_kbps = 0;
    }
    
    // SATA 硬盘 6：离线（不在 data.disks 中）
    
    // M.2 NVMe 硬盘 1：在线且健康（索引 5）
    int m2_online_idx = 5;
    snprintf(data.disks[m2_online_idx].name, sizeof(data.disks[m2_online_idx].name), "M.2 1");
    snprintf(data.disks[m2_online_idx].model_name, sizeof(data.disks[m2_online_idx].model_name), "Samsung 980 PRO 1TB");
    snprintf(data.disks[m2_online_idx].mount, sizeof(data.disks[m2_online_idx].mount), "/dev/nvme0n1");
    data.disks[m2_online_idx].size_gb = 1000;
    data.disks[m2_online_idx].used_gb = 500 + (counter % 500);
    data.disks[m2_online_idx].used_pct = (data.disks[m2_online_idx].used_gb * 100) / data.disks[m2_online_idx].size_gb;
    data.disks[m2_online_idx].temp = 40 + (counter % 10);
    data.disks[m2_online_idx].health = HEALTH_OK;  // 健康
    
    // M.2 NVMe 读写速度（比 SATA 快）
    data.disks[m2_online_idx].read_kbps = 300000 + (counter % 100000);   // 300-400 MB/s
    data.disks[m2_online_idx].write_kbps = 250000 + (counter % 80000);  // 250-330 MB/s
    
    // M.2 NVMe 硬盘 2：离线（不在 data.disks 中）
    
    // 卷信息（2个卷）
    data.volume_count = 2;
    for (int i = 0; i < 2; i++) {
        snprintf(data.volumes[i].name, sizeof(data.volumes[i].name), "Volume %d", i+1);
        data.volumes[i].total_gb = 8000 + (i * 4000);
        data.volumes[i].used_gb = 4000 + (counter % 1500) + (i * 500);
        data.volumes[i].used_pct = (data.volumes[i].used_gb * 100) / data.volumes[i].total_gb;
        strncpy(data.volumes[i].raid, "RAID5", sizeof(data.volumes[i].raid));
        strncpy(data.volumes[i].status, "normal", sizeof(data.volumes[i].status));
    }
    
    // 服务（8个服务）
    data.service_count = 8;
    const char* service_names[] = {"SMB", "NFS", "AFP", "FTP", "SSH", "Docker", "Web Station", "Surveillance"};
    for (int i = 0; i < 8; i++) {
        strncpy(data.services[i].name, service_names[i], sizeof(data.services[i].name));
        data.services[i].running = (i < 6);
        data.services[i].is_docker = (i == 5);
    }
    
    // 网络信息 (合理范围: 10-100 MB/s)
    data.network.rx_bps = (10 + (counter % 90)) * 1000000;  // 10-99 MB/s
    data.network.tx_bps = (5 + (counter % 45)) * 1000000;   // 5-49 MB/s
    strncpy(data.network.ip, "192.168.1.100", sizeof(data.network.ip));
    strncpy(data.network.interface, "eth0", sizeof(data.network.interface));
    
    // 多网口信息（模拟3个网口）
    data.interface_count = 3;
    
    // eth0 - 主网口（千兆以太网）
    strncpy(data.interfaces[0].name, "eth0", sizeof(data.interfaces[0].name));
    strncpy(data.interfaces[0].ip, "192.168.1.100", sizeof(data.interfaces[0].ip));
    data.interfaces[0].rx_bps = (10 + (counter % 90)) * 125;  // 10-99 KB/s (转换为bytes/s)
    data.interfaces[0].tx_bps = (5 + (counter % 45)) * 125;   // 5-49 KB/s
    data.interfaces[0].active = true;
    
    // eth1 - 第二网口（千兆以太网，未使用）
    strncpy(data.interfaces[1].name, "eth1", sizeof(data.interfaces[1].name));
    strncpy(data.interfaces[1].ip, "192.168.2.100", sizeof(data.interfaces[1].ip));
    data.interfaces[1].rx_bps = (counter % 5) * 125;  // 0-4 KB/s
    data.interfaces[1].tx_bps = (counter % 3) * 125;  // 0-2 KB/s
    data.interfaces[1].active = false;
    
    // wlan0 - WiFi（2.4GHz）
    strncpy(data.interfaces[2].name, "wlan0", sizeof(data.interfaces[2].name));
    strncpy(data.interfaces[2].ip, "192.168.1.150", sizeof(data.interfaces[2].ip));
    data.interfaces[2].rx_bps = (2 + (counter % 20)) * 125;  // 2-21 KB/s
    data.interfaces[2].tx_bps = (1 + (counter % 10)) * 125;  // 1-10 KB/s
    data.interfaces[2].active = true;
    
    // 默认选择第一个网口
    data.active_interface_idx = 0;
    
    // 风扇信息
    data.fan.rpm = 1200 + (counter % 300);
    data.fan.pwm_pct = 40 + (counter % 20);
    data.fan.enabled = true;
    data.fan.stall_alarm = false;
    data.fan.ctrl_temp = data.system.temp_cpu;
}

bool MockDataSource::init(Preferences& prefs) {
    Serial.printf("[Mock] Init: Mock Data Source\n");
    generateMockData();
    return true;
}

bool MockDataSource::connect() {
    Serial.println("[Mock] Connected (simulated)");
    return true;
}

bool MockDataSource::poll() {
    uint32_t now = millis();
    if (now - last_poll_ms < g_config.poll_sec * 1000UL) return false;

    generateMockData();
    data.is_online = true;
    data.last_update_ms = now;
    data.has_update = true;
    last_poll_ms = now;
    return true;
}

bool MockDataSource::isConnected() {
    return true;
}

const NasData& MockDataSource::getData() {
    return data;
}

const char* MockDataSource::getTypeName() {
    return "Mock Data Source";
}

const char* MockDataSource::getConnIcon() {
    return "\xf0\x9f\x92\xbb";  // 💻 emoji
}
