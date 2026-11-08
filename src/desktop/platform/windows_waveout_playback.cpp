#ifdef _WIN32
#include "windows_waveout_playback.hpp"
#include <algorithm>
#include <string>
namespace beatglass::windows {
WaveOutPlayback::~WaveOutPlayback(){ stop(); }
void CALLBACK WaveOutPlayback::callback(HWAVEOUT, UINT msg, DWORD_PTR instance, DWORD_PTR p1, DWORD_PTR){ if(msg==WOM_DONE){ auto* b=reinterpret_cast<Buffer*>(p1); if(b)b->free.store(true,std::memory_order_release); } }
bool WaveOutPlayback::open(std::uint32_t rate, std::uint16_t channels, std::size_t max_samples, std::string& error){
  stop(); if(!rate||!channels||!max_samples){error="Invalid waveOut format";return false;} WAVEFORMATEX f{}; f.wFormatTag=WAVE_FORMAT_PCM; f.nChannels=channels; f.nSamplesPerSec=rate; f.wBitsPerSample=16; f.nBlockAlign=channels*2; f.nAvgBytesPerSec=rate*f.nBlockAlign;
  if(waveOutOpen(&handle_,WAVE_MAPPER,&f,reinterpret_cast<DWORD_PTR>(&callback),reinterpret_cast<DWORD_PTR>(this),CALLBACK_FUNCTION)!=MMSYSERR_NOERROR){error="waveOutOpen failed";handle_=nullptr;return false;} channels_=channels; max_samples_=max_samples;
  for(auto& b:buffers_){b.pcm=std::make_unique<short[]>(max_samples_); b.header.lpData=reinterpret_cast<LPSTR>(b.pcm.get()); b.header.dwBufferLength=static_cast<DWORD>(max_samples_*sizeof(short)); if(waveOutPrepareHeader(handle_,&b.header,sizeof(b.header))!=MMSYSERR_NOERROR){error="waveOutPrepareHeader failed";stop();return false;} b.free.store(true); } return true;
}
bool WaveOutPlayback::submit(std::span<const float> samples) noexcept { if(!handle_||samples.empty()||samples.size()>max_samples_)return false; Buffer* chosen=nullptr; for(auto& b:buffers_){bool expected=true;if(b.free.compare_exchange_strong(expected,false,std::memory_order_acq_rel)){chosen=&b;break;}} if(!chosen)return false; for(std::size_t i=0;i<samples.size();++i){const auto v=std::clamp(samples[i],-1.0f,1.0f);chosen->pcm[i]=static_cast<short>(v*32767.0f);} chosen->header.dwBufferLength=static_cast<DWORD>(samples.size()*sizeof(short)); if(waveOutWrite(handle_,&chosen->header,sizeof(chosen->header))!=MMSYSERR_NOERROR){chosen->free.store(true,std::memory_order_release);return false;} return true; }
void WaveOutPlayback::stop() noexcept { if(!handle_)return; waveOutReset(handle_); for(auto& b:buffers_){if(b.header.dwFlags&WHDR_PREPARED)waveOutUnprepareHeader(handle_,&b.header,sizeof(b.header));b.pcm.reset();b.header={};b.free.store(true);} waveOutClose(handle_);handle_=nullptr; }
}
#endif
