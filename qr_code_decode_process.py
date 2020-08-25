from PyQt5.QtCore import pyqtSlot, QIODevice, QByteArray 
from PyQt5.QtSerialPort import QSerialPortInfo, QSerialPort
from PyQt5.QtWidgets import QWidget, QMessageBox,QProgressDialog
from PyQt5 import QtGui
from PyQt5.QtCore import *
from PyQt5.QtWebEngineWidgets import *
from PyQt5.QtCore import QTimer
from PyQt5.QtGui import QImage

from pyzbar_test import pyzbar
import argparse
from imutils.video import VideoStream
import imutils

from qr_code_window import Ui_QRCODE

import datetime
import signal
import os
import cv2
import numpy as np


class Window(QWidget, Ui_QRCODE):

    def __init__(self, *args, **kwargs):
        super(Window, self).__init__(*args, **kwargs)
        self.setupUi(self)

        self.running = False
        self.playing = False
        self.timer_record = QTimer()  
        self.timer_record.timeout.connect(self.record); 
        self.timer = QTimer()
        self.timer.timeout.connect(self.on_timeout);
        self.timer_play = QTimer()
        self.timer_play.timeout.connect(self.video_play);

        self.vs = VideoStream(src=0).start()
        self.writer = None

        # construct the argument parser and parse the arguments
        ap = argparse.ArgumentParser()
        ap.add_argument("-o", "--output", type=str, default="barcodes.csv",
        help="path to output CSV file containing barcodes")
        args = vars(ap.parse_args())

        self.csv = open(args["output"], "w")
        self.found = set()
  
        self.pushButton.clicked.connect(self.start)
        self.pushButton_2.clicked.connect(self.stop)
        self.pushButton_3.clicked.connect(self.record)
        self.pushButton_4.clicked.connect(self.play)
	
    def start(self):
        if self.playing==True :
            return 
        self.timer.start(100)  
        self.running = True       

    def stop(self):
        self.camera.setPixmap(QtGui.QPixmap("qr-code.jpg"))
        self.decode.setPixmap(QtGui.QPixmap("qr-code.jpg"))
        self.detail.setPixmap(QtGui.QPixmap("qr-code.jpg"))
        self.timer.stop()
        self.timer_record.stop()
        self.timer_play.stop()
        self.running = False 
        self.playing = False 
        self.writer=None 

    def record(self):

        if self.running == False :
            print("not recording")
            return 

        if self.writer == None:
            print("init")
            # store the image dimensions, initialzie the video writer,
            # and construct the zeros array

            fourcc = cv2.VideoWriter_fourcc('m', 'p', '4', 'v')
            self.writer=cv2.VideoWriter('./demoVideo/tmp.avi', fourcc, 10, (640, 480))
            self.timer_record.start(100)               
        else :
            print("recording")
            self.frame_2 = self.vs.read()
            self.frame_2 = imutils.resize(self.frame_2, width=640)
            self.writer.write(self.frame_2)


    def play(self):

        if self.running == True :
            return 

        print("playing")
        self.timer.stop()
        self.timer_record.stop()
        self.running = False  
        self.writer=None 

        self.cap = cv2.VideoCapture('./demoVideo/'+'tmp.avi')
        self.timer_play.start(100)        

    
    def video_play(self):    
        
        ret, img = self.cap.read()

        if ret:
            self.playing=True
            self.camera.setPixmap(self.convertFrame(img))
            frame,result=self.qrcode_decode(img)
            if result != None :
                self.result.setText(result)
                self.textBrowser_2.setText(result) 
            self.decode.setPixmap(self.convertFrame(frame))
            self.detail.setPixmap(self.convertFrame(frame))
          
        else :
            self.playing=False 
            self.cap.release()
            self.timer_play.stop()  


    def convertFrame(self,frame):

        rgbImage = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        h, w, ch = rgbImage.shape
        bytesPerLine = ch * w
        convertToQtFormat = QImage(rgbImage.data, w, h, bytesPerLine, QImage.Format_RGB888)
        p = convertToQtFormat.scaled(280, 280, Qt.KeepAspectRatio)

        return QtGui.QPixmap(p)
       
    def qrcode_decode(self,frame):

      # find the barcodes in the frame and decode each of the barcodes
        barcodes = pyzbar.decode(frame)

        barcodeData=None;
        # loop over the detected barcodes
        for barcode in barcodes:
		# extract the bounding box location of the barcode and draw
		# the bounding box surrounding the barcode on the image
                (x, y, w, h) = barcode.rect
                cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 0, 255), 2)

		# the barcode data is a bytes object so if we want to draw it
		# on our output image we need to convert it to a string first
                barcodeData = barcode.data.decode("utf-8")
                barcodeType = barcode.type

		# draw the barcode data and barcode type on the image
                text = "{} ({})".format(barcodeData, barcodeType)
                cv2.putText(frame, text, (x, y - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

		# if the barcode text is currently not in our CSV file, write
		# the timestamp + barcode to disk and update the set
                if barcodeData not in self.found:
                        self.csv.write("{},{}\n".format(datetime.datetime.now(),
				barcodeData))
                        self.csv.flush()
                        self.found.add(barcodeData)

        return frame , barcodeData 

    def on_timeout(self):
        self.frame = self.vs.read()
        self.frame = imutils.resize(self.frame, width=400)

        self.camera.setPixmap(self.convertFrame(self.frame)) 
        frame,result=self.qrcode_decode(self.frame) 
        if result != None :
                self.result.setText(result)
                self.textBrowser_2.setText(result)
        self.decode.setPixmap(self.convertFrame(frame)) 
        self.detail.setPixmap(self.convertFrame(frame)) 


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
    w.setFixedSize(950,450)
    signal.signal(signal.SIGINT,signal_handler)
    w.show()
    sys.exit(app.exec_())




