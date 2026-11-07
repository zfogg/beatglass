#include "beatglass/capture/coreaudio_internal.hpp"
#include "beatglass/capture/coreaudio_source_internal.hpp"
#import <CoreAudio/CoreAudio.h>
#import <CoreAudio/HostTime.h>
#include <limits>
#include <memory>
#include <utility>

namespace beatglass::coreaudio {
namespace {
class NativeBackend final : public SourceBackend {
public:
  EnumerationResult enumerate() override {
    std::string error;
    auto sources = enumerate_outputs(error);
    if (!error.empty()) return {EnumerationOutcome::error, {}, SourceError::backend_error, error};
    return {EnumerationOutcome::ok, std::move(sources), {}, {}};
  }
  bool start(const std::string& uid, std::uint64_t epoch) override { return tap_.start(uid, epoch); }
  std::optional<AudioFormat> format() const override { return tap_.format(); }
  TapStatus status() override { return tap_.status(); }
  bool pop(CaptureBlock& block) override { return tap_.pop(block); }
  bool stop() noexcept override { return tap_.stop(); }
  std::optional<std::int64_t> host_time_ns(std::uint64_t ticks) const noexcept override {
    const auto ns = AudioConvertHostTimeToNanos(ticks);
    if (ns > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) return std::nullopt;
    return static_cast<std::int64_t>(ns);
  }
private:
  OutputTap tap_;
};
}
std::unique_ptr<SourceBackend> make_native_backend() { return std::make_unique<NativeBackend>(); }
}
