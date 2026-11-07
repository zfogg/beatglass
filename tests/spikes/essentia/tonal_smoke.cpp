// Standalone upstream build spike, not linked into Beatglass production targets.
#include <essentia.h>
#include <algorithmfactory.h>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using essentia::Real;
using essentia::standard::Algorithm;
using essentia::standard::AlgorithmFactory;
using Algo = std::unique_ptr<Algorithm>;

static std::string estimate(const std::vector<double>& notes) {
  constexpr int rate = 44100, size = 4096;
  auto& f = AlgorithmFactory::instance();
  Algo window(f.create("Windowing", "type", "hann"));
  Algo spectrum(f.create("Spectrum"));
  Algo peaks(f.create("SpectralPeaks", "sampleRate", rate));
  Algo hpcp(f.create("HPCP", "size", 12));
  Algo chord(f.create("Key", "profileType", "tonictriad", "usePolyphony", false));
  std::vector<Real> frame(size), windowed, spec, freq, mag, chroma;
  for (int i = 0; i < size; ++i) {
    double value = 0;
    for (double hz : notes) value += std::sin(2 * 3.141592653589793 * hz * i / rate);
    frame[i] = static_cast<Real>(value / notes.size());
  }
  window->input("frame").set(frame); window->output("frame").set(windowed);
  spectrum->input("frame").set(windowed); spectrum->output("spectrum").set(spec);
  peaks->input("spectrum").set(spec); peaks->output("frequencies").set(freq); peaks->output("magnitudes").set(mag);
  hpcp->input("frequencies").set(freq); hpcp->input("magnitudes").set(mag); hpcp->output("hpcp").set(chroma);
  std::string key, scale; Real strength = 0, separation = 0;
  chord->input("pcp").set(chroma); chord->output("key").set(key); chord->output("scale").set(scale);
  chord->output("strength").set(strength); chord->output("firstToSecondRelativeStrength").set(separation);
  window->compute(); spectrum->compute(); peaks->compute(); hpcp->compute(); chord->compute();
  return key + " " + scale;
}
int main() {
  essentia::init();
  try {
    const auto major = estimate({261.625565, 329.627557, 391.995436});
    const auto minor = estimate({220.0, 261.625565, 329.627557});
    std::cout << "C triad: " << major << "\nA minor triad: " << minor << '\n';
    essentia::shutdown();
    return major == "C major" && minor == "A minor" ? 0 : 1;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n'; essentia::shutdown(); return 1;
  }
}
