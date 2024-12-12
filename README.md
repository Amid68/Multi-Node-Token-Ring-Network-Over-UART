# Multi-Node-Token-Ring-Network-Over-UART
This repository hosts the development of a UART-based token-ring communication protocol for multi-node embedded systems. The goal is to achieve a deterministic, collision-free communication mechanism by sequentially passing a “token” frame through a closed loop of nodes, each granted turn-based transmission access. Leveraging the Zephyr Real-Time Operating System, the project focuses on predictable timing, robust error handling, and straightforward integration with common development boards.

**Key Features:**
- **Deterministic Communication:** Ensures only the token-holder transmits, preventing collisions.
- **Scalable Design:** Initially targets three nodes, but can be extended to five or more.
- **Robust Error Handling:** Integrates CRC checks and token regeneration strategies.
- **RTOS Integration:** Employs Zephyr’s threading and synchronization for reliable real-time operation.

**Project Status:**  
The requirements and design documents have been finalized. The repository setup and initial directory structure are the next steps. Implementation of the UART HAL, token management logic, and application-level data handling will follow incrementally.

**Planned Documentation:**
- [Requirements Document](docs/requirements.pdf)
- [Design Document](docs/design.pdf)
