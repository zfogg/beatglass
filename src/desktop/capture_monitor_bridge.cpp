#include "capture_monitor_bridge.hpp"
QString CaptureMonitorBridge::status() const { switch(lifecycle_.state()){case beatglass::SourceState::starting:return "Starting";case beatglass::SourceState::running:return "Monitoring";case beatglass::SourceState::interrupted:return "Reacquiring";case beatglass::SourceState::error:return "Error";default:return "Stopped";} }
bool CaptureMonitorBridge::start(){const bool ok=lifecycle_.start().has_value();if(ok)emit changed();return ok;} void CaptureMonitorBridge::stop(){lifecycle_.stop();emit changed();}
