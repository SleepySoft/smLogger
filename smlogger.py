import os
import sys
import time
import traceback
from ctypes import *


def main():
    smlogger = cdll.LoadLibrary("libsmLogger/libsmLogger.so")
    if smlogger is None:
        return False
    smlogger.initLogger('/tmp/iplog.txt'.encode('utf-8'), 10 * 1024, 10)
    read_rounter = 0
    buffer = bytes(1024)
    while True:
        if smlogger.logInited():
            readed = smlogger.logRead(buffer, 1024)
            if readed > 0:
                print(buffer.decode('utf-8'))
                read_rounter = read_rounter + 1
                if read_rounter % 10:
                    trigger_text = '-----> Trigger from Python ' + str(read_rounter)
                    smlogger.logWrite(trigger_text.encode('utf-8'), len(trigger_text))
            else:
                time.sleep(0.01)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print('Error =>', e)
        print('Error =>', traceback.format_exc())
        exit()
    finally:
        pass