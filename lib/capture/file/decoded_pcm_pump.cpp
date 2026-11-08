#include "beatglass/capture/decoded_pcm_pump.hpp"
#include <stdexcept>
namespace beatglass {
DecodedPcmPumpStatus pump_decoded_pcm(FileReader& reader, std::span<float> buffer,
                                      const std::function<void(const AudioBlock&)>& sink,
                                      std::stop_token cancel) {
  if (buffer.empty() || !sink) throw std::invalid_argument("PCM pump requires storage and sink");
  for (;;) {
    if (cancel.stop_requested()) return DecodedPcmPumpStatus::cancelled;
    const auto result = reader.read(buffer, cancel);
    if (result.status == FileReadStatus::cancelled) return DecodedPcmPumpStatus::cancelled;
    if (result.status == FileReadStatus::end) return DecodedPcmPumpStatus::end;
    if (result.block.samples.empty()) throw std::runtime_error("Decoder returned an empty PCM block");
    sink(result.block);
  }
}
}
