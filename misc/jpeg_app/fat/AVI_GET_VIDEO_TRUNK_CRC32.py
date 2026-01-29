import av
import zlib

def calculate_mjpeg_crc32(avi_file_path):
    """
    Read an AVI file, extract MJPEG video packets, 
    and calculate the CRC32 value for each packet's data.

    Args:
        avi_file_path (str): Path to the AVI file.

    Returns:
        dict: A dictionary where the key is the packet's PTS (timestamp) 
              and the value is the CRC32 of that packet's data.
              Returns an empty dictionary if no MJPEG video stream is found.
    """
    crc32_values = {}

    try:
        container = av.open(avi_file_path)
        video_stream = None

        for stream in container.streams:
            print(f"Stream type: {stream.type}, Available attributes: {dir(stream)}")  # Debug info
            if stream.type == 'video' and getattr(stream.codec_context, 'name', None) == 'mjpeg':
                video_stream = stream
                break

        if video_stream is None:
            print("No MJPEG video stream found.")
            return crc32_values

        for packet in container.demux(video_stream):
            if packet.stream == video_stream:
                # packet.raw_packet.data contains the original compressed data
                mjpeg_data = bytes(packet)
                crc32_value = zlib.crc32(mjpeg_data)
                print(f"PTS: {packet.pts}, Length: {len(mjpeg_data)}, CRC32: {crc32_value:08x}")
                crc32_values[packet.pts] = crc32_value  # Use PTS as key

    except OSError as e:
        print(f"Error occurred while reading AVI file: {e}")

    finally:
        if 'container' in locals() and container:
            container.close()

    return crc32_values


if __name__ == "__main__":
    avi_file = './root/movie.avi'  # Replace with your AVI file path
    crc_values = calculate_mjpeg_crc32(avi_file)
