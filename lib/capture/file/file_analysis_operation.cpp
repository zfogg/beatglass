#include "beatglass/library/file_analysis_operation.hpp"
#include "beatglass/capture/file_reader.hpp"
#include <array>
#include <sstream>
namespace beatglass::library {
bool file_decoder_snapshot_compatible(const std::string& snapshot) noexcept {
  return snapshot == kFileDecoderSnapshot || snapshot == kCliDecoderSnapshot;
}
AnalysisOperation make_file_decode_operation(std::filesystem::path path, AnalysisIdentity identity, std::string decoder_snapshot) {
  AnalysisOperation op; op.source.path = path; op.source.content_hash = identity.content_hash; op.identity = std::move(identity); op.decoder_snapshot = std::move(decoder_snapshot);
  op.verify = [path](std::stop_token stop) {
    if (stop.stop_requested()) return FileVerificationResult{FileVerificationOutcome::cancelled};
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec) return FileVerificationResult{FileVerificationOutcome::unverifiable, "source file is unavailable"};
    return FileVerificationResult{FileVerificationOutcome::match};
  };
  op.decode = [path](std::stop_token stop, const auto& progress) {
    FileReader reader(path); std::array<float, 4096> buffer{}; std::uint64_t frames = 0;
    for (;;) { auto read = reader.read(buffer, stop); if (read.status == FileReadStatus::end) break; if (read.status == FileReadStatus::cancelled) return std::string{}; if (read.block.format.channels == 0) return std::string{}; frames += read.block.samples.size() / read.block.format.channels; progress(frames); }
    std::ostringstream out; out << "decoded_frames=" << frames; return out.str();
  };
  return op;
}
}
