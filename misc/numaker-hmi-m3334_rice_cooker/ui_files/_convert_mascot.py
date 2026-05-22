"""
Split Nuvoton mascot image into 3 frames, remove background, and convert to LVGL ARGB8888 C arrays.
Usage: python _convert_mascot.py
Input: nuvoton_mascot.png (3-view mascot image)
Output: ui_img_mascot.c (3 frames as LVGL image descriptors with transparency)
"""
import os
from PIL import Image
import struct

os.chdir(os.path.dirname(os.path.abspath(__file__)))

# Target frame size for 320x240 display
FRAME_W = 112
FRAME_H = 112

def remove_background(img):
    """Remove grid/blue background using flood-fill from corners."""
    img = img.convert("RGBA")
    w, h = img.size
    pixels = img.load()

    # Use flood-fill from all 4 corners to find background
    from collections import deque
    visited = set()
    queue = deque()

    # Seed from corners
    for sx, sy in [(0,0), (w-1,0), (0,h-1), (w-1,h-1)]:
        queue.append((sx, sy))
        visited.add((sx, sy))

    def is_bg_color(r, g, b, a):
        # Background is light (near-white, light blue, light gray)
        # Character has saturated colors (red sleeves, skin tone, dark eyes)
        if a == 0:
            return True
        # Very light pixels (background grid area)
        if r > 210 and g > 210 and b > 210:
            return True
        # Light blue tint (grid background)
        if r > 190 and g > 200 and b > 220:
            return True
        return False

    while queue:
        x, y = queue.popleft()
        r, g, b, a = pixels[x, y]
        if is_bg_color(r, g, b, a):
            pixels[x, y] = (r, g, b, 0)  # Make transparent
            # Spread to neighbors
            for dx, dy in [(-1,0),(1,0),(0,-1),(0,1)]:
                nx, ny = x+dx, y+dy
                if 0 <= nx < w and 0 <= ny < h and (nx, ny) not in visited:
                    visited.add((nx, ny))
                    nr, ng, nb, na = pixels[nx, ny]
                    if is_bg_color(nr, ng, nb, na):
                        queue.append((nx, ny))

    return img

def image_to_cf_argb8888(img):
    """Convert PIL RGBA image to LVGL CF_ARGB8888 byte array."""
    data = []
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = img.getpixel((x, y))
            # LVGL ARGB8888: B, G, R, A (little-endian BGRA)
            data.append(b)
            data.append(g)
            data.append(r)
            data.append(a)
    return data

def write_c_array(f, name, img):
    """Write one image frame as a C array."""
    data = image_to_cf_argb8888(img)
    f.write(f"\nstatic const uint8_t {name}_map[] = {{\n")
    for i, b in enumerate(data):
        if i % 16 == 0:
            f.write("    ")
        f.write(f"0x{b:02x}, ")
        if i % 16 == 15:
            f.write("\n")
    f.write("\n};\n")
    f.write(f"\nconst lv_image_dsc_t {name} = {{\n")
    f.write(f"    .header.w = {img.width},\n")
    f.write(f"    .header.h = {img.height},\n")
    f.write(f"    .header.stride = {img.width * 4},\n")
    f.write(f"    .header.cf = LV_COLOR_FORMAT_ARGB8888,\n")
    f.write(f"    .data_size = {len(data)},\n")
    f.write(f"    .data = {name}_map,\n")
    f.write("};\n")

def main():
    src = Image.open("nuvoton_mascot.png").convert("RGBA")
    print(f"Source image: {src.width}x{src.height}")

    # Split into 3 equal horizontal parts
    frame_w = src.width // 3
    frames = []
    for i in range(3):
        crop = src.crop((i * frame_w, 0, (i + 1) * frame_w, src.height))
        # Remove background
        crop = remove_background(crop)
        # Resize to target
        crop = crop.resize((FRAME_W, FRAME_H), Image.LANCZOS)
        frames.append(crop)
        print(f"  Frame {i}: bg removed & resized to {FRAME_W}x{FRAME_H}")

    # Write C file
    with open("ui_img_mascot.c", "w", encoding="utf-8") as f:
        f.write("/**\n")
        f.write(" * @file  ui_img_mascot.c\n")
        f.write(" * @brief Nuvoton mascot animation frames (auto-generated, bg removed)\n")
        f.write(" */\n")
        f.write('#include "lvgl.h"\n\n')

        names = ["img_mascot_front", "img_mascot_side", "img_mascot_back"]
        for i, (name, img) in enumerate(zip(names, frames)):
            write_c_array(f, name, img)

    # Write header
    with open("ui_img_mascot.h", "w", encoding="utf-8") as f:
        f.write("#ifndef __UI_IMG_MASCOT_H__\n")
        f.write("#define __UI_IMG_MASCOT_H__\n\n")
        f.write('#include "lvgl.h"\n\n')
        f.write("extern const lv_image_dsc_t img_mascot_front;\n")
        f.write("extern const lv_image_dsc_t img_mascot_side;\n")
        f.write("extern const lv_image_dsc_t img_mascot_back;\n\n")
        f.write("#define MASCOT_FRAME_COUNT  3\n")
        f.write(f"#define MASCOT_W  {FRAME_W}\n")
        f.write(f"#define MASCOT_H  {FRAME_H}\n\n")
        f.write("#endif\n")

    print("Generated: ui_img_mascot.c, ui_img_mascot.h (with transparency)")

if __name__ == "__main__":
    main()
