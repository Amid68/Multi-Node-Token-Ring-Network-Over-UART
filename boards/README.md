# Board Configurations (boards)

This directory contains board-specific configuration files and device tree overlays.

## Purpose

- **nrf52840dk_nrf52840.overlay**, **nucleo_f446re.overlay**, **frdm_k64f.overlay**:
  Tailor UART pins, clock settings, and peripheral configurations for each supported target board.

## Importance

By isolating board-specific details here, we maintain a clean separation between application logic and hardware dependencies, making the system more portable and scalable.


