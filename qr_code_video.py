from PyQt5.QtCore import pyqtSlot, QIODevice, QByteArray 
from PyQt5.QtSerialPort import QSerialPortInfo, QSerialPort
from PyQt5.QtWidgets import QWidget, QMessageBox,QProgressDialog
from PyQt5 import QtGui
from PyQt5.QtCore import *
from PyQt5.QtWebEngineWidgets import *
from PyQt5.QtCore import QTimer
from PyQt5.QtGui import QImage

from imutils.video import VideoStream
import imutils

from qr_code_window import Ui_QRCODE

import datetime
import signal
import os
import cv2

class Window(QWidget, Ui_QRCODE):

    def __init__(self, *args, **kwargs):
        super(Window, self).__init__(*args, **kwargs)
        self.setupUi(self)

        self.vs = VideoStream(src=0).start()

        self.timer = QTimer()
        self.timer.timeout.connect(self.on_timeout);
        self.timer.start(100)
	

#    def request_will_be_sent(**kwargs):
#        print("loading: %s" % kwargs.get('request').get('url'))

    def on_timeout(self):
        self.frame = self.vs.read()
        self.frame = imutils.resize(self.frame, width=400)

        rgbImage = cv2.cvtColor(self.frame, cv2.COLOR_BGR2RGB)
        h, w, ch = rgbImage.shape
        bytesPerLine = ch * w
        convertToQtFormat = QImage(rgbImage.data, w, h, bytesPerLine, QImage.Format_RGB888)
        p = convertToQtFormat.scaled(640, 480, Qt.KeepAspectRatio)
        self.label.setPixmap(QtGui.QPixmap(p)) 

'''
        cap = cv2.VideoCapture(0)
        while True:
            ret, frame = cap.read()
            if ret:
                # https://stackoverflow.com/a/55468544/6622587
                rgbImage = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                h, w, ch = rgbImage.shape
                bytesPerLine = ch * w
                convertToQtFormat = QImage(rgbImage.data, w, h, bytesPerLine, QImage.Format_RGB888)
                p = convertToQtFormat.scaled(640, 480, Qt.KeepAspectRatio)
                self.label.setPixmap(QtGui.QPixmap(p))      
'''


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




