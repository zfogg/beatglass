#pragma once
#include <filesystem>
#include <optional>
namespace beatglass::library_detail {
/** Observe the filesystem path associated with an address in a loaded module.
 * Worker-thread diagnostic only; this allocates and can race module unload or
 * filesystem changes. The caller must keep the module containing address loaded
 * for the duration of the call. A null, unknown or unsupported address returns
 * no value. Paths may refer to a replaced file and do not prove loaded bytes,
 * transitive dependencies, signatures or configuration. Never use this result
 * to authorize cache reuse, publication, or other trust decisions.
 * @param address Address of an object within the loaded module, or null.
 * @return Observed module pathname, or empty when unavailable.
 */
std::optional<std::filesystem::path> loaded_module_path(const void* address);
}
