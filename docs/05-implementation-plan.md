# Implementation Plan

## Prototype Proof of Concept

The first objective of this project is to establish a basic working prototype while becoming familiar with the PX4 Navigator architecture and RTL-related code paths.

Based on the existing navigation modes in the repository, `BackTrackRtl` will inherit from `NavigatorMode` and `ModuleParams`.

```cpp
class BackTrackRtl : public NavigatorMode, public ModuleParams
```

For the prototype, most BackTrackRTL-specific behavior will remain contained within the `BackTrackRtl` class. This includes path storage, path recording, path validation, activation state, reverse traversal, and generation of the next position target.

The prototype should avoid introducing additional helper classes or abstractions unless they become necessary. Keeping the initial implementation localized will make the behavior easier to understand, debug, and review while the design is still changing.

Changes outside of the BackTrackRTL implementation should be limited to the PX4 integration required to make the mode available. This will include registering the mode with Navigator, adding any required navigation-state handling, and adding parameter metadata when configurable BackTrackRTL parameters are introduced.

The initial implementation will be developed incrementally:

1. Create the `BackTrackRtl` class and integrate it with Navigator sufficiently for the mode lifecycle callbacks to execute.
2. Record valid local-position samples while BackTrackRTL is inactive.
3. Store the recorded path in a fixed-size in-memory buffer with bounded memory usage.
4. Add basic path validity checks and clear the path when a new arming cycle begins.
5. Allow BackTrackRTL to activate using a valid stored path.
6. Traverse the stored points in reverse order and generate position targets for each point.
7. Verify the vehicle follows the recorded route in PX4 SITL before adding more advanced path simplification, failure handling, or configurable parameters.

The proof of concept is complete when a simulated multicopter can fly a route, activate BackTrackRTL, and visibly retrace the stored path in reverse using the existing PX4 navigation and position-control infrastructure.

More advanced features such as path simplification, configurable tolerances, timeout behavior, failsafe-triggered activation, and optimized memory usage should be added only after the basic retracing behavior is working reliably.
