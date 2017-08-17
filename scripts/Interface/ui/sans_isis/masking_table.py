from __future__ import (absolute_import, division, print_function)
import ui_masking_table
from PyQt4 import QtGui
from abc import ABCMeta, abstractmethod
from six import with_metaclass


class MaskingTable(QtGui.QWidget, ui_masking_table.Ui_MaskingTable):
    class MaskingTableListener(with_metaclass(ABCMeta, object)):
        """
        Defines the elements which a presenter can listen to for the masking table
        """
        @abstractmethod
        def on_row_changed(self):
            pass

        @abstractmethod
        def on_update_rows(self):
            pass

        @abstractmethod
        def on_display(self):
            pass

    def __init__(self, parent=None):
        super(MaskingTable, self).__init__(parent)
        self.setupUi(self)

        # Hook up signal and slots
        self.connect_signals()
        self._masking_tab_listeners = []

    def add_listener(self, listener):
        if not isinstance(listener, MaskingTable.MaskingTableListener):
            raise ValueError("The listener ist not of type MaskingTableListener but rather {}".format(type(listener)))
        self._masking_tab_listeners.append(listener)

    def clear_listeners(self):
        self._masking_tab_listeners = []

    def _call_masking_tab_listeners(self, target):
        for listener in self._masking_tab_listeners:
            target(listener)

    def on_row_changed(self):
        self._call_masking_tab_listeners(lambda listener: listener.on_row_changed())

    def on_update_rows(self):
        self._call_masking_tab_listeners(lambda listener: listener.update_rows())

    def on_display(self):
        self._call_masking_tab_listeners(lambda listener: listener.on_display())

    def connect_signals(self):
        self.select_row_combo_box.currentIndexChanged.connect(self.on_row_changed)

    # ------------------------------------------------------------------------------------------------------------------
    # Actions
    # ------------------------------------------------------------------------------------------------------------------
    def get_current_row(self):
        value = self.select_row_combo_box.currentText()
        if not value:
            value = -1
        return int(value)

    def set_row(self, index):
        found_index = self.select_row_combo_box.findText(str(index))
        if found_index and found_index != -1:
            self.select_row_combo_box.setCurrentIndex(found_index)

    def update_rows(self, indices):
        self.select_row_combo_box.blockSignals(True)
        self.select_row_combo_box.clear()
        for index in indices:
            self.select_row_combo_box.addItem(str(index))
        self.select_row_combo_box.blockSignals(False)

    def set_table(self, table_entries):
        # Remove all rows
        for index in reversed(range(self.masking_table.rowCount())):
            self.masking_table.removeRow(index)

        # Set the number of rows
        self.masking_table.setRowCount(len(table_entries))

        # Populate the rows
        for row, table_entry in enumerate(table_entries):
            entry_type = QtGui.QTableWidgetItem(table_entry.first)
            entry_detector = QtGui.QTableWidgetItem(table_entry.second)
            entry_detail = QtGui.QTableWidgetItem(table_entry.third)

            self.masking_table.setItem(row, 0, entry_type)
            self.masking_table.setItem(row, 1, entry_detector)
            self.masking_table.setItem(row, 2, entry_detail)