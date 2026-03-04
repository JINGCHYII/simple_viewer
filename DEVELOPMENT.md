# Development Notes

## Qt math helpers and STL math policy

To avoid platform-specific build issues and transitive-include surprises, use the following rules:

1. Prefer C++ standard library APIs for common math helpers:
   - `std::max` / `std::min` from `<algorithm>`.
   - `std::sin` / `std::cos` from `<cmath>`.
2. When converting degrees to radians for `std::sin` / `std::cos`, convert explicitly (for example with `qDegreesToRadians(...)` or an equivalent local conversion) and keep units consistent.
3. If Qt math symbols are used in a `.cpp` file (for example `qDegreesToRadians`, `qBound`, `qAtan2`), include the required Qt header explicitly in that same source file (typically `<QtMath>`), and do not rely on transitive includes.
4. On Windows builds, guard against `min/max` macro pollution by defining `NOMINMAX` at compile time.

These rules apply to new code and refactors to prevent recurring `min/max` and math-API related compile errors.
