from PyQt5.QtCore import pyqtSlot, QIODevice, QByteArray 
from PyQt5.QtSerialPort import QSerialPortInfo, QSerialPort
from PyQt5.QtWidgets import QWidget, QMessageBox,QProgressDialog
from PyQt5 import QtGui
from PyQt5.QtCore import *
from PyQt5.QtWebEngineWidgets import *
from PyQt5.QtCore import QTimer
from PyQt5.QtGui import QImage

from pyzbar import pyzbar
import argparse
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

        self.CAMERA.setPixmap(QtGui.QPixmap("qr-code.jpg"))
        self.label.setPixmap(QtGui.QPixmap("qr-code.jpg"))

        self.vs = VideoStream(src=0).start()

        # construct the argument parser and parse the arguments
        ap = argparse.ArgumentParser()
        ap.add_argument("-o", "--output", type=str, default="barcodes.csv",
        help="path to output CSV file containing barcodes")
        args = vars(ap.parse_args())

        self.csv = open(args["output"], "w")
        self.found = set()
  
        self.pushButton.clicked.connect(self.start)
        self.pushButton_2.clicked.connect(self.stop)
#        self.pushButton_3.clicked.connect(self.record)
#        self.pushButton_4.clicked.connect(self.play)
	
    def start(self):
        self.timer = QTimer()
        self.timer.timeout.connect(self.on_timeout);
        self.timer.start(100)        

    def stop(self):
        self.CAMERA.setPixmap(QtGui.QPixmap("qr-code.jpg"))
        self.label.setPixmap(QtGui.QPixmap("qr-code.jpg"))
        self.timer.stop()

#    def start(self):
        

#    def play(self):

#    def request_will_be_sent(**kwargs):
#        print("loading: %s" % kwargs.get('request').get('url'))

    def on_timeout(self):
        self.frame = self.vs.read()
        self.frame = imutils.resize(self.frame, width=400)

        rgbImage = cv2.cvtColor(self.frame, cv2.COLOR_BGR2RGB)
        h, w, ch = rgbImage.shape
        bytesPerLine = ch * w
        convertToQtFormat = QImage(rgbImage.data, w, h, bytesPerLine, QImage.Format_RGB888)
        #p = convertToQtFormat.scaled(640, 480, Qt.KeepAspectRatio)
        self.CAMERA.setPixmap(QtGui.QPixmap(convertToQtFormat)) 


        # find the barcodes in the frame and decode each of the barcodes
        barcodes = pyzbar.decode(self.frame)

    	# loop over the detected barcodes
        for barcode in barcodes:
		# extract the bounding box location of the barcode and draw
		# the bounding box surrounding the barcode on the image
                (x, y, w, h) = barcode.rect
                cv2.rectangle(self.frame, (x, y), (x + w, y + h), (0, 0, 255), 2)

		# the barcode data is a bytes object so if we want to draw it
		# on our output image we need to convert it to a string first
                barcodeData = barcode.data.decode("utf-8")
                barcodeType = barcode.type

		# draw the barcode data and barcode type on the image
                text = "{} ({})".format(barcodeData, barcodeType)
                cv2.putText(self.frame, text, (x, y - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

		# if the barcode text is currently not in our CSV file, write
		# the timestamp + barcode to disk and update the set
                if barcodeData not in self.found:
                        self.csv.write("{},{}\n".format(datetime.datetime.now(),
				barcodeData))
                        self.csv.flush()
                        self.found.add(barcodeData)

	# show the output frame
        #cv2.imshow("Barcode Scanner", self.frame)

        rgbImage = cv2.cvtColor(self.frame, cv2.COLOR_BGR2RGB)
        h, w, ch = rgbImage.shape
        bytesPerLine = ch * w
        convertToQtFormat = QImage(rgbImage.data, w, h, bytesPerLine, QImage.Format_RGB888)
        #p = convertToQtFormat.scaled(640, 480, Qt.KeepAspectRatio)
        self.label.setPixmap(QtGui.QPixmap(convertToQtFormat)) 


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
    w.setFixedSize(950,400)
    signal.signal(signal.SIGINT,signal_handler)
    w.show()
    sys.exit(app.exec_())




