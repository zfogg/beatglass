#pragma once
#include <cstdint>
namespace beatglass {
/** Linear-light RGB color used by the visualization mapping contract. */
struct VisualizationColor { float red{}, green{}, blue{}; };
/** Three normalized energy bands supplied by a future analysis consumer. */
struct VisualizationEnergy { float low{}, mid{}, high{}; };
/** Palette and contrast policy for deterministic energy-to-color mapping. */
struct VisualizationColorMapping {
  VisualizationColor low{0.10f, 0.74f, 1.0f};
  VisualizationColor mid{0.65f, 0.45f, 1.0f};
  VisualizationColor high{1.0f, 0.28f, 0.58f};
  VisualizationColor background{0.04f, 0.05f, 0.08f};
  float minimum_contrast{1.35f};
};
/** Map clamped, normalized low/mid/high energy to one bounded contrast-safe RGB color. */
VisualizationColor map_visualization_energy(VisualizationEnergy energy, const VisualizationColorMapping& mapping = {});
/** Return relative luminance contrast ratio for two bounded RGB colors. */
float visualization_contrast(VisualizationColor foreground, VisualizationColor background);
}
