# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'qr_code.ui'
#
# Created by: PyQt5 UI code generator 5.11.3
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_QRCODE(object):
    def setupUi(self, QRCODE):
        QRCODE.setObjectName("QRCODE")
        QRCODE.resize(951, 449)
        self.decode = QtWidgets.QLabel(QRCODE)
        self.decode.setGeometry(QtCore.QRect(340, 40, 261, 261))
        self.decode.setText("")
        self.decode.setPixmap(QtGui.QPixmap("qr-code.jpg"))
        self.decode.setObjectName("decode")
        self.detail = QtWidgets.QLabel(QRCODE)
        self.detail.setGeometry(QtCore.QRect(640, 40, 261, 261))
        self.detail.setText("")
        self.detail.setPixmap(QtGui.QPixmap("qr-code.jpg"))
        self.detail.setObjectName("detail")
        self.result = QtWidgets.QTextBrowser(QRCODE)
        self.result.setGeometry(QtCore.QRect(340, 310, 261, 101))
        self.result.setObjectName("result")
        self.textBrowser_2 = QtWidgets.QTextBrowser(QRCODE)
        self.textBrowser_2.setGeometry(QtCore.QRect(640, 310, 261, 101))
        self.textBrowser_2.setObjectName("textBrowser_2")
        self.camera = QtWidgets.QLabel(QRCODE)
        self.camera.setGeometry(QtCore.QRect(40, 40, 261, 261))
        self.camera.setText("")
        self.camera.setPixmap(QtGui.QPixmap("qr-code.jpg"))
        self.camera.setObjectName("camera")
        self.pushButton = QtWidgets.QPushButton(QRCODE)
        self.pushButton.setGeometry(QtCore.QRect(40, 310, 51, 41))
        self.pushButton.setObjectName("pushButton")
        self.pushButton_2 = QtWidgets.QPushButton(QRCODE)
        self.pushButton_2.setGeometry(QtCore.QRect(110, 310, 51, 41))
        self.pushButton_2.setObjectName("pushButton_2")
        self.pushButton_3 = QtWidgets.QPushButton(QRCODE)
        self.pushButton_3.setGeometry(QtCore.QRect(180, 310, 51, 41))
        self.pushButton_3.setObjectName("pushButton_3")
        self.pushButton_4 = QtWidgets.QPushButton(QRCODE)
        self.pushButton_4.setGeometry(QtCore.QRect(250, 310, 51, 41))
        self.pushButton_4.setObjectName("pushButton_4")

        self.retranslateUi(QRCODE)
        QtCore.QMetaObject.connectSlotsByName(QRCODE)

    def retranslateUi(self, QRCODE):
        _translate = QtCore.QCoreApplication.translate
        QRCODE.setWindowTitle(_translate("QRCODE", "QRCODE"))
        self.pushButton.setText(_translate("QRCODE", "start"))
        self.pushButton_2.setText(_translate("QRCODE", "stop"))
        self.pushButton_3.setText(_translate("QRCODE", "record"))
        self.pushButton_4.setText(_translate("QRCODE", "play"))

