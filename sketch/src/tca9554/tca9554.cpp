#include "TCA9554.h"

// --- TCA9554 IO Expander ---
// IO 位定义已移至 user_config.h

TCA9554::TCA9554(i2c_master_dev_handle_t handle) {
    dev = handle;
    outputState = 0x00;
    configState = 0xFF;
}

void TCA9554::begin() {
    writeRegister(CONFIG_REGISTER, configState);
    writeRegister(OUTPUT_REGISTER, outputState);
    log_i("TCA9554 OK");
}

void TCA9554::setPinMode(uint8_t pin_mask, uint8_t mode) {
    if (mode == 0)
        configState &= ~pin_mask;
    else
        configState |= pin_mask;

    writeRegister(CONFIG_REGISTER, configState);
}

void TCA9554::digitalWrite(uint8_t pin_mask, uint8_t state) {
    if (state)
        outputState |= pin_mask;
    else
        outputState &= ~pin_mask;

    writeRegister(OUTPUT_REGISTER, outputState);
}

uint8_t TCA9554::digitalRead(uint8_t pin_mask) {
    uint8_t value = readRegister(INPUT_REGISTER);
    return (value & pin_mask) ? 1 : 0;
}

void TCA9554::writeRegister(uint8_t reg, uint8_t data) {
    uint8_t buf[1] = { data };
    i2c_write_buff(dev, reg, buf, 1);
}

uint8_t TCA9554::readRegister(uint8_t reg) {
    uint8_t buf = 0;
    i2c_read_buff(dev, reg, &buf, 1);
    return buf;
}
