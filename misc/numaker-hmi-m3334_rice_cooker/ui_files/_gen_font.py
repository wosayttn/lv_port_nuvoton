"""Generate CJK fonts using lv_font_conv with explicit Unicode codepoint ranges."""
import subprocess, os

os.chdir(os.path.dirname(os.path.abspath(__file__)))

# Read symbols
with open('_symbols.txt', 'r', encoding='utf-8') as f:
    chars = f.read().strip()

codepoints = sorted(set(ord(c) for c in chars))
ranges = ','.join(f'0x{cp:04X}' for cp in codepoints)
print(f'Total non-ASCII codepoints: {len(codepoints)}')

font = r'C:\D\PRJ\lv_port_nuvoton\lvgl\scripts\built_in_font\SourceHanSansSC-Normal.otf'

# Generate 16px
cmd16 = (
    f'lv_font_conv --bpp 4 --size 16 --no-compress --no-prefilter --font "{font}" '
    f'--range 0x20-0x7F,{ranges} '
    f'--format lvgl --lv-include lvgl.h --lv-fallback lv_font_montserrat_16 '
    f'-o ui_font_cjk_16.c'
)
print('Generating 16px font...')
r = subprocess.run(cmd16, shell=True, capture_output=True, text=True)
if r.returncode != 0:
    print(f'ERROR: {r.stderr}')
else:
    print('  OK')

# Generate 20px
cmd20 = (
    f'lv_font_conv --bpp 4 --size 20 --no-compress --no-prefilter --font "{font}" '
    f'--range 0x20-0x7F,{ranges} '
    f'--format lvgl --lv-include lvgl.h --lv-fallback lv_font_montserrat_20 '
    f'-o ui_font_cjk_20.c'
)
print('Generating 20px font...')
r = subprocess.run(cmd20, shell=True, capture_output=True, text=True)
if r.returncode != 0:
    print(f'ERROR: {r.stderr}')
else:
    print('  OK')

print('Done!')
