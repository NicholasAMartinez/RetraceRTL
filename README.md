# RetraceRTL

RetraceRTL is an experimental PX4 recovery strategy that records an aircraft's flown path and attempts to retrace it during recovery.

The goal is to reduce traversal through unknown space by reusing a path the aircraft has already successfully flown.

## Motivation

Conventional RTL strategies may climb to a predefined altitude or fly directly toward home. In environments with trees, buildings, overhangs, or other obstacles, this can require entering previously untraversed space. For some applications this may be completely acceptable, but it is not always possible.

RetraceRTL instead attempts to follow the recorded flight path in reverse while maintaining a level of confidence in the return path. This project's goal is similar to ArduPilot's [SmartRTL](https://ardupilot.org/copter/docs/smartrtl-mode.html), but it will not inherently provide full Return to Launch/Land.

## Concept

1. Record the aircraft's trajectory during normal flight.
2. Maintain a bounded and simplified breadcrumb history.
3. Trigger RetraceRTL during an applicable recovery event (e.g., loss of the command-and-control (C2) link or another configured failsafe condition).
4. Follow the recorded path in reverse.
5. Return control to the pilot if communication is restored.
6. Fall back to another PX4 recovery strategy if reliable retracing is no longer possible.

## Design Goals

* Avoid intentional shortcuts through untraversed space.
* Keep CPU and memory usage suitable for PX4 flight controllers.
* Use bounded breadcrumb storage.
* Remove redundant path points without materially changing the recorded trajectory.
* Detect when localization or path tracking is no longer reliable.
* Integrate cleanly with existing PX4 failsafe and RTL behavior.

## Limitations

RetraceRTL does not guarantee a safe return.

Its effectiveness depends on factors including:

* localization accuracy
* path-following accuracy
* environmental changes
* dynamic obstacles
* available breadcrumb history

Previously traversed space only proves that at one point it was safe, not that it is safe for the return trip.

## Project Goal

Determine whether a small, bounded record of an aircraft's recent trajectory can provide a practical PX4 recovery strategy that minimizes traversal through unknown space.

## License

See [LICENSE](LICENSE).
