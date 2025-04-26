# Changelog

## [1.0.0] - 2025-04-26
### Added
- Initial implementation of Modular Factorization Pattern (MFP)
- Implemented Method 1: Expanded q Factorization
- Implemented Method 2: Ultrafast with Structural Filter
- Implemented Method 3: Parallelized with Dynamic Blocks
- Added comprehensive test suite for all three methods

### Fixed
- Corrected implementation to match the mathematical formulations in the specification
- Implemented the decimal redistribution pattern that is central to the MFP approach
- Added proper use of multipliers k ∈ {1, 3, 7, 9}
- Implemented correct q_max limits for each method
- Added structural filter check for Method 2
- Implemented block-based search with parallelization for Method 3

## Author
Daniel Charboneau
