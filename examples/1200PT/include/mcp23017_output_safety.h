#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#ifndef RELAY_ACTIVE_LOW
#define RELAY_ACTIVE_LOW 0
#endif

namespace hardware
{
inline std::uint8_t relay_output_byte(std::uint8_t logicalOutputs, bool activeLow = RELAY_ACTIVE_LOW != 0)
{
    return activeLow ? static_cast<std::uint8_t>(~logicalOutputs) : logicalOutputs;
}

class OutputBoardHealth
{
public:
    OutputBoardHealth()
    {
        boardFound[0].store(false);
        boardFound[1].store(false);
    }

    void record_probe(std::size_t boardIndex, bool found)
    {
        if (boardIndex >= 2) return;
        boardFound[boardIndex].store(found);
        if (!found) faulted.store(true);
    }

    void record_write_failure()
    {
        faulted.store(true);
    }

    void record_write(bool succeeded)
    {
        if (!succeeded) record_write_failure();
    }

    bool board_found(std::size_t boardIndex) const
    {
        return boardIndex < 2 && boardFound[boardIndex].load();
    }

    bool has_fault() const
    {
        return faulted.load();
    }

private:
    std::atomic<bool> boardFound[2];
    std::atomic<bool> faulted{false};
};
} // namespace hardware
