#include "beatglass/library/detail/file_snapshot.hpp"
#include <openssl/evp.h>
#include <array>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <limits>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <openssl/rand.h>
#else
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
  Fd original, snapshot; std::filesystem::path path; struct stat evidence{}; std::string digest; std::uint64_t size{};
};
FileSnapshot::FileSnapshot(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
FileSnapshot::~FileSnapshot() = default;
int FileSnapshot::borrowed_descriptor() const noexcept { return impl_->snapshot.value; }
SnapshotRead FileSnapshot::read_at(std::uint64_t offset, std::span<std::byte> buffer, std::stop_token stop) const {
  if (stop.stop_requested()) return {SnapshotStatus::cancelled, 0};
  if (offset > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max()) || buffer.empty()) return {SnapshotStatus::error, 0};
  auto n = ::pread(impl_->snapshot.value, buffer.data(), buffer.size(), static_cast<off_t>(offset));
  return n < 0 ? SnapshotRead{SnapshotStatus::error, 0} : SnapshotRead{SnapshotStatus::ok, static_cast<std::size_t>(n)};
}
std::uint64_t FileSnapshot::size() const noexcept { return impl_->size; }
std::optional<std::int64_t> FileSnapshot::modified_unix_ns() const noexcept {
#ifdef __APPLE__
  auto sec = impl_->evidence.st_mtimespec.tv_sec; auto ns = impl_->evidence.st_mtimespec.tv_nsec;
#else
  auto sec = impl_->evidence.st_mtim.tv_sec; auto ns = impl_->evidence.st_mtim.tv_nsec;
#endif
  constexpr std::int64_t scale = 1000000000;
  if (sec > std::numeric_limits<std::int64_t>::max() / scale || sec < std::numeric_limits<std::int64_t>::min() / scale || ns < 0 || ns >= scale) return std::nullopt;
  auto base = static_cast<std::int64_t>(sec) * scale;
  if (base > std::numeric_limits<std::int64_t>::max() - ns) return std::nullopt;
  return base + ns;
}
const std::string& FileSnapshot::digest() const noexcept { return impl_->digest; }
SnapshotStatus FileSnapshot::verify(std::stop_token stop, const std::function<void(std::uintmax_t)>& progress) const {
  try {
    if (stop.stop_requested()) return SnapshotStatus::cancelled;
    if (hash_fd(impl_->snapshot.value, stop) != impl_->digest) return SnapshotStatus::unverifiable;
    struct stat before{}, after{}, path_stat{};
    if (::fstat(impl_->original.value, &before) != 0) return SnapshotStatus::error;
    if (!same(before, impl_->evidence)) return SnapshotStatus::stale_input;
    auto digest = hash_fd(impl_->original.value, stop, -1, progress);
    if (::fstat(impl_->original.value, &after) != 0) return SnapshotStatus::error;
    if (::stat(impl_->path.c_str(), &path_stat) != 0) return errno == ENOENT ? SnapshotStatus::stale_input : SnapshotStatus::error;
    return same(before, after) && same(after, path_stat) && digest == impl_->digest ? SnapshotStatus::ok : SnapshotStatus::stale_input;
  } catch (const Cancelled&) { return SnapshotStatus::cancelled; } catch (...) { return SnapshotStatus::error; }
}
SnapshotOpen SnapshotOpen::create(const std::filesystem::path& path, std::stop_token stop, const std::function<void(std::uintmax_t)>& progress, SnapshotSourcePolicy policy) {
  try {
    if (stop.stop_requested()) return {SnapshotStatus::cancelled, {}, {}};
    if (policy != SnapshotSourcePolicy::detect_changes) return {SnapshotStatus::unverifiable, {}, "source sharing restriction unavailable"};
    auto impl = std::make_unique<FileSnapshot::Impl>(); impl->path = std::filesystem::absolute(path);
    impl->original.value = ::open(impl->path.c_str(), O_RDONLY | O_CLOEXEC);
    check(impl->original.value >= 0, "open source");
    struct stat before{}, after{}, named{};
    check(::fstat(impl->original.value, &before) == 0, "stat source");
    if (!S_ISREG(before.st_mode)) return {SnapshotStatus::unverifiable, {}, "source is not a regular file"};
    Fd writer;
#ifdef __linux__
    // Require an anonymous filesystem temporary: no named writable snapshot is exposed.
    writer.value = ::open(std::filesystem::temp_directory_path().c_str(), O_TMPFILE | O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (writer.value < 0) return {SnapshotStatus::unverifiable, {}, "anonymous temporary files unavailable"};
#else
    auto name = (std::filesystem::temp_directory_path() / "beatglass-snapshot-XXXXXX").string();
    std::vector<char> writable(name.begin(), name.end()); writable.push_back('\0');
    writer.value = ::mkstemp(writable.data()); check(writer.value >= 0, "create snapshot");
    TempName temporary{writable.data()};
    check(::fcntl(writer.value, F_SETFD, FD_CLOEXEC) == 0, "protect snapshot descriptor");
#endif
    auto copied_digest = hash_fd(impl->original.value, stop, writer.value, progress);
    check(::fstat(impl->original.value, &after) == 0, "restat source");
    check(::stat(impl->path.c_str(), &named) == 0, "stat source path");
    if (!same(before, after) || !same(after, named)) throw Stale{};
    check(::fchmod(writer.value, S_IRUSR) == 0, "restrict snapshot");
    struct stat written{}, readback{}; check(::fstat(writer.value, &written) == 0, "stat snapshot");
#ifdef __linux__
    auto descriptor_path = std::string("/proc/self/fd/") + std::to_string(writer.value);
    impl->snapshot.value = ::open(descriptor_path.c_str(), O_RDONLY | O_CLOEXEC);
    if (impl->snapshot.value < 0) return {SnapshotStatus::unverifiable, {}, "cannot reopen anonymous snapshot read-only"};
#else
    impl->snapshot.value = ::open(temporary.name.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    check(impl->snapshot.value >= 0, "open snapshot read-only");
#endif
    check(::fstat(impl->snapshot.value, &readback) == 0, "stat read-only snapshot");
    if (written.st_dev != readback.st_dev || written.st_ino != readback.st_ino) throw Stale{};
#ifndef __linux__
    check(::unlink(temporary.name.c_str()) == 0, "unlink snapshot"); temporary.name.clear();
#endif
    check(::close(writer.release()) == 0, "close snapshot writer");
    impl->digest = hash_fd(impl->snapshot.value, stop);
    if (impl->digest != copied_digest) throw Stale{};
    impl->size = static_cast<std::uint64_t>(after.st_size);
    impl->evidence = after;
    return {SnapshotStatus::ok, std::unique_ptr<FileSnapshot>(new FileSnapshot(std::move(impl))), {}};
  } catch (const Cancelled&) { return {SnapshotStatus::cancelled, {}, {}}; }
    catch (const Stale&) { return {SnapshotStatus::stale_input, {}, {}}; }
    catch (const std::exception& e) { return {SnapshotStatus::error, {}, e.what()}; }
}
#else
namespace {
struct Cancelled {};
struct Stale {};
struct WinHandle { HANDLE value{INVALID_HANDLE_VALUE}; ~WinHandle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); } HANDLE release() { auto old = value; value = INVALID_HANDLE_VALUE; return old; } };
void check_win(bool ok, const char* what) { if (!ok) throw std::runtime_error(std::string(what) + " (Windows error " + std::to_string(GetLastError()) + ")"); }
struct Evidence { FILE_ID_INFO identity{}; FILE_BASIC_INFO basic{}; FILE_STANDARD_INFO standard{}; };
Evidence evidence(HANDLE handle) {
  Evidence out; check_win(GetFileInformationByHandleEx(handle, FileIdInfo, &out.identity, sizeof(out.identity)) != 0, "file identity");
  check_win(GetFileInformationByHandleEx(handle, FileStandardInfo, &out.standard, sizeof(out.standard)) != 0, "file size");
  check_win(GetFileInformationByHandleEx(handle, FileBasicInfo, &out.basic, sizeof(out.basic)) != 0, "file attributes");
  if (out.standard.Directory || GetFileType(handle) != FILE_TYPE_DISK) throw std::runtime_error("source is not a regular disk file");
  return out;
}
bool same(const Evidence& a, const Evidence& b) {
  return a.identity.VolumeSerialNumber == b.identity.VolumeSerialNumber &&
    std::memcmp(a.identity.FileId.Identifier, b.identity.FileId.Identifier, sizeof(a.identity.FileId.Identifier)) == 0 &&
    a.standard.EndOfFile.QuadPart == b.standard.EndOfFile.QuadPart &&
    a.basic.LastWriteTime.QuadPart == b.basic.LastWriteTime.QuadPart && a.basic.ChangeTime.QuadPart == b.basic.ChangeTime.QuadPart;
}
std::string hash_handle(HANDLE input, std::stop_token stop, HANDLE output = INVALID_HANDLE_VALUE,
                        const std::function<void(std::uintmax_t)>& progress = {}) {
  using Context = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
  Context ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
  if (!ctx || EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) throw std::runtime_error("SHA-256 init failed");
  LARGE_INTEGER zero{}; check_win(SetFilePointerEx(input, zero, nullptr, FILE_BEGIN) != 0, "seek source");
  std::array<unsigned char, 65536> bytes{}; std::uintmax_t total = 0;
  for (;;) {
    if (stop.stop_requested()) throw Cancelled{};
    DWORD n = 0; check_win(ReadFile(input, bytes.data(), static_cast<DWORD>(bytes.size()), &n, nullptr) != 0, "read source");
    if (n == 0) break;
    if (EVP_DigestUpdate(ctx.get(), bytes.data(), n) != 1) throw std::runtime_error("SHA-256 update failed");
    if (output != INVALID_HANDLE_VALUE) {
      DWORD offset = 0;
      while (offset < n) { DWORD written = 0; check_win(WriteFile(output, bytes.data() + offset, n - offset, &written, nullptr) != 0 && written > 0, "write snapshot"); offset += written; }
    }
    total += n; if (progress) progress(total);
  }
  if (stop.stop_requested()) throw Cancelled{};
  std::array<unsigned char, EVP_MAX_MD_SIZE> hash{}; unsigned length = 0;
  if (EVP_DigestFinal_ex(ctx.get(), hash.data(), &length) != 1 || length != 32) throw std::runtime_error("SHA-256 finalization failed");
  static constexpr char hex[] = "0123456789abcdef"; std::string out = "sha256:";
  for (unsigned i = 0; i < length; ++i) { out.push_back(hex[hash[i] >> 4]); out.push_back(hex[hash[i] & 15]); }
  return out;
}
}
struct FileSnapshot::Impl { WinHandle original, snapshot; std::filesystem::path path; Evidence evidence{}; std::string digest; std::uint64_t size{}; };
FileSnapshot::FileSnapshot(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
FileSnapshot::~FileSnapshot() = default;
int FileSnapshot::borrowed_descriptor() const noexcept { return -1; }
SnapshotRead FileSnapshot::read_at(std::uint64_t offset, std::span<std::byte> buffer, std::stop_token stop) const {
  if (stop.stop_requested()) return {SnapshotStatus::cancelled, 0};
  if (buffer.empty() || offset > static_cast<std::uint64_t>(std::numeric_limits<LONGLONG>::max())) return {SnapshotStatus::error, 0};
  LARGE_INTEGER pos{}; pos.QuadPart = static_cast<LONGLONG>(offset);
  if (!SetFilePointerEx(impl_->snapshot.value, pos, nullptr, FILE_BEGIN)) return {SnapshotStatus::error, 0};
  DWORD read = 0; DWORD count = static_cast<DWORD>((std::min)(buffer.size(), static_cast<std::size_t>(MAXDWORD)));
  if (!ReadFile(impl_->snapshot.value, buffer.data(), count, &read, nullptr)) return {SnapshotStatus::error, 0};
  return {SnapshotStatus::ok, static_cast<std::size_t>(read)};
}
std::uint64_t FileSnapshot::size() const noexcept { return impl_->size; }
std::optional<std::int64_t> FileSnapshot::modified_unix_ns() const noexcept {
  constexpr std::int64_t epoch_ticks = 116444736000000000LL;
  auto ticks = impl_->evidence.basic.LastWriteTime.QuadPart;
  if (ticks < 0) return std::nullopt;
  auto unix_ticks = ticks - epoch_ticks;
  if (unix_ticks > std::numeric_limits<std::int64_t>::max() / 100 || unix_ticks < std::numeric_limits<std::int64_t>::min() / 100) return std::nullopt;
  return unix_ticks * 100;
}
const std::string& FileSnapshot::digest() const noexcept { return impl_->digest; }
SnapshotStatus FileSnapshot::verify(std::stop_token stop, const std::function<void(std::uintmax_t)>& progress) const {
  try {
    if (stop.stop_requested()) return SnapshotStatus::cancelled;
    if (hash_handle(impl_->snapshot.value, stop) != impl_->digest) return SnapshotStatus::unverifiable;
    auto before = evidence(impl_->original.value); if (!same(before, impl_->evidence)) return SnapshotStatus::stale_input;
    auto digest = hash_handle(impl_->original.value, stop, INVALID_HANDLE_VALUE, progress); auto after = evidence(impl_->original.value);
    WinHandle named; named.value = CreateFileW(impl_->path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (named.value == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND ? SnapshotStatus::stale_input : SnapshotStatus::error;
    return same(before, after) && same(after, evidence(named.value)) && digest == impl_->digest ? SnapshotStatus::ok : SnapshotStatus::stale_input;
  } catch (const Cancelled&) { return SnapshotStatus::cancelled; } catch (...) { return SnapshotStatus::error; }
}
SnapshotOpen SnapshotOpen::create(const std::filesystem::path& path, std::stop_token stop, const std::function<void(std::uintmax_t)>& progress, SnapshotSourcePolicy policy) {
  try {
    if (stop.stop_requested()) return {SnapshotStatus::cancelled, {}, {}};
    if (policy != SnapshotSourcePolicy::detect_changes && policy != SnapshotSourcePolicy::deny_writes) return {SnapshotStatus::unverifiable, {}, "invalid source policy"};
    auto impl = std::make_unique<FileSnapshot::Impl>(); impl->path = std::filesystem::absolute(path);
    const DWORD sharing = policy == SnapshotSourcePolicy::deny_writes ? FILE_SHARE_READ : FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    impl->original.value = CreateFileW(impl->path.c_str(), GENERIC_READ, sharing, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (impl->original.value == INVALID_HANDLE_VALUE && policy == SnapshotSourcePolicy::deny_writes && GetLastError() == ERROR_SHARING_VIOLATION) return {SnapshotStatus::unverifiable, {}, "source is already open for write or delete"};
    check_win(impl->original.value != INVALID_HANDLE_VALUE, "open source");
    auto before = evidence(impl->original.value); WinHandle writer;
    for (int attempt = 0; attempt < 16 && writer.value == INVALID_HANDLE_VALUE; ++attempt) {
      std::array<unsigned char, 16> random{}; if (RAND_bytes(random.data(), static_cast<int>(random.size())) != 1) throw std::runtime_error("random snapshot name failed");
      static constexpr wchar_t digits[] = L"0123456789abcdef"; std::wstring name = L"beatglass-snapshot-" + std::to_wstring(GetCurrentProcessId()) + L"-";
      for (auto byte : random) { name.push_back(digits[byte >> 4]); name.push_back(digits[byte & 15]); }
      auto temporary = std::filesystem::temp_directory_path() / name;
      writer.value = CreateFileW(temporary.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
      if (writer.value == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS) check_win(false, "create snapshot");
    }
    check_win(writer.value != INVALID_HANDLE_VALUE, "create snapshot");
    auto copied = hash_handle(impl->original.value, stop, writer.value, progress); auto after = evidence(impl->original.value);
    WinHandle named; named.value = CreateFileW(impl->path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (named.value == INVALID_HANDLE_VALUE || !same(before, after) || !same(after, evidence(named.value))) throw Stale{};
    HANDLE reader = INVALID_HANDLE_VALUE;
    check_win(DuplicateHandle(GetCurrentProcess(), writer.value, GetCurrentProcess(), &reader, FILE_GENERIC_READ, FALSE, 0) != 0, "duplicate read-only snapshot");
    impl->snapshot.value = reader; check_win(CloseHandle(writer.release()) != 0, "close snapshot writer");
    impl->digest = hash_handle(impl->snapshot.value, stop);
    if (impl->digest != copied) throw Stale{};
    impl->size = static_cast<std::uint64_t>(after.standard.EndOfFile.QuadPart);
    impl->evidence = after;
    return {SnapshotStatus::ok, std::unique_ptr<FileSnapshot>(new FileSnapshot(std::move(impl))), {}};
  } catch (const Cancelled&) { return {SnapshotStatus::cancelled, {}, {}}; }
    catch (const Stale&) { return {SnapshotStatus::stale_input, {}, {}}; }
    catch (const std::exception& e) { return {SnapshotStatus::error, {}, e.what()}; }
}
#endif
}
