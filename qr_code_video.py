from PyQt5.QtCore import pyqtSlot, QIODevice, QByteArray 
from PyQt5.QtSerialPort import QSerialPortInfo, QSerialPort
from PyQt5.QtWidgets import QWidget, QMessageBox,QProgressDialog
from PyQt5 import QtGui
from PyQt5.QtCore import *
from PyQt5.QtWebEngineWidgets import *
from PyQt5.QtCore import QTimer


from qr_code_window import Ui_QRCODE

import datetime
import signal
import os

class Window(QWidget, Ui_QRCODE):

    def __init__(self, *args, **kwargs):
        super(Window, self).__init__(*args, **kwargs)
        self.setupUi(self)

   
#    def request_will_be_sent(**kwargs):
#        print("loading: %s" % kwargs.get('request').get('url'))


def signal_handler(signal,frame):
    print('You pressed Ctrl+C!')
#    tab.stop()
    sys.exit(0)


if __name__ == '__main__':
    import sys
    import cgitb
    sys.excepthook = cgitb.enable(1, None, 5, '')
    from PyQt5.QtWidgets import QApplication

    app = QApplication(sys.argv)
    w = Window()
    w.setFixedSize(850,600)
    signal.signal(signal.SIGINT,signal_handler)
    w.show()
    sys.exit(app.exec_())




