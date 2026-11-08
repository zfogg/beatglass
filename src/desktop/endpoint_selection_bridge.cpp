#include "endpoint_selection_bridge.hpp"
#ifdef _WIN32
#include "platform/windows_endpoints.hpp"
#endif
void EndpointSelectionBridge::refresh(){
#ifdef _WIN32
 const auto result=beatglass::windows::enumerate_render_endpoints(); endpoints_.clear(); for(const auto& e:result.endpoints) endpoints_.push_back(QString::fromStdString(e.name)); status_=result.error.empty()?QStringLiteral("Ready"):QString::fromStdString(result.error);
#else
 endpoints_.clear(); status_=QStringLiteral("Output selection is unavailable on this platform");
#endif
 emit changed();
}
