# Design

RetraceRTL is implemented as a PX4 Navigator mode that records the vehicle's flown path while another navigation mode is active and retraces that path in reverse when RetraceRTL is activated.

The design prioritizes predictable behavior, bounded memory usage, and simple integration with the existing PX4 Navigator architecture.

## PX4 Integration

`RetraceRtl` inherits from both `NavigatorMode` and `ModuleParams`.

```cpp
class RetraceRtl : public NavigatorMode, public ModuleParams
```

`NavigatorMode` provides the lifecycle interface used by PX4 Navigator. RetraceRTL uses the standard mode callbacks:

* `on_inactive()` records the vehicle path while RetraceRTL is not active.
* `on_activation()` validates the stored path and initializes the return sequence.
* `on_active()` commands the vehicle through the stored path in reverse order.
* `on_inactivation()` clears temporary return-state information when the mode is exited.

The mode receives a pointer to the parent `Navigator` object, allowing it to access navigation state and publish or update position setpoints through the existing Navigator infrastructure.

`ModuleParams` provides access to PX4 parameters used by RetraceRTL. Parameters can later expose values such as minimum point spacing, waypoint acceptance tolerance, or timeout limits without hard-coding those values into the flight logic.

For the initial prototype, fixed constants may be used where runtime configuration is not yet necessary.

## Path Representation

The recorded path is stored as a fixed-size array of 3D local-position points.

Each point contains:

```cpp
matrix::Vector3f
```

representing the vehicle's local X, Y, and Z position.

A fixed-size structure is used instead of dynamic allocation so memory consumption remains deterministic during flight.

When the buffer becomes full, the oldest point is discarded and newer points continue to be recorded.

## Path Recording

Path recording occurs only while RetraceRTL is inactive.

The first valid local position after arming becomes the first stored path point.

Additional points are added only when the vehicle has moved at least a configured minimum distance from the most recently stored point.

This prevents unnecessary storage of nearly identical positions while still preserving the flown route.

Path recording stops while RetraceRTL itself is active.

If the local-position estimate becomes invalid, the stored path is marked invalid because the continuity of the recorded route can no longer be guaranteed.

The path is cleared when a new arming cycle begins.

## Path Simplification

The initial implementation may remove unnecessary intermediate points when consecutive points are approximately collinear.

If the middle point can be removed while keeping the resulting straight segment within the allowed positional error, it is discarded.

This reduces memory usage without changing the order of the recorded path.

No global route optimization or waypoint reordering is performed.

## Activation Checks

RetraceRTL may activate only when the recorded path is considered usable.

The initial checks should include:

* valid local position;
* valid stored path;
* sufficient number of recorded points;
* no known gap in position history;
* vehicle reasonably close to the newest recorded point.

If these checks fail, RetraceRTL should reject activation or hand control to the configured fallback behavior.

## Retracing Algorithm

When activated, RetraceRTL begins with the newest stored path point and moves backward through the array.

The vehicle is commanded toward one point at a time.

A point is considered reached only when both horizontal and vertical position errors are within their required tolerances.

After a point is reached, the target index moves to the previous stored point.

```text
Recorded flight:

P0 -> P1 -> P2 -> P3 -> P4

RetraceRTL:

P4 -> P3 -> P2 -> P1 -> P0
```

Points are followed in order and are not skipped in the initial implementation.

This keeps the return behavior directly tied to the path that the vehicle previously flew.

## Failure and Handoff Logic

RetraceRTL should stop attempting to retrace the route when continuing would no longer be considered safe or valid.

Examples include:

* loss of a valid local-position estimate;
* excessive distance from the expected path;
* failure to reach a target point within a timeout;
* invalid or exhausted path data.

When this occurs, RetraceRTL should hand control to an existing PX4 failsafe or navigation behavior rather than attempting to recover independently.

The initial RetraceRTL implementation is responsible for retracing a previously flown route. It is not responsible for guaranteeing landing, obstacle avoidance, or recovery after localization failure.
