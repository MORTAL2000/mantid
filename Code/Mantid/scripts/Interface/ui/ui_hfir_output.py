# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui/hfir_output.ui'
#
# Created: Thu Sep 15 11:34:07 2011
#      by: PyQt4 UI code generator 4.7.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_Frame(object):
    def setupUi(self, Frame):
        Frame.setObjectName("Frame")
        Frame.resize(938, 784)
        Frame.setFrameShape(QtGui.QFrame.NoFrame)
        Frame.setFrameShadow(QtGui.QFrame.Raised)
        Frame.setLineWidth(0)
        self.verticalLayout = QtGui.QVBoxLayout(Frame)
        self.verticalLayout.setMargin(0)
        self.verticalLayout.setObjectName("verticalLayout")
        self.label = QtGui.QLabel(Frame)
        self.label.setObjectName("label")
        self.verticalLayout.addWidget(self.label)
        self.scrollArea = QtGui.QScrollArea(Frame)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.scrollArea.sizePolicy().hasHeightForWidth())
        self.scrollArea.setSizePolicy(sizePolicy)
        self.scrollArea.setWidgetResizable(True)
        self.scrollArea.setObjectName("scrollArea")
        self.scrollAreaWidgetContents = QtGui.QWidget(self.scrollArea)
        self.scrollAreaWidgetContents.setGeometry(QtCore.QRect(0, 0, 936, 625))
        self.scrollAreaWidgetContents.setObjectName("scrollAreaWidgetContents")
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.scrollAreaWidgetContents)
        self.verticalLayout_2.setMargin(0)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.output_text_edit = QtGui.QTextEdit(self.scrollAreaWidgetContents)
        self.output_text_edit.setObjectName("output_text_edit")
        self.verticalLayout_2.addWidget(self.output_text_edit)
        self.scrollArea.setWidget(self.scrollAreaWidgetContents)
        self.verticalLayout.addWidget(self.scrollArea)
        self.rebin_groupbox = QtGui.QGroupBox(Frame)
        self.rebin_groupbox.setObjectName("rebin_groupbox")
        self.verticalLayout_3 = QtGui.QVBoxLayout(self.rebin_groupbox)
        self.verticalLayout_3.setObjectName("verticalLayout_3")
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.n_q_bins_label = QtGui.QLabel(self.rebin_groupbox)
        self.n_q_bins_label.setMinimumSize(QtCore.QSize(150, 0))
        self.n_q_bins_label.setMaximumSize(QtCore.QSize(150, 16777215))
        self.n_q_bins_label.setObjectName("n_q_bins_label")
        self.horizontalLayout.addWidget(self.n_q_bins_label)
        self.n_q_bins_edit = QtGui.QLineEdit(self.rebin_groupbox)
        self.n_q_bins_edit.setMinimumSize(QtCore.QSize(80, 0))
        self.n_q_bins_edit.setMaximumSize(QtCore.QSize(80, 16777215))
        self.n_q_bins_edit.setObjectName("n_q_bins_edit")
        self.horizontalLayout.addWidget(self.n_q_bins_edit)
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.lin_binning_radio = QtGui.QRadioButton(self.rebin_groupbox)
        self.lin_binning_radio.setObjectName("lin_binning_radio")
        self.horizontalLayout.addWidget(self.lin_binning_radio)
        self.log_binning_radio = QtGui.QRadioButton(self.rebin_groupbox)
        self.log_binning_radio.setObjectName("log_binning_radio")
        self.horizontalLayout.addWidget(self.log_binning_radio)
        spacerItem1 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem1)
        self.rebin_button = QtGui.QPushButton(self.rebin_groupbox)
        self.rebin_button.setObjectName("rebin_button")
        self.horizontalLayout.addWidget(self.rebin_button)
        spacerItem2 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem2)
        self.verticalLayout_3.addLayout(self.horizontalLayout)
        self.verticalLayout.addWidget(self.rebin_groupbox)
        self.plot_area_layout = QtGui.QHBoxLayout()
        self.plot_area_layout.setSizeConstraint(QtGui.QLayout.SetDefaultConstraint)
        self.plot_area_layout.setContentsMargins(0, 20, -1, 20)
        self.plot_area_layout.setObjectName("plot_area_layout")
        self.label_2 = QtGui.QLabel(Frame)
        self.label_2.setText("")
        self.label_2.setObjectName("label_2")
        self.plot_area_layout.addWidget(self.label_2)
        self.verticalLayout.addLayout(self.plot_area_layout)

        self.retranslateUi(Frame)
        QtCore.QMetaObject.connectSlotsByName(Frame)

    def retranslateUi(self, Frame):
        Frame.setWindowTitle(QtGui.QApplication.translate("Frame", "Frame", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("Frame", "Reduction Output", None, QtGui.QApplication.UnicodeUTF8))
        self.rebin_groupbox.setTitle(QtGui.QApplication.translate("Frame", "I(Q) rebinning", None, QtGui.QApplication.UnicodeUTF8))
        self.n_q_bins_label.setText(QtGui.QApplication.translate("Frame", "Number of Q bins", None, QtGui.QApplication.UnicodeUTF8))
        self.n_q_bins_edit.setToolTip(QtGui.QApplication.translate("Frame", "Enter the number of Q bins for the output I(Q) distribution.", None, QtGui.QApplication.UnicodeUTF8))
        self.lin_binning_radio.setToolTip(QtGui.QApplication.translate("Frame", "Select to use linear binning for I(Q).", None, QtGui.QApplication.UnicodeUTF8))
        self.lin_binning_radio.setText(QtGui.QApplication.translate("Frame", "Linear Q binning", None, QtGui.QApplication.UnicodeUTF8))
        self.log_binning_radio.setToolTip(QtGui.QApplication.translate("Frame", "Select to use logarithmic binning for I(Q).", None, QtGui.QApplication.UnicodeUTF8))
        self.log_binning_radio.setText(QtGui.QApplication.translate("Frame", "Log Q binning", None, QtGui.QApplication.UnicodeUTF8))
        self.rebin_button.setText(QtGui.QApplication.translate("Frame", "Rebin", None, QtGui.QApplication.UnicodeUTF8))

