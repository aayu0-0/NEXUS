# NEXUS

Fault-injection framework for STM32 + FreeRTOS. It deliberately triggers real-time failures and records what the scheduler did, with cycle-accurate timestamps.

**Status:** work in progress. Baseline fault scenarios are implemented. The priority-inheritance comparison is the next milestone.

## Why this exists

RTOS bugs such as priority inversion, task starvation and stack overflow are timing problems. They are hard to reproduce and harder to prove. NEXUS makes them repeatable: each fault is injected on purpose, logged as a structured event and timed with the Cortex-M DWT cycle counter, so the log shows what happened and how long it took.

Relevant wherever a task freeze or missed deadline is unacceptable, for example automotive, medical and industrial controllers.

## Features

- 10 fault scenarios, injected and observed one at a time
- Ring-buffer event logger (`Logger.h` / `Logger.c`) with a fixed set of event IDs, so logging stays cheap inside tasks and survives for post-crash analysis
- Cycle-accurate timing from the DWT cycle counter (`dwt.h` / `dwt.c`)
- Priority-inversion scenarios with dedicated event IDs `0x60`-`0x64`
- Deterministic starvation detection: `EVT_PI_STARVATION` fires because a medium-priority task permanently monopolises the CPU, not because of a wall-clock tick comparison

## Hardware and toolchain

| Item | Value |
|---|---|
| MCU / board | STM32F4 series (add exact part, e.g. STM32F401 Black Pill) |
| RTOS | FreeRTOS (add version) |
| IDE / toolchain | STM32CubeIDE, arm-none-eabi-gcc (confirm) |
| Debug / output | UART (add baud rate) |

## Fault scenarios

| # | Scenario | What it demonstrates | Expected log evidence |
|---|---|---|---|
| 1 | Priority inversion (no inheritance) | High-priority task blocked by a low-priority mutex holder while a medium task runs | `0x60`-`0x64`, then `EVT_PI_STARVATION` |
| 2 | Stack overflow | A task exceeds its stack; the overflow is detected and logged | add event ID |
| 3 | Memory exhaustion | Heap runs out and allocation fails | add event ID |
| 4 | Hard fault | Deliberate invalid access; event buffer survives for analysis | add event ID |
| 5-10 | add remaining scenarios | add one line each | add event IDs |

The last column doubles as a requirement-to-test trace: each scenario states the event that proves it worked.

## Event log format

Events are written to a ring buffer and dumped over UART. Each entry holds an event ID, a task identifier and a DWT cycle timestamp. Event ID ranges are defined in `Logger.h`; priority-inversion events use `0x60`-`0x64`.

## Project structure

```
NEXUS/
├── Core/
│   ├── Inc/        # Logger.h, dwt.h, fault scenario headers
│   └── Src/        # Logger.c, dwt.c, fault scenarios, main.c
├── README.md
└── .gitignore
```

Update the tree to match the actual folders.

## Build and run

1. Clone: `git clone https://github.com/aayu0-0/NEXUS.git`
2. Open the project in STM32CubeIDE and build.
3. Flash the board and open a serial terminal at the configured baud rate.
4. Select a scenario in the scenario config (add file name) and read the log.

## Results

Numbers will be added after the priority-inheritance run.

| Scenario | Without inheritance (cycles) | With inheritance (cycles) |
|---|---|---|
| Priority inversion | TBD | TBD |

## Limitations

- Tested on one board and one FreeRTOS configuration so far
- Faults are injected by software, so hardware-level faults such as bit flips are out of scope
- No automated pass/fail check yet; results are read from the log

## Roadmap

- [ ] Run the priority-inversion scenario with mutex priority inheritance enabled and compare cycle counts
- [ ] Add the results table and a short write-up
- [ ] Add an automated check that compares the logged event sequence against the expected one for each scenario
- [ ] Run static analysis (`cppcheck`) and record the result here

## Author

Aayush Sardana, [@aayu0-0](https://github.com/aayu0-0)
