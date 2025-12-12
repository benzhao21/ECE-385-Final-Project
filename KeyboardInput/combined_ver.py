import threading
import queue
import serial
import time
import win32gui
import win32con
from ctypes import *
from ctypes.wintypes import *

# this stuff is from online source
RID_INPUT = 0x10000003
RIDEV_INPUTSINK = 0x00000100
WM_INPUT = 0x00FF
RIDI_DEVICENAME = 0x20000007

class RAWINPUTDEVICE(Structure):
    _fields_ = [("usUsagePage", USHORT),
                ("usUsage", USHORT),
                ("dwFlags", DWORD),
                ("hwndTarget", HWND)]

class RAWKEYBOARD(Structure):
    _fields_ = [("MakeCode", USHORT),
                ("Flags", USHORT),
                ("Reserved", USHORT),
                ("VKey", USHORT),
                ("Message", UINT),
                ("ExtraInformation", ULONG)]

class RAWINPUTHEADER(Structure):
    _fields_ = [("dwType", DWORD),
                ("dwSize", DWORD),
                ("hDevice", HANDLE),
                ("wParam", WPARAM)]

class RAWINPUT(Structure):
    _fields_ = [("header", RAWINPUTHEADER),
                ("keyboard", RAWKEYBOARD)]

#global Vars

event_queue = queue.Queue()
device_names = {}
key_state = {}

# pairing state machine and device vars
PAIRING_STAGE = 0
player1_device = None
player2_device = None

VK_ENTER = 0x0D

# =============================================== code below

def get_device_name(hDevice): #defines player 1 vs player 2 HID/VID
    size = UINT(0)
    windll.user32.GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, None, byref(size))
    buf = create_unicode_buffer(size.value)
    windll.user32.GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, buf, byref(size))
    return buf.value

# ============================= raw input

def wnd_proc(hwnd, msg, wparam, lparam): #define our own wnd_proc for WM_INPUT messages
    global PAIRING_STAGE, player1_device, player2_device #mark external

    if msg == WM_INPUT:
        size = UINT(0)
        windll.user32.GetRawInputData(lparam, RID_INPUT, None, byref(size), sizeof(RAWINPUTHEADER))
        buf = create_string_buffer(size.value)
        windll.user32.GetRawInputData(lparam, RID_INPUT, buf, byref(size), sizeof(RAWINPUTHEADER)) #get raw input data

        raw = RAWINPUT.from_buffer_copy(buf)

        if raw.header.dwType == 1:  # keyboard
            hDev = raw.header.hDevice #parse info
            vkey = raw.keyboard.VKey
            pressed = raw.keyboard.Message == win32con.WM_KEYDOWN
            
            
            if hDev not in device_names: #if we haven't seen this device yet add to the dictonary
                device_names[hDev] = get_device_name(hDev)

            # ================ pairing
            if pressed and vkey == VK_ENTER:

                if PAIRING_STAGE == 0: #define player 1
                    player1_device = hDev
                    PAIRING_STAGE = 1
                    print(f"[PAIR] Player 1 = {device_names[hDev]}")
                    return 0

                elif PAIRING_STAGE == 1 and hDev != player1_device: #define player 2
                    player2_device = hDev
                    PAIRING_STAGE = 2
                    print(f"[PAIR] Player 2 = {device_names[hDev]}")
                    print("[PAIR] Pairing complete. Game starting!")
                    return 0

            # after pairing send events to queue for serial to avoid race conditions
            if PAIRING_STAGE == 2:
                event_queue.put((hDev, vkey, pressed))

    return win32gui.DefWindowProc(hwnd, msg, wparam, lparam)





# ============================== raw input thread

def rawinput_thread():
    wc = win32gui.WNDCLASS() #I'm ngl i found ts online I'm really not sure whats happening here
    wc.lpfnWndProc = wnd_proc
    wc.lpszClassName = "RawInputClass"
    win32gui.RegisterClass(wc)

    hwnd = win32gui.CreateWindow( #make invisible window for input
        wc.lpszClassName, "RawInputWindow",
        0, 0, 0, 100, 100,
        0, 0, 0, None
    )

    rid = RAWINPUTDEVICE() #keyboard specifications
    rid.usUsagePage = 0x01
    rid.usUsage = 0x06
    rid.dwFlags = RIDEV_INPUTSINK
    rid.hwndTarget = hwnd

    windll.user32.RegisterRawInputDevices(byref(rid), 1, sizeof(RAWINPUTDEVICE))

    print("Press ENTER on the P1 keyboard, then ENTER on the P2 keyboard...")
    win32gui.PumpMessages() #this part is a blocking run so we need to make this part a seperate thread

    


# serial communication

def serial_thread(): #this stuff is chill, just connect to FPGA COM port
    ser = serial.Serial("COM3", 115200, timeout=0.1)
    print("[Serial] Connected to FPGA.")

    time.sleep(0.01)

    while True:
        hDev, vkey, pressed = event_queue.get() #get latest input thread events

        # map to player
        if hDev == player1_device:
            player = 1
        elif hDev == player2_device:
            player = 2
        else:
            continue  # ignore other keyboards

        state = 1 if pressed else 0

        print(f"[Serial] P{player} key {hex(vkey)} {'DOWN' if state else 'UP'}")
        
        packet = bytes([player, vkey, state])
        ser.write(packet)

        time.sleep(0.002)

        if ser.in_waiting:
            print("[Serial] FPGA:", ser.read(3)) #print readback(won't actually need this)

# main below

if __name__ == "__main__":

    
    threading.Thread(target=serial_thread, daemon=True).start() #spawn thread for serial com
    rawinput_thread() # run raw input thread
