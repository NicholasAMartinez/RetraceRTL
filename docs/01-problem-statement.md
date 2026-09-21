# Problem Statement

This document defines the problem RetraceRTL is intended to address, summarizes existing approaches to the problem, and introduces the proposed solution.

## Problem

PX4 currently provides several RTL and recovery methods. Depending on the situation, these methods may not always be ideal, and attempting to regain control of the vehicle may be a better option than immediately returning to launch or landing.

For example, climbing to a return altitude may not be safe if there are obstacles above the vehicle, such as trees, structures, overhangs, or other obstructions. Landing in place may also cause the vehicle to land in an unsafe or inaccessible area or potentially damage the vehicle.

Retracing the vehicle's previous path is one way to reduce this risk. The idea is to attempt to follow a path that the vehicle has already successfully traveled through instead of returning through previously untraversed space.

This does not guarantee that the previous path is still safe. A gate could have closed, an object could have moved into the path, or the environment could have otherwise changed. Retracing therefore cannot guarantee obstacle avoidance, but it can attempt to return through space that was previously known to be traversable.

## Existing and Related Solutions

PX4 provides several failsafe and RTL behaviors. Each approach has to make tradeoffs depending on the available position information, environment, vehicle state, and desired recovery behavior. RetraceRTL will have to make similar tradeoffs.

ArduPilot provides a recovery mode called [SmartRTL](https://ardupilot.org/copter/docs/smartrtl-mode.html) that records the vehicle's path using breadcrumbs and performs additional path simplification and loop removal. SmartRTL has several useful features and configuration options and provides a strong existing example of breadcrumb-based recovery. PX4 already supports reversing a planned mission path, but this is different from recording and retracing the vehicle's actual flown path. The PX4 [Return mode documentation](https://docs.px4.io/main/en/flight_modes/return), reviewed on September 13, 2026, does not describe an equivalent SmartRTL-style breadcrumb recovery mode.

More novel forms of path-based or graph-based retracing are beyond the scope of the initial RetraceRTL implementation. After implementing and evaluating a more traditional breadcrumb-based RTL approach similar in concept to SmartRTL, these more advanced approaches could be explored and compared against it.

## Proposed Solution

RetraceRTL will record the vehicle's path as it travels and use that stored path to retrace the vehicle's previous movement in reverse during an applicable recovery event.

Recording and reversing a path is fairly straightforward when large amounts of memory and processing power are available, such as on a companion computer. A flight controller is much more constrained. Because of these limitations, tradeoffs have to be made between path complexity, maximum recorded distance, memory usage, and the accuracy of the reconstructed return path.

Because the complete flight path cannot always be retained, RetraceRTL cannot guarantee full return-to-launch capability. If sufficient path information is no longer available, or if the system otherwise determines that RetraceRTL cannot safely be used, the mode must be disabled or hand control to another recovery strategy. The vehicle should also notify the user when RetraceRTL is unavailable. ArduPilot SmartRTL similarly reports when it is unavailable, but it becomes unavailable when its buffer fills rather than discarding older history to allow partial retracing. Its configured link-loss failsafes can fall back to RTL or Land if SmartRTL cannot be entered. [SmartRTL documentation](https://ardupilot.org/copter/docs/smartrtl-mode.html).

RetraceRTL is intended to be an additional recovery option rather than a replacement for conventional RTL. It may be selected directly by the pilot, but the project is primarily interested in evaluating it as a recovery strategy during events such as loss of the control or C2 link. The primary goal is to faithfully retrace previously traveled space in an attempt to regain control or reach a safer recovery state rather than to find the shortest possible path back to the launch point.
