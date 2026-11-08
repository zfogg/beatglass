#pragma once
#include "beatglass/capture/file_reader.hpp"
#include <cstddef>
#include <functional>
#include <span>
#include <stop_token>
namespace beatglass {
/** Terminal result of a worker-owned decoded PCM pump. */
enum class DecodedPcmPumpStatus { end, cancelled };
/**
 * Pull decoded frames from a FileReader and synchronously hand each borrowed
 * block to an output sink. The pump is deliberately backend-neutral: a
 * platform output adapter may copy the block into its own bounded queue, but
 * must not retain the borrowed span after the callback returns. This function
 * performs blocking decoder I/O and must run on a playback worker, never on a
 * GUI or native audio callback thread.
 *
 * @param reader Worker-owned reader to consume.
 * @param buffer Caller-owned interleaved sample storage; reused each pull.
 * @param sink Called once for every nonempty decoded block.
 * @param cancel Cancellation checked before each pull.
 * @return End when the reader reaches EOF, or cancelled when cancellation is
 * observed before a pull.
 * @throws std::invalid_argument for an empty or non-frame-aligned buffer.
 * @throws any exception raised by FileReader or sink; the caller must discard
 * the reader/output session after decoder or sink failure.
 */
DecodedPcmPumpStatus pump_decoded_pcm(FileReader& reader, std::span<float> buffer,
                                      const std::function<void(const AudioBlock&)>& sink,
                                      std::stop_token cancel = {});
}
