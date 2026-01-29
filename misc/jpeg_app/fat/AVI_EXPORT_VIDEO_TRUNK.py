import av
import os

def save_video_packets_to_files(avi_file_path, output_directory):
    """
    Read an AVI file, extract the raw binary data of each video packet,
    and save each packet as a separate file.

    Args:
        avi_file_path (str): Path to the AVI file.
        output_directory (str): Directory used to store the packet files.
    """
    try:
        container = av.open(avi_file_path)
        video_stream = None

        for stream in container.streams:
            print(f"Stream type: {stream.type}, Available attributes: {dir(stream)}")  # Debug use
            if stream.type == 'video':
                video_stream = stream
                break

        if video_stream is None:
            print("No video stream found.")
            return

        # Create output directory (do not error if it already exists)
        os.makedirs(output_directory, exist_ok=True)

        for packet in container.demux(video_stream):
            if packet.stream == video_stream:
                packet_data = bytes(packet)
                filename = os.path.join(
                    output_directory,
                    f"video_packet_pts_{packet.pts}.jpg"
                )

                with open(filename, 'wb') as outfile:
                    outfile.write(packet_data)

                print(
                    f"Saved packet (PTS: {packet.pts}, Length: {len(packet_data)}) to {filename}"
                )
            else:
                print(
                    f"Warning: Unable to get data from packet (PTS: {packet.pts})"
                )

    except OSError as e:
        print(f"Error occurred while reading AVI file: {e}")

    finally:
        if 'container' in locals() and container:
            container.close()


if __name__ == "__main__":
    avi_file = './root/movie.avi'   # Replace with your AVI file path
    output_dir = 'video_packets'    # Directory for saving packet files

    save_video_packets_to_files(avi_file, output_dir)
    print(f"Video packets have been saved to directory: {output_dir}")
