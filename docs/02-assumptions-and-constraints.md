# Assumptions and Constraints

This document defines the assumptions RetraceRTL is designed around and the constraints that limit how it can be implemented.

## Assumptions

### Vehicle

RetraceRTL will initially target PX4 multirotors. Other vehicle types may be supported later, but they are outside the scope of the initial implementation.

The flight controller is assumed to have enough available memory and processing power to run RetraceRTL without noticeably affecting normal PX4 operation. Since different flight controllers have different amounts of available memory, RetraceRTL should support multiple memory configurations rather than requiring one fixed amount.

### Position and State Estimation

RetraceRTL assumes PX4 can provide a valid local position estimate with enough horizontal and vertical accuracy to record and retrace the vehicle's path. The first implementation will use PX4's local position estimate, which reports the vehicle's position in meters relative to a local origin in a North-East-Down (NED) frame. Its vertical coordinate increases downward. [PX4 VehicleLocalPosition message](https://docs.px4.io/main/en/msg_docs/VehicleLocalPosition).

RetraceRTL should depend on the quality of that position estimate rather than specifically requiring GPS. If the position becomes unreliable, RetraceRTL should stop retracing and hand control over to another recovery behavior supported by the remaining state estimate. Conventional PX4 Return mode requires a global position estimate and a home position, so it cannot always serve as a fallback when only local positioning is available. [PX4 Return mode documentation](https://docs.px4.io/main/en/flight_modes/return).

PX4 may also reset its local position estimate during flight. Its local position message provides horizontal and vertical reset counters and position changes for detecting and accounting for these resets. If a reset changes how the current estimate relates to the stored path, RetraceRTL will either need to update the existing breadcrumbs or invalidate them. For the initial implementation, invalidating the path may be the simpler and safer option. [PX4 VehicleLocalPosition message](https://docs.px4.io/main/en/msg_docs/VehicleLocalPosition).

### Environment

RetraceRTL assumes that previously traveled space is more likely to remain traversable than space the vehicle has never traveled through. This does not mean that the previous path is guaranteed to still be safe. A gate could close, an object could move into the path, or the environment could otherwise change.

The initial implementation will therefore focus on faithfully retracing the recorded path rather than taking shortcuts through previously untraveled space.

This can be useful when normal RTL behavior is not ideal. Climbing may be unsafe around trees, buildings, ceilings, or overhangs. Altitude restrictions may also prevent the vehicle from simply climbing above every obstacle, and wind conditions may be worse at a higher RTL altitude than along the path originally flown.

### Activation and Recovery

RetraceRTL may be selected directly by the pilot or used as a recovery behavior after an event such as loss of the control or C2 link.

PX4 treats manual control loss, ground-station data-link loss, and Offboard control loss as separate failsafe conditions. RetraceRTL will need to define which of these conditions can activate it and how it interacts with their configured timeouts and actions. [PX4 failsafe documentation](https://docs.px4.io/main/en/config/safety).

The vehicle may continue recording breadcrumbs until PX4 determines that the control link has been lost. Once RetraceRTL begins retracing, it should stop recording new breadcrumbs so that the recovery path does not begin replacing the original path.

RetraceRTL does not necessarily need to return all the way to the launch point to be useful. Retracing enough of the route to regain control or reach a more controllable area may be enough.

If the complete path is still stored, a full return may be possible. If the flight was too long or complex, older parts of the path may already have been removed from memory.

## Constraints

### Flight Controller Resources and Path Storage

RetraceRTL should use less than 1% additional CPU during normal breadcrumb recording. This is currently a design target and will need to be verified through testing.

Path storage will use a fixed amount of memory. Initial configurations will likely include 8 KiB, 16 KiB, 32 KiB, and 64 KiB. The 8 KiB configuration provides a lower-memory option, while larger configurations can retain longer or more complex paths.

The breadcrumb buffer should be reserved before normal flight operation and reused throughout the flight. RetraceRTL should not continuously allocate more memory while flying. Dynamic allocation could reduce unused memory early in the flight, but the added overhead and possibility of allocation failure are probably not worth the benefit.

When the buffer becomes full, older path data may be removed to make room for newer breadcrumbs. Path simplification may also be used to preserve more useful flight history without increasing the memory limit.

Breadcrumbs may be recorded at up to about 3 Hz, although a new point does not need to be stored every update. A movement threshold can prevent the buffer from being filled with nearly identical points while the vehicle is stationary or barely moving. Altitude must also be stored so that RetraceRTL can retrace the original three-dimensional path.

The proposed recording rate will need to be tested against vehicle speed and path curvature. At 3 Hz, a vehicle moving at 10 m/s travels about 3.3 m between updates, so this rate alone does not guarantee that tight turns are captured accurately.

### Path Simplification and Accuracy

Path simplification error is the difference between the original recorded breadcrumb path and the simplified path kept by RetraceRTL. This only represents error introduced by removing or merging breadcrumbs.

The current target is a maximum path simplification error of about 0.5 m. Around 1.0 m is currently considered the upper acceptable limit. Smaller values such as 0.25 m and 0.125 m may also be tested to determine whether the additional path accuracy is worth the increased memory usage.

The actual vehicle may still be farther from its original physical path. Recording only discrete points can miss movement between samples. Position-estimation error, including drift between recording and retracing, and the error introduced while the vehicle attempts to follow the stored path also contribute to the final retrace error. The proposed simplification limits are design targets, not demonstrated limits on the vehicle's total retrace error or obstacle clearance.

Testing should therefore measure both the difference between the original and simplified stored paths and the difference between the original and retraced vehicle trajectories.

### PX4 Integration and Development Standards

RetraceRTL should fit into the existing PX4 architecture rather than creating a separate control system. It will need to use the existing navigation, flight-mode, parameter, communication, position-checking, and failsafe systems where appropriate.

The implementation should follow the same general patterns used by existing PX4 components such as Navigator, Commander, and RTL. RetraceRTL is intended to be another recovery option, not a replacement for the existing RTL system.

PX4 also has [contribution guidance](https://docs.px4.io/main/en/contribute/code) and [repository contribution instructions](https://github.com/PX4/PX4-Autopilot/blob/main/CONTRIBUTING.md) that define its development model, coding and style standards, formatting rules, commit message conventions, and testing expectations. RetraceRTL will follow those conventions.

PX4 development also calls for appropriate unit and integration testing. Since RetraceRTL should be a purely software feature, SITL will be the minimum acceptable system-level test environment before physical flight testing is considered.

SITL testing should cover normal retracing, complex paths, different memory limits, different simplification limits, full buffers, manual activation, control-link loss, degraded position estimates, estimator resets, insufficient stored paths, and transitions to another recovery behavior. The original and retraced trajectories should also be recorded so that retrace error can be measured directly.

### Safety Limitations

RetraceRTL cannot guarantee successful recovery. Previously traveled space may no longer be safe, and the initial implementation will not provide independent obstacle avoidance.

RetraceRTL also depends on having a reliable position estimate. If the required position becomes unavailable or unreliable, it should stop retracing and transition to another recovery behavior supported by the remaining state estimate.

It cannot recover a vehicle that has already collided, become physically uncontrollable, or otherwise failed before recovery begins. RetraceRTL should therefore be treated as another recovery option with its own tradeoffs rather than as a replacement for every existing RTL or failsafe method.
