import serial
import serial.tools.list_ports
from numpy import array, uint64, uint32, uint16, uint8, zeros, append
from zlib import crc32
from time import sleep
import struct
import matplotlib.pyplot as plt



def __sml_fourbytestofloat(data):
    return struct.unpack('<f', data)


def __sml_twobytestodec(data):
    ret = struct.unpack('<H', data)[0]
    return ret


def __receive_ack_struct(ser):
    timeout = 100
    i = 1
    error = 0

    while timeout > 0:
        size = ser.read(2)
        sleep(0.001)
        timeout = timeout - 1
        if len(size) == 2:
            break
        if timeout == 0:
            return bytes(9)

    size16 = __sml_twobytestodec(size)

    if ser.in_waiting > 0:
        while ser.in_waiting != size16 - 1:
            # wait until the correct amount of data is in the buffer
            # or wait for timeout
            timeout = timeout - 1
            # wait 1 ms
            sleep(0.001)
            if timeout == 0:
                return bytes(9)

        ret = ser.read(ser.in_waiting)
    else:
        return bytes(8)

    if len(ret) != size16 - 1:
        # size specified by package is not equal to actually received amount of bytes
        return bytes(9)
    # concat size at beginning of serial array
    ret = size16 + ret

    # calculate checksum
    crc_calc = __crc32(ret[0:-4])

    # get received checksum
    crc_check = ret[len(ret) - 4:len(ret)]

    crc_check = 0
    for i in range(len(ret) - 4, len(ret)):
        crc_check += ret[i] << 8 * (i - (len(ret) - 4))

    # compare both checksums
    if crc_calc != crc_check:
        # checksum ist not equal, so discard this package
        return bytes(9)

    if ret[1] != 50 and ret[1] != 70:
        # if the ack byte isn't the ACK value (50 for controllino and 90 for prototypes)
        # then discard the package
        return bytes(9)

    return ret


def __send_cmd_struct(ser, data):
    ser.write(data)
    return


def __crc32(data):
    amt = 4 - (len(data) % 4)
    data = data + bytes(amt)
    temp_crc32 = crc32(data)
    return temp_crc32


def __create_cmd_struct(cmd, major, minor, data):
    tmp = array([data], dtype=uint32).tobytes()
    vec = bytearray(11 + len(tmp))
    vec[0] = 11 + len(tmp)
    vec[1] = cmd
    tmp = array([major], dtype=uint16).tobytes()
    vec[2:4] = tmp
    tmp = array([minor], dtype=uint16).tobytes()
    vec[4:6] = tmp
    if type(data) is float:
        tmp = array([data], dtype='single').tobytes()
    else:
        tmp = array([data]).tobytes()
    for i in range(0, len(tmp)):
        vec[i + 6] = tmp[i]
        # 234 is the delimiter and always at the last 5th place of vec
    vec[-5] = 234
    temp_crc32 = __crc32(vec[0:-4])
    tmp = array([temp_crc32], dtype=uint32).tobytes()
    vec[11:15] = tmp

    return vec


def __sml_transfer(ser, cmd, major=0, minor=0, data=0, tries=5):
    """Transfer function for all SMU:
    Creates and transmit the protocol frame with the given parameters
    and transmits it over the serial object.
    Then it tries to receive an ack frame and if the ack frame wasn't
    received or ist corrupted it tries again 'tries' times"""
    if tries == 0:
        print('')
        print("Tried " + tries.__str__() + " times, got no ACK! Aborting...")
        return bytes(9)
    print(".", end='')
    tries = tries - 1
    ret_struct = __create_cmd_struct(cmd, major, minor, data)
    __send_cmd_struct(ser, ret_struct)
    ret = __receive_ack_struct(ser)
    if ret[1] != 70:
        return __sml_transfer(ser, cmd, major, minor, data, tries)
    print('')
    return ret


def __sml_check_connect(ser, tries=5):
    ret = __sml_transfer(ser, 7, tries=tries)
    if ret[1] != 70:
        return 0
    return 1


def sml_connect():
    """Tries to connect to any
    connected SMU"""
    print("Looking for Smart Meter Unit....");
    slist = serial.tools.list_ports.comports(include_links=False);
    print(len(slist).__str__() + " COM Ports found!")
    for k in range(0, len(slist)):
        print("checking " + slist[k].device, end='')
        ser = serial.Serial(slist[k].device, 3000000, timeout=0);
        if not ser.isOpen():
            # serial port is closed, so open it
            ser.open()
            uuid = __sml_check_connect(ser, 5)
            if uuid == 1:
                print("SMU found! :)")
                return ser
            ser.close()
        else:

            uuid = __sml_check_connect(ser, 5)
            if uuid == 1:
                print("SMU found! :)")
                return ser
