# UART Driver

This directory hosts the UART driver implementation and related files for the token-ring network.

## Structure

- **include/**: Public headers providing UART driver APIs, accessible by other subsystems and the application.
- **src/**: Source files for UART initialization, interrupt service routines, and integration with Zephyr’s device tree and driver model.

## Responsibilities

- Configure UART peripherals for supported boards (nRF52840-DK, NUCLEO-F446RE, FRDM-K64F)
- Provide a consistent, platform-agnostic interface to send and receive frames
- Support interrupt-driven, buffered I/O for deterministic token passing


