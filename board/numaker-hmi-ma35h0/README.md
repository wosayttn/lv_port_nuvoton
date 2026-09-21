# **NuMaker-HMI-MA35H0**

| Major Folder | Description |
|-|-|
| GCC | GCC project file |

## **GCC project**

To build the target execution using Eclipse-based IDE (NuEclipse V1.02.023r).
| Target | Description |
|-|-|
| MA35H0_1024x600 | Use 1024x600 LCD panel with resistive touch screen(H/W ADC Sampling) |
| MA35H0_1024x600_GT911_CTP | Use 1024x600 LCD panel with capacitive touch screen (1024x600 GT911) |

## **Compiling options**

- The porting sets `CONFIG_DISP_DIRECT_REFRESH` to 1 by default (Direct refresh approach). Set to 0 to use Partial refresh approach.

  ```c
  #define CONFIG_DISP_DIRECT_REFRESH       1
  ```

- The porting sets `LV_USE_DEMO_WIDGETS` to 1 by default; the LVGL Widgets demo will be executed at startup.

  ```c
  #define LV_USE_DEMO_WIDGETS             1
  //#define LV_USE_DEMO_BENCHMARK           1
  ```

- The porting sets `LV_USE_DRAW_GFX` to 1 by default to enable the hardware 2D Graphics Engine (GFX) accelerator.

  ```c
  #define LV_USE_DRAW_GFX                  1
  ```

## **Hardware 2D Graphics Accelerator (GFX)**

This board port includes a driver for the built-in hardware GFX (2D Graphics Engine, Vivante GC520L 2D GPU) of the Nuvoton MA35 family, providing hardware acceleration for the LVGL v9 drawing pipeline.

### **1. Enabling the GFX Accelerator**

Configure the macro switch in [lv_conf.h](lv_conf.h):

```c
#define LV_USE_DRAW_GFX 1
```

- **Enabled (`1`, default)**:
  - Registers the dedicated GFX draw unit (`lv_draw_gfx_init()`) during startup.
  - Automatically offloads supported drawing operations (e.g., solid fills with/without global alpha blending, image blit & scaling, layer blending, and buffer copying) to the hardware GFX accelerator.
  - In partial refresh mode (`CONFIG_DISP_DIRECT_REFRESH` set to `0`), dirty areas are also blitted to the framebuffer using `gfx_blt` hardware acceleration.
- **Disabled (`0`)**:
  - Disables hardware GFX acceleration and falls back to CPU software rendering (ARM Cortex-A35 NEON SIMD vector acceleration / `lv_draw_sw`).

### **2. Advantages of Enabling GFX Acceleration**

1. **Significantly Reduces CPU Overhead (Offloads CPU Load)**
   - Offloads computationally intensive 2D rendering tasks (such as BitBLT, rectangular fills with global alpha blending, image scaling, and layer blending) to the dedicated GFX hardware.
   - Frees up Cortex-A35 CPU cycles for application logic, network communication, and other real-time tasks.

2. **Supports Asynchronous Multi-Threaded Rendering**
   - Under FreeRTOS, the GFX draw unit operates asynchronously within a dedicated rendering thread (`gfx_draw`), executing concurrently with CPU tasks to increase overall system throughput and UI responsiveness.

3. **Lowers System Power Consumption and Heat Generation**
   - Dedicated hardware accelerators have much higher energy efficiency (performance-per-watt) than general-purpose CPU cores for graphical workloads.
   - Completing drawing operations faster reduces CPU active high-load time, lowering overall chip power consumption and heat dissipation.

4. **Accelerates Framebuffer Blitting in Partial Refresh Mode**
   - In partial refresh mode, dirty regions are transferred directly to the framebuffer via hardware `gfx_blt`, bypassing slow CPU-driven pixel copy loops.

> **Note on Benchmark & FPS:**
> With the implementation of hardware-accelerated **solid fill with global alpha blending (Fill + Global Alpha Blending)**, GFX acceleration reaches **approx. 55 FPS** in synthetic benchmarks such as `LV_USE_DEMO_BENCHMARK`, outperforming CPU software rendering (approx. 54 FPS with Cortex-A35 NEON SIMD).
> Small UI elements (e.g., text, anti-aliased arcs, small icons, and borders) continue to be processed in CPU L1 cache with NEON instructions to avoid GPU/cache synchronization overhead, while GFX accelerates heavy operations (such as surface fills with/without global alpha blending, and image scaling). Beyond raw FPS, the primary benefit of GFX is offloading CPU utilization, allowing concurrent application and background processing without degrading UI responsiveness.

## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/tw/numaker-hmi-ma35h0-a1)

## **Resources**

[NuWriter](https://github.com/OpenNuvoton/MA35H0_NuWriter)
