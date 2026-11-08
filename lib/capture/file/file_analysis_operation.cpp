#include "beatglass/library/file_analysis_operation.hpp"
#include "beatglass/capture/file_reader.hpp"
#include <array>
#include <sstream>
namespace beatglass::library {
AnalysisOperation make_file_decode_operation(std::filesystem::path path, AnalysisIdentity identity) {
  AnalysisOperation op; op.source.path = path; op.source.content_hash = identity.content_hash; op.identity = std::move(identity);
  op.verify = [](std::stop_token stop) { return FileVerificationResult{stop.stop_requested() ? FileVerificationOutcome::cancelled : FileVerificationOutcome::match}; };
  op.serialize = [] { return std::string("decode operation requires worker execution"); };
  op.decode = [path](std::stop_token stop, const auto& progress) {
    FileReader reader(path); std::array<float, 4096> buffer{}; std::uint64_t frames = 0;
    for (;;) { auto read = reader.read(buffer, stop); if (read.status == FileReadStatus::end) break; if (read.status == FileReadStatus::cancelled) return std::string{}; if (read.block.format.channels == 0) return std::string{}; frames += read.block.samples.size() / read.block.format.channels; progress(frames); }
    std::ostringstream out; out << "decoded_frames=" << frames; return out.str();
  };
  return op;
}
}
