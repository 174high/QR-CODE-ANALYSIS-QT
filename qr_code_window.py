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
        self.horizontalLayoutWidget = QtWidgets.QWidget(QRCODE)
        self.horizontalLayoutWidget.setGeometry(QtCore.QRect(40, 40, 271, 260))
        self.horizontalLayoutWidget.setObjectName("horizontalLayoutWidget")
        self.horizontalLayout = QtWidgets.QHBoxLayout(self.horizontalLayoutWidget)
        self.horizontalLayout.setContentsMargins(0, 0, 0, 0)
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.CAMERA = QtWidgets.QLabel(self.horizontalLayoutWidget)
        self.CAMERA.setText("")
        self.CAMERA.setPixmap(QtGui.QPixmap("qr-code.jpg"))
        self.CAMERA.setObjectName("CAMERA")
        self.horizontalLayout.addWidget(self.CAMERA)
        self.label = QtWidgets.QLabel(QRCODE)
        self.label.setGeometry(QtCore.QRect(350, 40, 261, 261))
        self.label.setText("")
        self.label.setObjectName("label")
        self.pushButton = QtWidgets.QPushButton(QRCODE)
        self.pushButton.setGeometry(QtCore.QRect(40, 310, 56, 31))
        self.pushButton.setObjectName("pushButton")
        self.pushButton_2 = QtWidgets.QPushButton(QRCODE)
        self.pushButton_2.setGeometry(QtCore.QRect(110, 310, 56, 31))
        self.pushButton_2.setObjectName("pushButton_2")
        self.pushButton_3 = QtWidgets.QPushButton(QRCODE)
        self.pushButton_3.setGeometry(QtCore.QRect(180, 310, 56, 31))
        self.pushButton_3.setObjectName("pushButton_3")
        self.pushButton_4 = QtWidgets.QPushButton(QRCODE)
        self.pushButton_4.setGeometry(QtCore.QRect(250, 310, 56, 31))
        self.pushButton_4.setObjectName("pushButton_4")
        self.label_2 = QtWidgets.QLabel(QRCODE)
        self.label_2.setGeometry(QtCore.QRect(650, 40, 261, 261))
        self.label_2.setText("")
        self.label_2.setObjectName("label_2")

        self.retranslateUi(QRCODE)
        QtCore.QMetaObject.connectSlotsByName(QRCODE)

    def retranslateUi(self, QRCODE):
        _translate = QtCore.QCoreApplication.translate
        QRCODE.setWindowTitle(_translate("QRCODE", "QRCODE"))
        self.pushButton.setText(_translate("QRCODE", "start"))
        self.pushButton_2.setText(_translate("QRCODE", "stop"))
        self.pushButton_3.setText(_translate("QRCODE", "record"))
        self.pushButton_4.setText(_translate("QRCODE", "play"))

