import serial
import struct
import matplotlib.pyplot as plt
import matplotlib.animation as animation

SERIAL_PORT = 'COM19' 
BAUD_RATE = 115200

ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)

fig, ax = plt.subplots()
x_data = list(range(360))
line, = ax.plot(x_data, [0]*360)

ax.set_xlim(0, 360)
ax.set_ylim(-10, 2000) 
ax.set_xlabel("Angle Index (0-359)")
ax.set_ylabel("Distance Diff")

title_text = ax.set_title("Enemy Distance | My Pos: (0, 0)")

buffer = b''
HEADER = b'\xff\xff\xaa\x55'
DATA_SIZE = 724 # 360*2 + 2 + 2

def update(frame):
    global buffer

    if ser.in_waiting > 0:
        buffer += ser.read(ser.in_waiting)
    
    updated = False

    while True:
        idx = buffer.find(HEADER)
        if idx == -1:
            if len(buffer) > 3:
                buffer = buffer[-3:]
            break

        if len(buffer) < idx + len(HEADER) + DATA_SIZE:
            break

        raw_data = buffer[idx + len(HEADER) : idx + len(HEADER) + DATA_SIZE]

        buffer = buffer[idx + len(HEADER) + DATA_SIZE:]

        unpacked = struct.unpack('<360Hhh', raw_data)
        y_data = unpacked[:360]
        my_x = unpacked[360]
        my_y = unpacked[361]

        line.set_ydata(y_data)
        title_text.set_text(f"Enemy Distance | My Pos: ({my_x}, {my_y})")
        updated = True

    if updated:
        return line, title_text
    else:
        return line, title_text

ani = animation.FuncAnimation(fig, update, interval=10, blit=True)

try:
    plt.show()
finally:
    ser.close()