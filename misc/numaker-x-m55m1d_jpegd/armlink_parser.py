import pandas as pd
import re

def parse_map_file(file_path):
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # 定義要搜尋的區段：從 "Image component sizes" 開始到下一條分隔線或總計
    # 這裡使用正則表達式定位表格內容
    pattern = r'Image component sizes\s+Code \(inc\. data\).+?\n\n(.*?)(?=\n\s*-+)'
    section_match = re.search(pattern, content, re.DOTALL)

    if not section_match:
        return "找不到 'Image component sizes' 區段，請檢查 Map 檔案格式。"

    rows = []
    section_text = section_match.group(1)
    
    # 逐行解析數據
    for line in section_text.strip().split('\n'):
        if not line.strip():
            continue
        
        # 使用正則表達式提取數字和 Object Name
        # 欄位依序為: Code, inc. data, RO Data, RW Data, ZI Data, Debug, Object Name
        match = re.match(r'\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(.+)', line)
        if match:
            g = match.groups()
            rows.append({
                'Object Name': g[6].strip(),
                'Code': int(g[0]),
                'inc_data': int(g[1]),
                'RO_Data': int(g[2]),
                'RW_Data': int(g[3]),
                'ZI_Data': int(g[4]),
                'Debug': int(g[5])
            })
    
    return pd.DataFrame(rows)

# 執行解析
df = parse_map_file('lvgl_m55m1_800x480.map')

if isinstance(df, pd.DataFrame):
    # 儲存為 CSV
    df.to_csv('object_sizes.csv', index=False)
    print("解析完成！資料已儲存至 'object_sizes.csv'")
    
    # 顯示前 10 筆資料供確認
    print("\n前 10 筆資料預覽：")
    print(df[['Object Name', 'Code', 'RO_Data', 'RW_Data', 'ZI_Data']].head(10))
else:
    print(df)