// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_SLICESELECTOR_H_
#define MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_SLICESELECTOR_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "ui_SliceSelector.h"
// Mantid headers from other projects
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
// includes for interface
#include "DllConfig.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include <QMainWindow>

// 3rd party library headers
#include <qwt_plot_spectrogram.h>

// Forward Declarations
namespace MantidQt {
namespace MantidWidgets {
class MWView;
class RangeSelector;
} // namespace MantidWidgets
} // namespace MantidQt

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

/// Helper class containing pointer and some metadata for the loaded workspace
class WorkspaceRecord {

public:
  explicit WorkspaceRecord(const std::string &workspaceName);
  ~WorkspaceRecord();
  void updateMetadata(const size_t &newIndex);
  std::pair<double, double> getErange();

  boost::shared_ptr<Mantid::API::MatrixWorkspace> m_ws;
  const std::string m_name;
  double m_energy;
  std::string m_label;
};

class SliceSelector : public QMainWindow,
                      public MantidQt::API::WorkspaceObserver {
  Q_OBJECT

public:
  explicit SliceSelector(QWidget *parent = nullptr);
  ~SliceSelector();

protected:
  void preDeleteHandle(
      const std::string &workspaceName,
      const boost::shared_ptr<Mantid::API::Workspace> workspace) override;

signals:
  void signalSlicesLoaded(const QString &workspaceName);
  void signalSliceForFittingSelected(const size_t &workspaceIndex);

private slots:
  void showHelp();
  void loadSlices(const QString &workspaceName);
  void updateSelectedSlice(const int &newSelectedIndex);
  void newIndexFromPickedEnergy(const double &newEnergySelected);
  void updatePreviewPlotSelectedSlice();
  void updatePickerLine();
  void selectSliceForFitting();

private:
  void initLayout();
  void setupConnections();
  void tearConnections();
  void spawnPickerLine();
  void initPickerLine();
  bool isWorkspaceValid();
  void clearLoadedSlices();
  void handleRemoveEvent(Mantid::API::WorkspacePreDeleteNotification_ptr pNf);

  /// The form generated by Qt Designer
  Ui::SliceSelector m_uiForm;
  /// line that selects a slice in the 2D view
  MantidWidgets::RangeSelector *m_pickerLine;
  std::unique_ptr<WorkspaceRecord> m_loadedWorkspace;
  size_t m_selectedWorkspaceIndex;

}; // class SliceSelector
} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_SLICESELECTOR_H_
