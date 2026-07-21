import sys
import struct
import time


def send_kernel(serial_port, kernel_path):
    with open(kernel_path, "rb") as f:
        kernel_data = f.read()

    kernel_size = len(kernel_data)
    print(f"Kernel size: {kernel_size} bytes")

    magic = 0x544F4F42
    header = struct.pack("<II", magic, kernel_size)

    with open(serial_port, "wb", buffering=0) as tty:
        print("Sending header...")
        tty.write(header)
        time.sleep(0.1)

        print("Sending kernel data...")
        tty.write(kernel_data)
        print("Transmission complete!")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 send_kernel.py <serial_port> <kernel_bin_path>")
        sys.exit(1)
    send_kernel(sys.argv[1], sys.argv[2])
