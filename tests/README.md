# Testing Framework (tests)

This directory contains all test suites for ensuring code quality, functionality, reliability, and performance.

## Structure

- **unit/**: Tests for individual functions or modules in isolation (e.g., CRC calculation).
- **integration/**: Verifies combined operation of multiple subsystems (e.g., UART driver with token management).
- **system/**: End-to-end tests that confirm the entire token-ring network, across multiple nodes, meets timing and reliability requirements.

## Approach

Tests utilize Zephyr’s ztest framework for consistent execution and reporting. They can be run on hardware targets or in simulation, ensuring continuous verification as the project evolves.


