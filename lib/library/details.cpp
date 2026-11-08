#include "beatglass/library/details.hpp"
#include <algorithm>
#include <cctype>
namespace beatglass::library { namespace {
std::string lower(std::string s){ std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));}); return s; }
bool audio(const std::filesystem::path& p){ const auto e=lower(p.extension().string()); return e==".wav"||e==".flac"||e==".mp3"||e==".ogg"||e==".aiff"||e==".aif"||e==".m4a"; }
bool decodable(const std::string& e){ return e==".wav"||e==".flac"||e==".mp3"||e==".aiff"||e==".aif"; }
}
std::optional<FileDetails> inspect_file(const std::filesystem::path& path){ std::error_code ec; if(!std::filesystem::is_regular_file(path,ec)||ec||!audio(path)) return std::nullopt; const auto t=std::filesystem::last_write_time(path,ec); if(ec)return std::nullopt; const auto extension=lower(path.extension().string()); const auto size=std::filesystem::file_size(path,ec); if(ec)return std::nullopt; return FileDetails{path,path.filename().string(),extension,size,static_cast<std::int64_t>(t.time_since_epoch().count()),decodable(extension)}; }
}
