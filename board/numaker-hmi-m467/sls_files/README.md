# Design a GUI example using **SqaureLine Studio (SLS)**

To export SLS UI files into this folder and import all c source files to lvgl project. Please refer following steps.

- Specify SLS project name and storing path.

- Fill SLS project attributes.
  - Resolution: **480x272**  (MUST)
  - Rotation: **0 degree**  (MUST)
  - Offset: **X=0, Y=0**  (MUST)
  - Shape: **Rectangle**
  - Color depth: **16 bit**  (MUST)
  - LVGL version: **Latest**
  - Theme: **Dark**
  - Theme: **Disable**

- Design your UI

- Export C source files of UI
  - Export to lv_port_nuvoton/board/**board-name**/sls_files folder.
  - Ignore Pop-up window, it is no matter with following operation.

- Import all C source files in sls_files folder into lvgl project.

- Re-build project.

- Download execution using programming tool.

## **Resources**
[SquareLine Studio](https://squareline.io/downloads)
