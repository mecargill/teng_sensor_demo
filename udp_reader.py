import csv
import os
import socket
import struct
import threading
from collections import deque
from datetime import datetime

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation


UDP_BIND_ADDRESS = "0.0.0.0"
UDP_PORT = 6000
MAX_PACKET_SIZE = 2048
SOCKET_TIMEOUT_SECONDS = 1.0
PLOT_SAMPLE_COUNT = 4000

START_BYTES = b"\xAB\xAB"
END_BYTES = b"\xCD\xCD"

output_directory = "adc_data"
os.makedirs(output_directory, exist_ok=True)

plot_samples = deque(maxlen=PLOT_SAMPLE_COUNT)
state_lock = threading.Lock()
stop_event = threading.Event()

recording_file = None
recording_writer = None
packet_count = 0
total_sample_count = 0
last_sender = None


def parse_packet(packet):
    """Return the ADC samples in one ESP32 packet, or None if it is invalid."""
    if len(packet) < len(START_BYTES) + 2 + len(END_BYTES):
        return None
    if not packet.startswith(START_BYTES) or not packet.endswith(END_BYTES):
        return None

    num_samples = struct.unpack_from("<H", packet, len(START_BYTES))[0]
    payload_start = len(START_BYTES) + 2
    payload_end = payload_start + num_samples * 2
    expected_length = payload_end + len(END_BYTES)

    if len(packet) != expected_length:
        return None

    return list(struct.unpack_from(f"<{num_samples}H", packet, payload_start))


def start_recording():
    global recording_file, recording_writer

    with state_lock:
        if recording_file is not None:
            return recording_file.name

        output_file = os.path.join(
            output_directory,
            "adc_data_" + datetime.now().strftime("%Y-%m-%d_%H_%M_%S.csv"),
        )
        recording_file = open(output_file, "w", newline="")
        recording_writer = csv.writer(recording_file)
        return output_file


def stop_recording():
    global recording_file, recording_writer

    with state_lock:
        if recording_file is not None:
            recording_file.close()
            recording_file = None
            recording_writer = None


def receiver_loop(sock):
    global packet_count, total_sample_count, last_sender

    while not stop_event.is_set():
        try:
            packet, sender = sock.recvfrom(MAX_PACKET_SIZE)
        except socket.timeout:
            continue
        except OSError:
            break

        samples = parse_packet(packet)
        if samples is None:
            print(f"Ignoring invalid packet of {len(packet)} bytes from {sender}")
            continue

        with state_lock:
            plot_samples.extend(samples)
            packet_count += 1
            total_sample_count += len(samples)
            last_sender = sender

            if recording_writer is not None:
                recording_writer.writerow(samples)
                recording_file.flush()


def update_plot(_frame, line, axis, status_text):
    with state_lock:
        samples = list(plot_samples)
        packets = packet_count
        total_samples = total_sample_count
        sender = last_sender
        is_recording = recording_file is not None

    if samples:
        first_x = total_samples - len(samples)
        line.set_data(range(first_x, total_samples), samples)
        axis.set_xlim(first_x, total_samples)

    status = "Recording" if is_recording else "Not recording"
    source = f" | source: {sender[0]}:{sender[1]}" if sender else ""
    status_text.set_text(
        f"{status} | packets: {packets} | samples: {total_samples}{source}"
    )
    return line, status_text


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.settimeout(SOCKET_TIMEOUT_SECONDS)
sock.bind((UDP_BIND_ADDRESS, UDP_PORT))

receiver = threading.Thread(target=receiver_loop, args=(sock,), daemon=True)
receiver.start()

figure, axis = plt.subplots()
line, = axis.plot([], [], linewidth=1)
status_text = figure.text(0.02, 0.02, "Not recording")
axis.set_title("ESP32 ADC live data")
axis.set_xlabel("Sample")
axis.set_ylabel("ADC value")
axis.set_ylim(0, 4095)
axis.set_xlim(0, PLOT_SAMPLE_COUNT)
axis.grid(False)


def on_key(event):
    if event.key == "r":
        output_file = start_recording()
        print(f"Recording started: {output_file}")
    elif event.key == "e":
        stop_recording()
        print("Recording stopped")
    elif event.key == "q":
        stop_event.set()
        plt.close(figure)


def on_close(_event):
    stop_event.set()


figure.canvas.mpl_connect("key_press_event", on_key)
figure.canvas.mpl_connect("close_event", on_close)
animation = FuncAnimation(
    figure,
    update_plot,
    fargs=(line, axis, status_text),
    interval=50,
    cache_frame_data=False,
)

print(f"Listening for UDP packets on {UDP_BIND_ADDRESS}:{UDP_PORT}")
print("Press r in the graph to start recording, e to end recording, or q to quit")

try:
    plt.show()
finally:
    stop_event.set()
    sock.close()
    receiver.join(timeout=2)
    stop_recording()
    print(f"Received {packet_count} packets, {total_sample_count} samples")
    print("Socket closed")
