# NEXUS: Requirements-to-Test Traceability

Each requirement below is linked to a test, and each test states the log evidence that proves a pass. Status stays **Not run** until the test is executed on hardware and the result is recorded.

## 1. Requirements

| ID | Requirement | Verification method |
|---|---|---|
| NX-001 | Each of the 10 fault scenarios shall be selectable and injectable individually. | Test |
| NX-002 | Each injected fault shall produce at least one structured event in the log. | Test |
| NX-003 | Each event shall contain an event ID, a task identifier and a DWT cycle timestamp. | Test, Inspection |
| NX-004 | Event IDs shall come from the fixed set defined in `Logger.h`. | Inspection |
| NX-005 | Logging shall not block a task or allocate heap memory. | Inspection, Test |
| NX-006 | Timestamps shall use the Cortex-M DWT cycle counter. | Inspection, Test |
| NX-007 | The priority-inversion scenario shall emit events `0x60`-`0x64` in the defined order. | Test |
| NX-008 | `EVT_PI_STARVATION` shall be raised because a medium-priority task monopolises the CPU, not by a wall-clock tick comparison. | Test, Inspection |
| NX-009 | Ring-buffer contents shall remain readable after a fault such as a hard fault. | Test |
| NX-010 | The same scenario shall produce the same event sequence across repeated runs. | Test |
| NX-011 | Priority-inversion duration shall be measured in CPU cycles with and without mutex priority inheritance. | Test, Analysis |

## 2. Test cases

Replace each `TBD` with a real value when the test is run. Use the same board, FreeRTOS configuration and build for all tests, and record them.

| Test | Covers | Procedure | Expected evidence | Pass criteria | Status | Result / date |
|---|---|---|---|---|---|---|
| T-01 | NX-001 | Select each scenario in turn, one per build or run, and reset between runs. | Only the selected scenario's events appear. | 10 of 10 scenarios run alone with no events from another scenario. | Not run | TBD |
| T-02 | NX-002, NX-003 | Run each scenario and dump the log over UART. | At least one event per scenario, each with ID, task identifier and timestamp. | Every dumped entry has all three fields; no scenario produces an empty log. | Not run | TBD |
| T-03 | NX-004 | Compare every event ID in the dump against `Logger.h`. | All IDs found in `Logger.h`. | Zero unknown IDs. | Not run | TBD |
| T-04 | NX-005 | Review `Logger.c` for blocking calls and heap use; run with a heap-usage check before and after logging. | No malloc, no blocking call, heap usage unchanged. | Heap usage identical before and after N log calls. | Not run | TBD |
| T-05 | NX-006 | Toggle a GPIO at a known interval and compare the logged cycle delta with the expected cycle count at the configured core clock. | Logged delta matches the expected cycles. | Difference within TBD cycles. | Not run | TBD |
| T-06 | NX-007 | Run the priority-inversion scenario (no inheritance). | Events `0x60`-`0x64` appear in the defined order. | Order matches the table in the README. | Not run | TBD |
| T-07 | NX-008 | Run the priority-inversion scenario; confirm the medium task keeps running. Review the detection code. | `EVT_PI_STARVATION` appears; detection code uses task state, not a tick-time threshold. | Event present and code inspection confirms. | Not run | TBD |
| T-08 | NX-009 | Run the hard-fault scenario, reset or halt, then read the buffer. | Events logged before the fault are still readable. | All events up to the fault are recovered. | Not run | TBD |
| T-09 | NX-010 | Run each scenario 20 times. | Same event ID sequence each run. | Identical sequence in 20 of 20 runs; record the spread of cycle counts. | Not run | TBD |
| T-10 | NX-011 | Run priority inversion with and without priority inheritance; record cycles from the first to the last priority-inversion event. | Two cycle counts, one per mode. | Both measured over TBD runs; report mean and spread. | Not run | TBD |

## 3. Traceability matrix

| Requirement | Test(s) | Status |
|---|---|---|
| NX-001 | T-01 | Not run |
| NX-002 | T-02 | Not run |
| NX-003 | T-02 | Not run |
| NX-004 | T-03 | Not run |
| NX-005 | T-04 | Not run |
| NX-006 | T-05 | Not run |
| NX-007 | T-06 | Not run |
| NX-008 | T-07 | Not run |
| NX-009 | T-08 | Not run |
| NX-010 | T-09 | Not run |
| NX-011 | T-10 | Not run |

## 4. Open items

- Add the event ID for each scenario once the full list of 10 is written down.
- Fix the TBD tolerances (T-05, T-10) before running, not after.
- Record the board, clock speed, FreeRTOS version and compiler flags with each result.
