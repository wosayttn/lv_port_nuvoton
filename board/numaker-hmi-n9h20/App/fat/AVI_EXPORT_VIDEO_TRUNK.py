import av
import os

def save_video_packets_to_files(avi_file_path, output_directory):
    """
    讀取 AVI 檔案，提取每個視訊 Packet 的原始二進制數據，
    並將每個 Packet 儲存為一個單獨的檔案。

    Args:
        avi_file_path (str): AVI 檔案的路徑。
        output_directory (str): 用於儲存 Packet 檔案的目錄。
    """
    try:
        container = av.open(avi_file_path)
        video_stream = None
        for stream in container.streams:
            print(f"Stream type: {stream.type}, Available attributes: {dir(stream)}") # 偵錯用
            if stream.type == 'video':
                video_stream = stream
                break

        if video_stream is None:
            print("找不到視訊流。")
            return

        os.makedirs(output_directory, exist_ok=True) # 創建輸出目錄，如果已存在則不報錯

        for packet in container.demux(video_stream):
            if packet.stream == video_stream:
                packet_data = bytes(packet)
                filename = os.path.join(output_directory, f"video_packet_pts_{packet.pts}.jpg")
                with open(filename, 'wb') as outfile:
                    outfile.write(packet_data)
                print(f"已儲存 Packet (PTS: {packet.pts}, Length: {len(packet_data)}) 到 {filename}")
            else:
                print(f"警告: 無法從 Packet 中獲取數據 (PTS: {packet.pts})")

    except OSError as e:
        print(f"讀取 AVI 檔案時發生錯誤: {e}")
    finally:
        if 'container' in locals() and container:
            container.close()

if __name__ == "__main__":
    avi_file = './root/movie.avi'  # 將 'your_avi_file.avi' 替換為您的 AVI 檔案路徑
    output_dir = 'video_packets'    # 設定儲存 Packet 檔案的目錄名稱
    save_video_packets_to_files(avi_file, output_dir)
    print(f"視訊 Packets 已儲存到目錄: {output_dir}")