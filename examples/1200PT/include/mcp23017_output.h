#pragma once

#include "mcp23017_output_safety.h"

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <cstddef>
#include <cstdint>

#define MCP23017_1_ADDR       0x20
#define MCP23017_2_ADDR       0x21
#define MCP23017_I2C_SDA_PIN  21
#define MCP23017_I2C_SCL_PIN  22
#define MCP23017_I2C_CLOCK_HZ 100000

struct MCP23017RegisterWrite
{
    std::uint8_t address;
    std::uint8_t reg;
    std::uint8_t value;
};

class MCP23017OutputLock
{
public:
    MCP23017OutputLock();
    ~MCP23017OutputLock();
    bool is_locked() const { return locked; }
    MCP23017OutputLock(const MCP23017OutputLock &) = delete;
    MCP23017OutputLock &operator=(const MCP23017OutputLock &) = delete;

private:
    bool locked = false;
};

void mcp23017_output_init();
esp_err_t mcp23017_read_register(std::uint8_t address, std::uint8_t reg, std::uint8_t *value);
esp_err_t mcp23017_write_register_locked(std::uint8_t address, std::uint8_t reg, std::uint8_t value);
hardware::OutputBoardHealth &mcp23017_output_health();
