# v2 visual regression fixtures

`visual-regression.json` is the deterministic fixture contract for the v2 GUI and CLI states. Each case names its surface, route/state, viewport, baseline path, and comparison thresholds.

The baseline paths are intentionally explicit even when a native renderer has not yet produced the image. The validator reports those missing baselines as pending rather than treating design-only state as native evidence. Once the render harness is available, it should use the declared viewport and compare with both `maxDiffPixels` and `maxDiffPixelRatio`.

Run the portable contract check from the repository root:

```sh
python tests/unit/check_visual_regression_fixtures.py
```
