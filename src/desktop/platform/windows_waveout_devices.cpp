#ifdef _WIN32
#include "windows_waveout_devices.hpp"
#include <windows.h>
namespace beatglass::windows {
std::vector<WaveOutDevice> enumerate_waveout_devices(){ std::vector<WaveOutDevice> result; const auto count=waveOutGetNumDevs(); result.reserve(count); for(UINT i=0;i<count;++i){WAVEOUTCAPS caps{}; if(waveOutGetDevCaps(i,&caps,sizeof(caps))!=MMSYSERR_NOERROR)continue; const auto* p=caps.szPname; if(!p||!*p)continue; result.push_back({i,std::string(p)});} return result; }
}
#endif
