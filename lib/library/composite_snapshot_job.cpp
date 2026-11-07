#include "beatglass/library/detail/composite_snapshot_job.hpp"
#include "beatglass/library/detail/file_snapshot.hpp"
#include "beatglass/capture/byte_source.hpp"
#include "../capture/file/virtual_io.hpp"
#include <algorithm>
#include <vector>
namespace beatglass::library_detail {
namespace {
class Source final : public ByteSource {
public:
  explicit Source(FileSnapshot& source) : source_(source) {}
  std::uint64_t size() const override { return source_.size(); }
  ByteReadResult read_at(std::uint64_t offset, std::span<std::byte> output, std::stop_token stop) override {
    if (stop.stop_requested()) return {ByteReadStatus::cancelled,0};
    if (output.empty()) return {ByteReadStatus::error,0};
    if (offset>=size()) return {ByteReadStatus::end,0};
    const auto length=static_cast<std::size_t>((std::min)(static_cast<std::uint64_t>(output.size()),size()-offset));
    auto r=source_.read_at(offset,output.first(length),stop);
    if (r.status==SnapshotStatus::cancelled) return {ByteReadStatus::cancelled,0};
    if (r.status!=SnapshotStatus::ok || !r.bytes || r.bytes>length) return {ByteReadStatus::error,0};
    return {ByteReadStatus::data,r.bytes};
  }
private:
  FileSnapshot& source_;
};
CompositeJobResult failure(SnapshotStatus status, std::string diagnostic={}) {
  if (status==SnapshotStatus::cancelled) return {CompositeJobStatus::cancelled,{},std::move(diagnostic)};
  if (status==SnapshotStatus::stale_input || status==SnapshotStatus::unverifiable) return {CompositeJobStatus::stale_input,{},std::move(diagnostic)};
  return {CompositeJobStatus::error,{},std::move(diagnostic)};
}
}
CompositeJobResult analyze_snapshot_music(const std::filesystem::path& path, std::uint64_t epoch,
  MusicCollectorLimits limits, std::stop_token stop, const std::function<void(std::uint64_t)>& progress) {
  try {
    if (!epoch) return {CompositeJobStatus::error,{},"epoch must be nonzero"};
    auto opened=SnapshotOpen::create(path,stop);
    if (opened.status!=SnapshotStatus::ok) return failure(opened.status,opened.diagnostic);
    MusicCollector collector(epoch,limits);
    {
      Source source(*opened.value);
      detail::VirtualFileReader file(source,epoch,stop);
      std::vector<float> pcm(512*file.metadata().channels);
      for (;;) {
        auto read=file.read(pcm,stop);
        if (read.status==FileReadStatus::cancelled) return {CompositeJobStatus::cancelled,{},{}};
        if (read.status==FileReadStatus::end) break;
        collector.process(read.block);
        if (progress) progress(read.block.first_frame+read.block.samples.size()/read.block.format.channels);
        if (stop.stop_requested()) return {CompositeJobStatus::cancelled,{},{}};
      }
    }
    if (stop.stop_requested()) return {CompositeJobStatus::cancelled,{},{}};
    auto verified=opened.value->verify(stop);
    if (verified!=SnapshotStatus::ok) return failure(verified,"source revalidation failed");
    if (stop.stop_requested()) return {CompositeJobStatus::cancelled,{},{}};
    return {CompositeJobStatus::complete,collector.finish(),{}};
  } catch (const FileOpenCancelled&) { return {CompositeJobStatus::cancelled,{},{}}; }
    catch (const std::exception& e) { return {CompositeJobStatus::error,{},e.what()}; }
}
}
