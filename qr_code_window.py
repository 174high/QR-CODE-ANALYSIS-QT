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
        QRCODE.resize(951, 625)
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

        self.retranslateUi(QRCODE)
        QtCore.QMetaObject.connectSlotsByName(QRCODE)

    def retranslateUi(self, QRCODE):
        _translate = QtCore.QCoreApplication.translate
        QRCODE.setWindowTitle(_translate("QRCODE", "QRCODE"))

