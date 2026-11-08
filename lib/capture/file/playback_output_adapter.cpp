#include "beatglass/capture/playback_output_adapter.hpp"
namespace beatglass {
PlaybackOutputAdapter::PlaybackOutputAdapter(std::size_t s, std::size_t n):queue_(s,n){}
bool PlaybackOutputAdapter::render_one(std::span<float> storage,const DeviceWriter& writer) noexcept { if(!writer)return false; AudioBlock b{}; if(!queue_.pop(storage,b))return false; try{writer(b);}catch(...){return false;} return true; }
}
