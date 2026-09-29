# RTL Retrace Operation

## Build

From the PX4-Autopilot repository root, build SITL:

```sh
make px4_sitl_default
```

To build and launch the X500 Gazebo simulation:

```sh
make px4_sitl gz_x500
```

## Enable and Configure Retrace

Retrace is disabled by default. Enable it before arming, using the PX4 shell:

```sh
param set RTL_RTR_EN 1
```

You can also find these parameters in QGroundControl’s parameter editor by searching for `RTL_RTR_`.

| Parameter | Default | Purpose |
| --- | ---: | --- |
| `RTL_RTR_EN` | `0` | Enable route retrace |
| `RTL_RTR_DIST` | `5.0` m | Minimum movement between recorded route points |
| `RTL_RTR_RATE` | `1.0` Hz | Maximum route point recording rate |
| `RTL_RTR_XY_ACC` | `2.0` m | Horizontal distance for accepting a route point |
| `RTL_RTR_Z_ACC` | `1.0` m | Vertical distance for accepting a route point |
| `RTL_RTR_TOUT` | `10.0` s | Time allowed to reach each point before falling back |

For example, to record points more frequently and closer together:

```sh
param set RTL_RTR_DIST 2.0
param set RTL_RTR_RATE 2.0
```

Recording requires an armed vehicle, valid local position, and at least two route points. With the defaults, fly at least 5 m and allow at least one second for another point to be recorded.

## Use

1. Enable `RTL_RTR_EN` before arming.
2. Arm and take off.
3. Fly the route you want retrace to follow.
4. Request Return using QGroundControl’s Return mode or the PX4 shell:

   ```sh
   commander mode auto:rtl
   ```

If a usable route is available, PX4 follows its recorded points in reverse. When retrace completes, or cannot start or continue, Navigator switches to the configured regular RTL behavior.

When all is said and done the drone should return to launch and ideally not crash.
