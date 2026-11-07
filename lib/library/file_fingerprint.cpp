#include "beatglass/library/detail/file_fingerprint.hpp"
#include <openssl/evp.h>
#include <array>
#include <fstream>
#include <memory>
namespace beatglass::library_detail {
FileFingerprint fingerprint_file(const std::filesystem::path& path, std::stop_token stop,
                                 const std::function<void(std::uintmax_t)>& observer) {
  FileFingerprint out;
  try {
    if (stop.stop_requested()) { out.status = FingerprintStatus::cancelled; return out; }
    const auto before_size = std::filesystem::file_size(path);
    const auto before_time = std::filesystem::last_write_time(path);
    std::ifstream input(path, std::ios::binary);
    if (!input) { out.diagnostic = "cannot open input"; return out; }
    using Context = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
    Context context(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!context || EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1) { out.diagnostic = "digest initialization failed"; return out; }
    std::array<char, 65536> buffer{};
    std::uintmax_t count = 0;
    while (input) {
      if (stop.stop_requested()) { out.status = FingerprintStatus::cancelled; return out; }
      input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      const auto n = input.gcount();
      if (n > 0) {
        const auto bytes_read = static_cast<std::uintmax_t>(n);
        // count never exceeds before_size, so subtraction and addition cannot wrap.
        if (bytes_read > before_size - count) { out.status = FingerprintStatus::changed; return out; }
        if (EVP_DigestUpdate(context.get(), buffer.data(), static_cast<std::size_t>(n)) != 1) { out.diagnostic = "digest update failed"; return out; }
        count += bytes_read;
        if (observer) observer(count);
      }
    }
    if (input.bad() || !input.eof()) { out.diagnostic = "file read failed"; return out; }
    if (stop.stop_requested()) { out.status = FingerprintStatus::cancelled; return out; }
    if (count != before_size || std::filesystem::file_size(path) != before_size || std::filesystem::last_write_time(path) != before_time) {
      out.status = FingerprintStatus::changed; return out;
    }
    std::array<unsigned char, EVP_MAX_MD_SIZE> bytes{}; unsigned length = 0;
    if (EVP_DigestFinal_ex(context.get(), bytes.data(), &length) != 1 || length != 32) { out.diagnostic = "digest finalization failed"; return out; }
    static constexpr char hex[] = "0123456789abcdef";
    out.digest = "sha256:";
    for (unsigned i = 0; i < length; ++i) { out.digest.push_back(hex[bytes[i] >> 4]); out.digest.push_back(hex[bytes[i] & 15]); }
    out.size = count; out.modified = before_time; out.status = FingerprintStatus::ok;
  } catch (const std::exception& e) { out.status = FingerprintStatus::error; out.diagnostic = e.what(); }
  return out;
}
}
