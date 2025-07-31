import av
import zlib

def calculate_mjpeg_crc32(avi_file_path):
    """
    讀取 AVI 檔案，提取 MJPEG 視訊幀的 Packet 並計算每個 Packet 數據的 CRC32 值。

    Args:
        avi_file_path (str): AVI 檔案的路徑。

    Returns:
        dict: 一個字典，鍵是 Packet 的時間戳 (pts)，值是該 Packet 數據的 CRC32 值。
              如果找不到 MJPEG 視訊流，則返回一個空字典。
    """
    crc32_values = {}
    try:
        container = av.open(avi_file_path)
        video_stream = None
        for stream in container.streams:
            print(f"Stream type: {stream.type}, Available attributes: {dir(stream)}") # 偵錯用
            if stream.type == 'video' and getattr(stream.codec_context, 'name', None) == 'mjpeg':
                video_stream = stream
                break

        if video_stream is None:
            print("找不到 MJPEG 視訊流。")
            return crc32_values

        for packet in container.demux(video_stream):
            if packet.stream == video_stream:
                # packet.raw_packet.data 包含原始的壓縮數據
                mjpeg_data = bytes(packet)
                crc32_value = zlib.crc32(mjpeg_data)
                print(f"PTS: {packet.pts}, Length: {len(mjpeg_data)}, CRC32: {crc32_value:08x}")
                crc32_values[packet.pts] = crc32_value # 使用時間戳作為鍵

    except OSError as e:
        print(f"讀取 AVI 檔案時發生錯誤: {e}")
    finally:
        if 'container' in locals() and container:
            container.close()
    return crc32_values

if __name__ == "__main__":
    avi_file = './root/movie.avi'  # 將 'avi_file' 替換為您的 AVI 檔案路徑
    crc_values = calculate_mjpeg_crc32(avi_file)