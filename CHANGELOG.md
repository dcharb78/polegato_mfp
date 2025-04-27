# CHANGELOG.md

## 2025-04-27 - Enhanced MFP Method 3 with Search Space Reduction

### Added
- Implemented search space reduction for Method 3 based on UFRF framework
- Added system level determination to adapt search space based on number size
- Added Fibonacci-related offset focus with key positions (7, 2, 3)
- Added directional bias (60% positive, 40% negative) for search space filtering
- Created test suite to verify search space reduction effectiveness

### Changed
- Modified Method 3 to use a fixed search space of approximately 60 digits for larger numbers
- Updated parallelizedFactorization to incorporate system level determination
- Updated searchBlock and searchQSweep to skip values that should be excluded
- Updated CMakeLists.txt to include the new test executable

### Performance Improvements
- For system levels 1-9: No reduction (original search space)
- For system level 10+: Progressive reduction starting at 1.01x
- For system level 20+: Over 96% reduction in search space (30x+ speedup)
- For system level 30+: Over 99.9% reduction in search space (968x+ speedup)
- For system level 40+: Over 99.999% reduction in search space (30,000x+ speedup)

### Copyright
- Copyright (c) 2025 Daniel Charboneau. All rights reserved.
