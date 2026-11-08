#include "analysis_presentation.hpp"
void AnalysisPresentation::publish(const beatglass::Snapshot& s){bpm_=s.bpm?QString::number(*s.bpm,'f',1):QStringLiteral("—");key_=s.key_candidates.empty()?QStringLiteral("—"):QString::fromStdString(s.key_candidates.front().label);beats_=s.beats.size();frames_=s.observed_through.frame;emit changed();}
