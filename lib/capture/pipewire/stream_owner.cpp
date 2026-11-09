#include "beatglass/capture/detail/pipewire_stream_owner.hpp"
namespace beatglass::pipewire::detail {
std::unique_ptr<StreamOwner> make_stream_owner() { return {}; }
}
