# **NuMaker-HMI-MA35H0**

| Major Folder | Description |
|-|-|
| GCC | GCC project file |

## **GCC project**

To build target execution using Eclipse-base IDE(NuEclipse V1.02.023r).
| Target | Description |
|-|-|
| MA35H0_1024x600 | Use 1024x600 LCD panel with resistive touch screen(H/W ADC Sampling) |

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
## **Purchase**

[Nuvoton Direct](https://direct.nuvoton.com/tw/numaker-hmi-ma35h0-a1)

## **Resources**

[NuWriter](https://github.com/OpenNuvoton/MA35H0_NuWriter)
