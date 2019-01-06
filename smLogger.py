import os
import sys
import time
import traceback
from ctypes import *


class smLogger:

    ROBOT_LIBRARY_VERSION = 1.0
    ROBOT_LIBRARY_SCOPE = 'GLOBAL'

    def __init__(self):
        __here = os.path.abspath(os.path.dirname(__file__))
        smloggerlib = os.path.join(__here, 'libsmLogger.so')

        self.__smlogger = None
        self.__try_to_load_lib(smloggerlib)

        if self.__smlogger is None:
            smloggerlib = os.path.join(__here, 'libsmLogger/libsmLogger.so')
            self.__try_to_load_lib(smloggerlib)

        if self.__smlogger is not None:
            self.__smlogger.initLogger('/tmp/iplog.txt'.encode('utf-8'), 1 * 1024 * 1024, 10)

    def read_log(self, size: int=1024) -> str:
        readed = 0
        buffer = bytes(size)
        if self.__smlogger is not None and self.__smlogger.logInited():
            readed = self.__smlogger.logRead(buffer, size)
        return '' if readed <= 0 else buffer.partition(b'\0')[0].decode('utf-8')

    def dump_log(self, prefix: str='', suffix: str='', size: int=1024) -> bool:
        logs = self.read_log(size)
        if len(logs) > 0:
            print(prefix + logs + suffix)
            return True
        return False

    def dump_all_log(self, prefix: str='', suffix: str='') -> bool:
        ret = False
        while True:
            logs = self.read_log(1024)
            if len(logs) > 0:
                ret = True
                print(prefix + logs + suffix)
            else:
                break
        return ret

    def trigger_log(self, trigger_text):
        if self.__smlogger is not None and self.__smlogger.logInited():
            self.__smlogger.logWrite(trigger_text.encode('utf-8'), len(trigger_text))

    def __try_to_load_lib(self, path):
        try:
            self.__smlogger = cdll.LoadLibrary(path)
        except Exception as e:
            pass
        finally:
            pass


def main():
    sm_logger = smLogger()

    read_rounter = 0
    while True:
        if sm_logger.dump_log():
            read_rounter = read_rounter + 1
            if read_rounter % 10:
                trigger_text = '-----> Trigger from Python ' + str(read_rounter)
                sm_logger.trigger_log(trigger_text)
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