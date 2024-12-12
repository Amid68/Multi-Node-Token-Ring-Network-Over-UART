# Application Entry Point (src)

This directory hosts the high-level application code that orchestrates the entire system.

## Contents

- **main.c**: The main entry point of the application, responsible for:
  - Initializing threads and synchronization primitives
  - Interacting with the token management subsystem
  - Handling sensor data and preparing payloads for transmission
  - Monitoring system health and handling configuration changes

## Integration
The  directory works closely with the  and  directories. It ensures that the hardware-level operations and token management logic are combined into a coherent, application-level solution.


