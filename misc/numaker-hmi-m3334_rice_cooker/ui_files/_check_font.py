import re
with open('ui_font_cjk_16.c', 'r', encoding='utf-8') as f:
    content = f.read()
m = re.search(r'glyph_bitmap\[\]\s*=\s*\{([^;]+)\};', content, re.DOTALL)
if m:
    data = m.group(1)
    vals = [x.strip() for x in data.split(',') if x.strip()]
    nonzero = sum(1 for v in vals if v not in ('0x00', '0', '0x0'))
    print(f'Bitmap: {len(vals)} bytes, {nonzero} non-zero ({100*nonzero/max(len(vals),1):.1f}%)')
    # CJK area starts after ASCII (~6000 bytes in)
    cjk_vals = vals[6000:]
    cjk_nz = sum(1 for v in cjk_vals if v not in ('0x00', '0', '0x0'))
    print(f'CJK area (after byte 6000): {len(cjk_vals)} bytes, {cjk_nz} non-zero')
else:
    print('bitmap array not found')
