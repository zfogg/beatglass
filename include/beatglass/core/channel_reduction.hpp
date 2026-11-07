#pragma once
#include <cstddef>
#include <span>
namespace beatglass {
/**
 * Reduce interleaved float PCM to mono in caller-owned storage.
 *
 * Selects the channel with greatest block sum-square energy; the lowest index
 * wins ties. This avoids anti-phase cancellation, but switching channels
 * between blocks can produce artifacts. It does not resample or clip. The
 * caller supplies non-overlapping input and output spans that remain valid
 * for the call. Work belongs on an analysis worker, not a capture callback.
 * @param input Interleaved frames in source channel order.
 * @param channels Positive number of input channels.
 * @param output Destination with exactly one element per input frame.
 * @return False without modifying output for malformed sizes, zero channels,
 *         or non-finite input samples; true after copying otherwise.
 */
bool reduce_loudest_channel(std::span<const float> input, std::size_t channels, std::span<float> output) noexcept;
}
