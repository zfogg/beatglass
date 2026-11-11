#include "beatglass/core/visualization_color_mapping.hpp"
#include <algorithm>
#include <cmath>
namespace beatglass {
namespace { float clamp(float v) { return std::clamp(v, 0.0f, 1.0f); }
VisualizationColor clamp_color(VisualizationColor c) { return {clamp(c.red), clamp(c.green), clamp(c.blue)}; }
float luminance(VisualizationColor c) { c=clamp_color(c); return .2126f*c.red+.7152f*c.green+.0722f*c.blue; } }
float visualization_contrast(VisualizationColor a, VisualizationColor b) { const auto x=luminance(a), y=luminance(b); return (std::max(x,y)+.05f)/(std::min(x,y)+.05f); }
VisualizationColor map_visualization_energy(VisualizationEnergy e, const VisualizationColorMapping& m) {
  e.low=clamp(e.low); e.mid=clamp(e.mid); e.high=clamp(e.high); const float total=e.low+e.mid+e.high;
  VisualizationColor result;
  if (total <= .00001f) result={(m.low.red+m.high.red)*.5f,(m.low.green+m.high.green)*.5f,(m.low.blue+m.high.blue)*.5f};
  else result={(m.low.red*e.low+m.mid.red*e.mid+m.high.red*e.high)/total,(m.low.green*e.low+m.mid.green*e.mid+m.high.green*e.high)/total,(m.low.blue*e.low+m.mid.blue*e.mid+m.high.blue*e.high)/total};
  result=clamp_color(result); const auto background=clamp_color(m.background); const float target=std::max(1.0f,m.minimum_contrast);
  if (visualization_contrast(result,background)<target) { const auto white=VisualizationColor{1,1,1}, black=VisualizationColor{0,0,0}; result=visualization_contrast(white,background)>=visualization_contrast(black,background)?white:black; }
  return result;
}
}
