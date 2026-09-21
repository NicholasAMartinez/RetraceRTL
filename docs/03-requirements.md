# Requirements

This document defines what RetraceRTL needs to do and the standards it needs to follow for possible inclusion in PX4. Each requirement should have a clear way to check whether it has been met.

Implementation details are covered in the design documents.

Resource limits, supported vehicle types, positioning assumptions, and other system constraints are defined in [Assumptions and Constraints](02-assumptions-and-constraints.md). That document also lists the project's numerical targets.

Open Decisions lists the limits and recovery choices that need to be settled before testing.

## FR-1: Path Recording

RetraceRTL shall maintain a history of the path previously traveled by the vehicle.

* **FR-1.1:** Each path point shall store the vehicle's three-dimensional local position.
* **FR-1.2:** RetraceRTL shall record the first valid local position after the vehicle is armed as the first path point.
* **FR-1.3:** After the first path point is recorded, RetraceRTL shall add a new point only when the vehicle has moved at least the configured minimum distance from the last recorded point.
* **FR-1.4:** RetraceRTL shall not record new path points while retracing is active or local position is invalid.
* **FR-1.5:** RetraceRTL shall mark the recorded path as invalid if the vehicle's local position becomes invalid after path recording has begun.
* **FR-1.6:** RetraceRTL shall clear the recorded path and restore the path to a valid state before recording begins for a new arming cycle.

### Verification

**Unit Tests (gtest)**

* Verify that each recorded path point stores the vehicle's three-dimensional local position.
* Verify that the first valid local position after arming is recorded as the first path point.
* Verify that a new point is recorded only after the vehicle has moved at least the configured minimum distance from the last recorded point.
* Test the movement threshold just below, at, and above its configured value.
* Verify that vertical-only movement is included when evaluating the movement threshold.
* Verify that new path points are not recorded while retracing is active.
* Verify that new path points are not recorded while local position is invalid.
* Verify that the recorded path is marked invalid if local position becomes invalid after path recording has begun.
* Verify that a new arming cycle clears the previous recorded path and restores the path to a valid state.

**SITL Integration Tests (MAVSDK)**

* Fly a known three-dimensional path and verify that RetraceRTL records an ordered path history representing the vehicle's traveled path.
* Introduce an invalid local-position condition after recording begins and verify that the recorded path becomes invalid.
* Complete an arming cycle, begin a new arming cycle, and verify that the previous flight's path is not reused.

## FR-2: Path Management

RetraceRTL shall keep the recorded path within its memory limit while preserving the order of the points and meeting the simplification error limit.

* **FR-2.1:** RetraceRTL shall use a fixed amount of memory for path storage while the vehicle is armed.
* **FR-2.2:** When insufficient path storage remains after permitted simplification, RetraceRTL shall discard the oldest retained path information as necessary to store new path points.
* **FR-2.3:** Stored points shall remain in the order they were recorded.
* **FR-2.4:** A simplified section of the path shall remain within the configured three-dimensional error limit of the consecutive recorded path section that it replaces.
* **FR-2.5:** Simplification shall only replace consecutive path points with a straight segment between retained endpoints and shall not connect nonconsecutive sections of the recorded path.
* **FR-2.6:** RetraceRTL shall mark the recorded path as invalid if a horizontal or vertical local-position reset occurs after path recording has begun.

### Verification

**Unit Tests (gtest)**

* Fill the path storage to capacity and verify that path memory use remains fixed while the vehicle is armed.
* Exceed the available path storage after permitted simplification and verify that the oldest retained path information is discarded as necessary.
* Verify that stored path points remain in the order they were recorded.
* Test simplification against known three-dimensional geometries and verify that each simplified section remains within the configured error limit of the consecutive recorded path section it replaces.
* Verify that simplification does not connect nonconsecutive sections of the path.
* Test loops and path crossings and verify that nearby nonconsecutive sections are not joined during simplification.
* Test horizontal, vertical, and combined three-dimensional path changes against the configured simplification error limit.
* Verify that repeated simplification does not produce a simplified section that exceeds the configured error limit relative to the path section it represents.
* Verify that a horizontal or vertical local-position reset marks the recorded path as invalid.

**SITL Integration Tests (MAVSDK)**

* Simulate horizontal and vertical local-position resets during flight and verify that the recorded path becomes invalid.
* Fly paths containing turns, vertical changes, loops, and crossings and verify that path management preserves path order and does not create connections between nonconsecutive sections.

## FR-3: Activation

RetraceRTL shall only activate when the information required to perform retracing is available.

* **FR-3.1:** RetraceRTL shall require a valid recorded path before activation.
* **FR-3.2:** RetraceRTL shall require a valid local position before activation.
* **FR-3.3:** Before activation, the vehicle shall be no farther than the configured maximum distance from the newest recorded path point.
* **FR-3.4:** RetraceRTL shall allow activation through PX4's normal mode-selection system.
* **FR-3.5:** RetraceRTL shall be available as a configured failsafe response for the supported link-loss conditions.
* **FR-3.6:** RetraceRTL shall reject activation when its activation requirements are not satisfied.

### Verification

**SITL Integration Tests (MAVSDK)**

* Request RetraceRTL with a valid recorded path and valid local position and verify that the vehicle enters RetraceRTL.
* Attempt activation with an invalid recorded path and verify that RetraceRTL does not activate.
* Attempt activation with an invalid local position and verify that RetraceRTL does not activate.
* Attempt activation with the vehicle within, at, and beyond the configured maximum distance from the newest recorded path point and verify the expected activation result.
* Request RetraceRTL through PX4's normal mode-selection system and verify that activation occurs only when the activation requirements are satisfied.
* Configure RetraceRTL as the response for each supported link-loss condition and verify that the corresponding failsafe can request RetraceRTL.
* Configure a different failsafe response for each supported link-loss condition and verify that RetraceRTL is not entered.
* Trigger a supported link-loss failsafe when the RetraceRTL activation requirements are not satisfied and verify that PX4 proceeds to the configured fallback behavior.

## FR-4: Retracing

RetraceRTL shall retrace the retained path in the reverse direction.

* **FR-4.1:** RetraceRTL shall process retained path points from newest to oldest.
* **FR-4.2:** RetraceRTL shall generate three-dimensional retracing targets from the retained path.
* **FR-4.3:** RetraceRTL shall follow retained path segments in reverse order without skipping to a nonconsecutive older segment.
* **FR-4.4:** RetraceRTL shall advance past the current target only when its horizontal and vertical distances from the target are no greater than their configured target-acceptance limits.
* **FR-4.5:** RetraceRTL shall request fallback if the vehicle remains beyond the configured maximum tracking error for longer than the configured allowed time.
* **FR-4.6:** RetraceRTL shall request fallback if the vehicle fails to reach the current target within the configured target timeout.

### Verification

**Unit Tests (gtest)**

* Provide a known sequence of retained points and verify that retracing processes them from newest to oldest.
* Verify that retracing targets preserve all three components of each retained local position.
* Verify that retracing proceeds through consecutive retained path segments without skipping to a nonconsecutive older segment.
* Verify that the current target remains active until both the configured horizontal and vertical target-acceptance limits are satisfied.
* Test the horizontal and vertical target-acceptance limits separately just below, at, and above their configured values.
* Verify that fallback is requested when tracking error remains above the configured maximum for longer than the allowed time.
* Test the tracking-error distance and duration limits just below, at, and above their configured values.
* Verify that fallback is requested when the current target is not reached before the configured target timeout.

**SITL Integration Tests (MAVSDK)**

* Fly a known three-dimensional path, activate RetraceRTL, and verify that the retained path is followed in reverse order.
* Verify that horizontal and vertical changes in the recorded path are reproduced during retracing.
* Compare the commanded retracing targets with the retained path.
* Compare the simulated return trajectory with the original simulated flight path and measure three-dimensional retrace error.
* Introduce excessive tracking error and verify that RetraceRTL requests fallback after the configured duration.
* Prevent the vehicle from reaching a target and verify that RetraceRTL requests fallback after the configured target timeout.

## FR-5: Completion and Fallback

RetraceRTL shall stop retracing when it completes the retained path or can no longer continue safely under its requirements.

* **FR-5.1:** RetraceRTL shall detect when the oldest retained path point has been reached and no additional retained path remains.
* **FR-5.2:** RetraceRTL shall request fallback when the retained path is exhausted.
* **FR-5.3:** RetraceRTL shall request fallback if the required local position becomes invalid during retracing.
* **FR-5.4:** RetraceRTL shall request fallback if the retained path becomes invalid during retracing.
* **FR-5.5:** RetraceRTL shall stop retracing when PX4 accepts a pilot-requested mode change or selects another mode because of a higher-priority condition.
* **FR-5.6:** After RetraceRTL exits, it shall stop issuing retracing targets.

### Verification

**Unit Tests (gtest)**

* Verify detection of the oldest retained path point and exhaustion of the retained path.
* Verify that path exhaustion requests fallback.
* Verify that loss of valid local position during retracing requests fallback.
* Verify that an invalid retained path during retracing requests fallback.
* Verify that RetraceRTL stops issuing retracing targets after exit.

**SITL Integration Tests (MAVSDK)**

* Exhaust the retained path during RetraceRTL and verify that PX4 performs the configured fallback behavior.
* Invalidate local positioning during RetraceRTL and verify that fallback is requested.
* Invalidate the retained path during RetraceRTL and verify that fallback is requested.
* Request an allowed pilot mode change during RetraceRTL and verify that RetraceRTL exits.
* Trigger a higher-priority PX4 condition that selects another mode and verify that RetraceRTL exits.
* Verify that no additional retracing targets are issued after RetraceRTL exits.

## FR-6: Status and Reporting

RetraceRTL shall report information needed to determine its availability, activation state, and reason for exit.

* **FR-6.1:** RetraceRTL shall report when it becomes unavailable and identify the reason.
* **FR-6.2:** RetraceRTL shall report when activation is accepted or rejected and identify the reason for rejection.
* **FR-6.3:** RetraceRTL shall report when retracing starts, completes, or requests fallback and identify the reason for fallback.
* **FR-6.4:** When flight logging is enabled, RetraceRTL shall log sufficient information to reconstruct the recorded path, commanded retracing targets, and the reason retracing started or stopped.

### Verification

* Trigger each availability, activation, completion, and fallback condition and verify that the expected status or event is reported through PX4's existing reporting interfaces.
* Inspect a SITL flight log and verify that the recorded path, commanded targets, activation, and exit reason can be reconstructed from the logged information.

## PX4 Integration and Contribution Requirements

RetraceRTL shall follow the requirements below in preparation for possible submission to PX4.

* **CR-1:** RetraceRTL shall integrate with PX4's existing mode, navigation, parameter, reporting, and failsafe interfaces. When RetraceRTL is disabled, existing PX4 recovery behavior shall remain unchanged.
* **CR-2:** Contributions shall follow the target PX4 revision's coding style and pass the applicable formatting, static-analysis, build, and CI checks.
* **CR-3:** Upstream changes shall follow PX4's current contribution, commit, and pull-request requirements.
* **CR-4:** Contributions shall meet PX4's current licensing, author sign-off, and disclosure requirements.
* **CR-5:** The implementation shall include automated unit and SITL integration tests for the behavior defined by these requirements. Test procedures and results shall be documented so that others can reproduce the verification.

The upstream references are [CONTRIBUTING.md](https://github.com/PX4/PX4-Autopilot/blob/main/CONTRIBUTING.md) and [Source Code Management](https://docs.px4.io/main/en/contribute/code), reviewed on September 13, 2026. Their current requirements shall be checked again before submission.

### Verification

* Build RetraceRTL for each supported target and run the applicable PX4 formatting, static-analysis, build, and CI checks.
* Verify that existing recovery behavior remains unchanged when RetraceRTL is disabled.
* Record the PX4 revision, supported build targets, check results, and automated test results.
* Review commits and submission materials against the current PX4 contribution requirements before submission.

## Open Decisions

Record these choices in the design and verification plan before testing the related requirements.

| Decision                                                                                                   | Requirements affected         |
| ---------------------------------------------------------------------------------------------------------- | ----------------------------- |
| Minimum distance between recorded path points                                                              | FR-1.3                        |
| Supported path-memory limits and how resource use will be measured                                         | FR-2.1                        |
| Allowed simplification error and how repeated simplification will remain within that limit                 | FR-2.4                        |
| Maximum allowed distance between the vehicle and newest recorded point at activation                       | FR-3.3                        |
| Supported link-loss conditions that may request RetraceRTL                                               | FR-3.5                        |
| Horizontal and vertical target-acceptance limits                                                           | FR-4.4                        |
| Maximum allowed path-tracking error and duration before fallback                                           | FR-4.5                        |
| Maximum time allowed to reach a retracing target                                                        | FR-4.6                        |
| Allowed total retrace error used during flight verification                                                | FR-4 verification             |
| Fallback behavior after path exhaustion, position loss, path invalidation, or rejected failsafe activation | FR-3.6, FR-5.2 through FR-5.4 |
| Recording behavior after RetraceRTL exits, including whether recording resumes or a new path is required | FR-1.4, FR-1.6, FR-5.6        |
| PX4 base revision, supported multirotor build targets, and test environment                                | CR-1, CR-2, CR-5              |

## Out of Scope

The initial implementation does not attempt to:

* Detect obstacles that have entered the previously traveled path.
* Guarantee that previously traveled space remains safe.
* Explore previously untraversed space.
* Generate shortcuts through previously untraversed space.
* Plan a new route around obstacles.
* Guarantee return to launch or landing.
* Provide initial support for vehicle types outside the defined project constraints.
* Replace PX4's existing position-estimation or failsafe systems.

## Potential Future Work

Possible future extensions include:

* Comparing additional path-simplification algorithms.
* Adjusting point spacing based on vehicle speed, turns, or other flight conditions.
* Supporting additional vehicle types.
* Detecting obstacles during retracing.
* Recording heading or additional vehicle-state information with each path point.
* Handling local-position resets without invalidating the retained path.
* Hardware-in-the-loop and expanded physical flight testing.
* Additional conditions for ending retracing and selecting a recovery mode.

## Requirement Traceability

Detailed verification procedures and test identifiers shall be defined in [Verification Plan](06-verification-plan.md).

Each test or review shall identify the requirements it verifies.

Each numbered requirement shall link to test results or a documented review before it is marked verified.
