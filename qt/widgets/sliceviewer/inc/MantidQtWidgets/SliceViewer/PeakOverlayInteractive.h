// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/SliceViewer/PeakOverlayView.h"
#include <QCursor>
#include <QWidget>

// Forward dec
class QwtPlot;
class QRect;

namespace MantidQt {

namespace MantidWidgets {
// Forward dec
class InputController;
} // namespace MantidWidgets

namespace SliceViewer {
// Forward dec
class PeaksPresenter;

/** Widget base class for representing peaks. Contains common code used by
 Interactive/Editable peak overlay widgets.
*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakOverlayInteractive
    : public QWidget,
      public PeakOverlayView {
  Q_OBJECT

public:
  /// Constructor
  PeakOverlayInteractive(PeaksPresenter *const peaksPresenter, QwtPlot *plot,
                         const int plotXIndex, const int plotYIndex,
                         QWidget *parent);
  /// Destructor
  ~PeakOverlayInteractive() override;

  /// Enter peak deletion mode.
  void peakDeletionMode() override;
  /// Enter peak addition mode
  void peakAdditionMode() override;
  /// Enter display mode
  void peakDisplayMode() override;

  QSize sizeHint() const override;
  QSize size() const;
  int height() const;
  int width() const;

protected:
  /// Owning presenter
  PeaksPresenter *m_presenter;
  /// QwtPlot containing this
  QwtPlot *m_plot;
  /// Plot x index
  const int m_plotXIndex;
  /// Plot y index
  const int m_plotYIndex;

private:
  /// Input controller.
  MantidQt::MantidWidgets::InputController *m_tool;

  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void wheelEvent(QWheelEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;
  void enterEvent(QEvent *e) override;
  void leaveEvent(QEvent *e) override;

  void paintEvent(QPaintEvent *event) override;

  // Call do paint on sub-classes
  virtual void doPaintPeaks(QPaintEvent *event) = 0;

  void captureMouseEvents(bool capture);

private slots:

  void erasePeaks(const QRect &rect);
  void addPeakAt(int coordX, int coordY);
};
} // namespace SliceViewer
} // namespace MantidQt
