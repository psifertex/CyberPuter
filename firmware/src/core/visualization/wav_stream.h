#pragma once
#include <cstddef>
#include <cstdint>

namespace Visualization {
struct WavReader {
    virtual ~WavReader() = default;
    virtual uint32_t size() const = 0;
    virtual bool readAt(uint32_t offset, uint8_t* out, size_t bytes) = 0;
};
struct WavInfo { uint32_t rate=0, offset=0, bytes=0; };
// Bounded RIFF parser: mono signed 16-bit PCM at 8–44.1 kHz. Skips metadata
// chunks, rejects truncated/overflowing files. Never scans sample data.
bool readPcmWav(WavReader& reader, WavInfo& out);
} // namespace Visualization
