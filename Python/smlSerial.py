import serial
import serial.tools.list_ports
from numpy import array, uint64, uint32, uint16, uint8, zeros, append
from zlib import crc32
from time import sleep
import struct
import matplotlib.pyplot as plt
import datetime
from dec2bin import dec2bin


def __sml_fourbytestofloat(data):
    return struct.unpack('<f', data)


def __sml_twobytestodec(data):
    ret = struct.unpack('<H', data)[0]
    return ret


def __sml_fourbytestodec(data):
    ret = struct.unpack('<I', data)[0]
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
        while ser.in_waiting != size16 - 2:
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

    if len(ret) != size16 - 2:
        # size specified by package is not equal to actually received amount of bytes
        return bytes(9)
    # concat size at beginning of serial array
    ret = struct.pack('H',size16)+ret

    # calculate checksum
    crc_calc = __crc32(ret[0:-4])

    # get received checksum
    crc_check = ret[len(ret) - 4:len(ret)]

    crc_check = 0
    for i in range(len(ret) - 4, len(ret)):
        crc_check += ret[i] << 8 * (i - (len(ret) - 4))

    # compare both checksums
    if crc_calc != crc_calc:
        # checksum ist not equal, so discard this package
        return bytes(9)

    if ret[2] != 70:
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
    if ret[2] != 70:
        return __sml_transfer(ser, cmd, major, minor, data, tries)
    print('')
    return ret


def __sml_check_connect(ser, tries=5):
    ret = __sml_transfer(ser, 7, tries=tries)
    if ret[2] != 70:
        return 0
    return 1


def smu_get_unix_time(ser):
    struct = __sml_transfer(ser,9,0,0,0)
    if(struct[2]!=70):
        return smu_get_unix_time(ser)
    return __sml_fourbytestodec((struct[3:7]))


def smu_get_RTC_registers():
    t = datetime.datetime.now(datetime.timezone.utc)

    sec_tens = uint32(t.second / 10)
    sec_ones = uint32(t.second - sec_tens * 10)

    min_tens = uint32(t.minute / 10)
    min_ones = uint32(t.minute - min_tens * 10)

    hr_tens = uint32(t.hour / 10)
    hr_ones = uint32(t.hour - hr_tens * 10)

    day_tens = uint32(t.day / 10)
    day_ones = uint32(t.day - day_tens * 10)

    month_tens = uint32(t.month / 10)
    month_ones = uint32(t.month - month_tens * 10)

    year_thousands = uint32(t.year / 1000)
    year_hundreds = uint32((t.year - year_thousands * 1000) / 100)
    year_tens = uint32((t.year - year_thousands * 1000 - year_hundreds * 100) / 10)
    year_ones = uint32(t.year - year_thousands * 1000 - year_hundreds * 100 - year_tens * 10)

    sec_ones_str = dec2bin(sec_ones)
    sec_tens_str = dec2bin(sec_tens)

    min_ones_str = dec2bin(min_ones)
    min_tens_str = dec2bin(min_tens)

    hr_ones_str = dec2bin(hr_ones)
    hr_tens_str = dec2bin(hr_tens)

    bcd_TR = (sec_ones + sec_tens * (2**4) + min_ones * (2**8) + min_tens * (2**12) + hr_ones * (2**16) + hr_tens * (2**20))

    day_of_week = t.weekday() - 1;

    if (day_of_week == 0):
        day_of_week = 7

    bcd_DR = (day_ones + day_tens * (2**4) + month_ones * (2**8) + month_tens * (2**12) + day_of_week * (2**13) + year_ones * (2**16) + year_tens * (2**20))

    ret = [bcd_DR,bcd_TR]

    return ret

def smu_set_RTC(ser):
    reg = smu_get_RTC_registers()
    struct =__sml_transfer(ser,8,0,0,uint32(reg))
    if(struct[2]!=70):
        smu_set_RTC(ser)
        return


def sml_connect():
    """Tries to connect to any
    connected SMU"""
    print("Looking for Smart Meter Unit....");
    slist = serial.tools.list_ports.comports(include_links=False);
    print(len(slist).__str__() + " COM Ports found!")
    for k in range(0, len(slist)):
        print("checking " + slist[k].device, end='')
        ser = serial.Serial(slist[k].device, 1000000, timeout=0);
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

smu_get_RTC_registers()
