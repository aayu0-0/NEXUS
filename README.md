# NEXUS

A fault-injection framework for **STM32 + FreeRTOS** that deliberately triggers real-time failures and records exactly what the scheduler did, with cycle-accurate timestamps.

> Status: work in progress. Baseline fault scenarios are implemented; the priority-inheritance comparison is the next milestone.

## Why this exists

RTOS bugs such as priority inversion, task starvation and stack overflow are timing problems. They are hard to reproduce and harder to prove. NEXUS makes them repeatable: each fault is injected on purpose, logged as a structured event, and timed with the Cortex-M DWT cycle counter, so you can see what happened and how long it took.

## Features

- **10 fault scenarios** that can be injected and observed one at a time
- **Ring-buffer event logger** (`Logger.h` / `Logger.c`) with a fixed set of event IDs, so logging stays cheap inside tasks
- **Cycle-accurate timing** using the DWT cycle counter (`dwt.h` / `dwt.c`)
- **Priority-inversion scenarios** with dedicated event IDs (`0x60`-`0x64`)
- **Deterministic starvation detection**: `EVT_PI_STARVATION` fires because a medium-priority task permanently monopolises the CPU, not because of a wall-clock tick comparison

## Hardware and toolchain

| Item | Value |
|---|---|
| MCU / board | _fill in (e.g. STM32F401 Black Pill)_ |
| RTOS | FreeRTOS _(fill in version)_ |
| IDE / toolchain | _fill in (e.g. STM32CubeIDE, arm-none-eabi-gcc)_ |
| Debug / output | _fill in (e.g. UART at 115200, SWO)_ |

## Fault scenarios

_List the 10 scenarios here with one line each, for example:_

| # | Scenario | What it demonstrates |
|---|---|---|
| 1 | Priority inversion (no inheritance) | High-priority task blocked by a low-priority mutex holder while a medium task runs |
| 2 | _..._ | _..._ |

## Event log format

Events are written to a ring buffer and dumped over _(fill in interface)_. Each entry holds an event ID, a task identifier and a DWT cycle timestamp. Event ID ranges are defined in `Logger.h`; priority-inversion events use `0x60`-`0x64`.

## Project structure

```
NEXUS/
├── Core/
│   ├── Inc/        # Logger.h, dwt.h, fault scenario headers
│   └── Src/        # Logger.c, dwt.c, fault scenarios, main.c
├── README.md
└── .gitignore
```

_Update this tree to match your actual folders._

## Build and run

1. Clone the repo: `git clone https://github.com/aayu0-0/NEXUS.git`
2. Open the project in _(your IDE)_ and build.
3. Flash to the board and open a serial terminal at _(baud rate)_.
4. Select a scenario in _(file / config)_ and observe the log.

## Results

_Add one table when you have numbers, for example priority inversion duration in CPU cycles without and with mutex priority inheritance._

| Scenario | Without inheritance (cycles) | With inheritance (cycles) |
|---|---|---|
| Priority inversion | _TBD_ | _TBD_ |

## Roadmap

- [ ] Run the same priority-inversion scenario with mutex priority inheritance enabled and compare cycle counts
- [ ] Add results table and a short write-up

## Author

Aayush Sardana, [@aayu0-0](https://github.com/aayu0-0)
