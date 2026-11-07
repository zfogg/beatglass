#pragma once

#include "beatglass/capture/source.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace beatglass::coreaudio {
/** Internal, provisional Core Audio discovery interface; not installed or stable. All calls run on the owning control thread and may allocate. */
/** Enumerate a point-in-time output-device list. On failure return an empty list and set error; a successful empty list clears error. Unreadable or disappearing devices are omitted. */
std::vector<SourceInfo> enumerate_outputs(std::string& error);

namespace detail {
/** Internal test seam for reading a volatile device list. The callback receives a byte count and either null for a size query or storage of that size. It updates the byte count as Core Audio does and returns zero on success. The callback and buffers are used synchronously on the caller's control thread. */
using DeviceListQuery = std::function<int(std::uint32_t&, std::uint32_t*)>;
/** Query and read the list, retrying size/read races at most three times. Return false on persistent failure or malformed sizes; no partial data is returned. */
bool read_device_list(const DeviceListQuery& query, std::vector<std::uint32_t>& result, int& status);
}
}
