#include "beatglass/library/detail/file_snapshot.hpp"
#include <openssl/evp.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <limits>
#include <vector>
#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace beatglass::library_detail {
#ifndef _WIN32
namespace {
struct Cancelled {};
struct Stale {};
struct Fd { int value{-1}; ~Fd() { if (value >= 0) ::close(value); } int release() { int old = value; value = -1; return old; } };
struct TempName { std::string name; ~TempName() { if (!name.empty()) ::unlink(name.c_str()); } };
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(std::string(message) + ": " + std::strerror(errno)); }
bool same(const struct stat& a, const struct stat& b) {
#ifdef __APPLE__
  const bool times = a.st_mtimespec.tv_sec == b.st_mtimespec.tv_sec && a.st_mtimespec.tv_nsec == b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_sec == b.st_ctimespec.tv_sec && a.st_ctimespec.tv_nsec == b.st_ctimespec.tv_nsec;
#else
  const bool times = a.st_mtim.tv_sec == b.st_mtim.tv_sec && a.st_mtim.tv_nsec == b.st_mtim.tv_nsec && a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
#endif
  return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_size == b.st_size && times;
}
std::string hash_fd(int fd, std::stop_token stop, int output = -1, const std::function<void(std::uintmax_t)>& progress = {}) {
  using Context = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
  Context ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
  if (!ctx || EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) throw std::runtime_error("SHA-256 init failed");
  std::array<unsigned char, 65536> buffer{}; off_t offset = 0;
  for (;;) {
    if (stop.stop_requested()) throw Cancelled{};
    const auto n = ::pread(fd, buffer.data(), buffer.size(), offset);
    if (n < 0 && errno == EINTR) continue;
    check(n >= 0, "read"); if (n == 0) break;
    if (EVP_DigestUpdate(ctx.get(), buffer.data(), static_cast<std::size_t>(n)) != 1) throw std::runtime_error("SHA-256 update failed");
    if (output >= 0) {
      ssize_t written = 0;
      while (written < n) { auto count = ::write(output, buffer.data() + written, static_cast<std::size_t>(n - written)); if (count < 0 && errno == EINTR) continue; check(count > 0, "snapshot write"); written += count; }
    }
    if (offset > std::numeric_limits<off_t>::max() - n) throw std::overflow_error("file too large");
    offset += n;
    if (progress) progress(static_cast<std::uintmax_t>(offset));
  }
  if (stop.stop_requested()) throw Cancelled{};
  std::array<unsigned char, EVP_MAX_MD_SIZE> hash{}; unsigned length = 0;
  if (EVP_DigestFinal_ex(ctx.get(), hash.data(), &length) != 1 || length != 32) throw std::runtime_error("SHA-256 finalization failed");
  static constexpr char hex[] = "0123456789abcdef"; std::string out = "sha256:";
  for (unsigned i = 0; i < length; ++i) { out.push_back(hex[hash[i] >> 4]); out.push_back(hex[hash[i] & 15]); }
  return out;
}
}
struct FileSnapshot::Impl {
  Fd original, snapshot; std::filesystem::path path; struct stat evidence{}; std::string digest;
};
FileSnapshot::FileSnapshot(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
FileSnapshot::~FileSnapshot() = default;
int FileSnapshot::borrowed_descriptor() const noexcept { return impl_->snapshot.value; }
const std::string& FileSnapshot::digest() const noexcept { return impl_->digest; }
SnapshotStatus FileSnapshot::verify(std::stop_token stop) const {
  try {
    if (stop.stop_requested()) return SnapshotStatus::cancelled;
    struct stat before{}, after{}, path_stat{};
    if (::fstat(impl_->original.value, &before) != 0) return SnapshotStatus::error;
    if (!same(before, impl_->evidence)) return SnapshotStatus::stale_input;
    auto digest = hash_fd(impl_->original.value, stop);
    if (::fstat(impl_->original.value, &after) != 0) return SnapshotStatus::error;
    if (::stat(impl_->path.c_str(), &path_stat) != 0) return errno == ENOENT ? SnapshotStatus::stale_input : SnapshotStatus::error;
    return same(before, after) && same(after, path_stat) && digest == impl_->digest ? SnapshotStatus::ok : SnapshotStatus::stale_input;
  } catch (const Cancelled&) { return SnapshotStatus::cancelled; } catch (...) { return SnapshotStatus::error; }
}
SnapshotOpen SnapshotOpen::create(const std::filesystem::path& path, std::stop_token stop, const std::function<void(std::uintmax_t)>& progress) {
  try {
    if (stop.stop_requested()) return {SnapshotStatus::cancelled, {}, {}};
    auto impl = std::make_unique<FileSnapshot::Impl>(); impl->path = std::filesystem::absolute(path);
    impl->original.value = ::open(impl->path.c_str(), O_RDONLY | O_CLOEXEC);
    check(impl->original.value >= 0, "open source");
    struct stat before{}, after{}, named{};
    check(::fstat(impl->original.value, &before) == 0, "stat source");
    if (!S_ISREG(before.st_mode)) return {SnapshotStatus::unverifiable, {}, "source is not a regular file"};
    auto name = (std::filesystem::temp_directory_path() / "beatglass-snapshot-XXXXXX").string();
    std::vector<char> writable(name.begin(), name.end()); writable.push_back('\0');
    Fd writer; writer.value = ::mkstemp(writable.data()); check(writer.value >= 0, "create snapshot");
    TempName temporary{writable.data()};
    check(::fcntl(writer.value, F_SETFD, FD_CLOEXEC) == 0, "protect snapshot descriptor");
    auto copied_digest = hash_fd(impl->original.value, stop, writer.value, progress);
    check(::fstat(impl->original.value, &after) == 0, "restat source");
    check(::stat(impl->path.c_str(), &named) == 0, "stat source path");
    if (!same(before, after) || !same(after, named)) throw Stale{};
    check(::fchmod(writer.value, S_IRUSR) == 0, "restrict snapshot");
    struct stat written{}, readback{}; check(::fstat(writer.value, &written) == 0, "stat snapshot");
    impl->snapshot.value = ::open(temporary.name.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    check(impl->snapshot.value >= 0, "open snapshot read-only");
    check(::fstat(impl->snapshot.value, &readback) == 0, "stat read-only snapshot");
    if (written.st_dev != readback.st_dev || written.st_ino != readback.st_ino) throw Stale{};
    check(::unlink(temporary.name.c_str()) == 0, "unlink snapshot"); temporary.name.clear();
    check(::close(writer.release()) == 0, "close snapshot writer");
    impl->digest = hash_fd(impl->snapshot.value, stop);
    if (impl->digest != copied_digest) throw Stale{};
    impl->evidence = after;
    return {SnapshotStatus::ok, std::unique_ptr<FileSnapshot>(new FileSnapshot(std::move(impl))), {}};
  } catch (const Cancelled&) { return {SnapshotStatus::cancelled, {}, {}}; }
    catch (const Stale&) { return {SnapshotStatus::stale_input, {}, {}}; }
    catch (const std::exception& e) { return {SnapshotStatus::error, {}, e.what()}; }
}
#else
struct FileSnapshot::Impl {};
FileSnapshot::FileSnapshot(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
FileSnapshot::~FileSnapshot() = default;
int FileSnapshot::borrowed_descriptor() const noexcept { return -1; }
const std::string& FileSnapshot::digest() const noexcept { static const std::string empty; return empty; }
SnapshotStatus FileSnapshot::verify(std::stop_token) const { return SnapshotStatus::unverifiable; }
SnapshotOpen SnapshotOpen::create(const std::filesystem::path&, std::stop_token, const std::function<void(std::uintmax_t)>&) { return {SnapshotStatus::unverifiable, {}, "snapshot unsupported on Windows"}; }
#endif
}
