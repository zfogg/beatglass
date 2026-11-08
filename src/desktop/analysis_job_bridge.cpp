#include "analysis_job_bridge.hpp"
std::shared_ptr<beatglass::library_detail::JobHandle> AnalysisJobBridge::submit(beatglass::library::AnalysisOperation op){ auto progress=op.progress; op.progress=[this,progress](auto done,auto total){frames_=done; if(progress)progress(done,total); emit progressChanged();}; return runner_.submit(std::move(op)); }
