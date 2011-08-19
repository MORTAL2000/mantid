#ifndef MDVIEWERWIDGET_H_
#define MDVIEWERWIDGET_H_

#include <QtGui/QWidget>
#include <QPointer>
#include "ui_MdViewerWidget.h"

class ViewBase;

class pqLoadDataReaction;
class pqPipelineSource;

class QAction;
class QHBoxLayout;
/**
 *
  This class represents the central widget for handling VATES visualization
  operations.

  @author Michael Reuter
  @date 11/08/2011

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MdViewerWidget : public QWidget
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget for the main window
   */
  MdViewerWidget(QWidget *parent = 0);
  /// Default destructor.
  virtual ~MdViewerWidget();

  /**
   * Connect ParaView's data loader the given action.
   * @param action the action to connect data loading to
   */
  void connectLoadDataReaction(QAction *action);

protected slots:
  /**
   * Load and render data from the given source.
   * @param source a ParaView compatible source
   */
  void onDataLoaded(pqPipelineSource *source);
  /**
   * Execute the logic for switching views on the main level window.
   * @param v the view mode to switch to
   */
  void switchViews(ModeControlWidget::Views v);

signals:
  /// Signal to disable all view modes but standard.
  void disableViewModes();
  /// Signal to enable the threeslice view mode button.
  void enableThreeSliceViewButton();
  /// Signal to enable the multislice view mode button.
  void enableMultiSliceViewButton();

private:
  Q_DISABLE_COPY(MdViewerWidget);
  ViewBase *currentView; ///< Holder for the current view
  pqLoadDataReaction *dataLoader; ///< Holder for the load data reaction
  ViewBase *hiddenView; ///< Holder for the view that is being switched from
  QPointer<pqPipelineSource> originSource; ///< Holder for the current source
  Ui::MdViewerWidget ui; ///< The MD viewer's UI form
  QHBoxLayout *viewLayout; ///< Layout manager for the view widget

  /// Disable communication with the proxy tab widget.
  void removeProxyTabWidgetConnections();
  /// Set the signals/slots for the ParaView components based on the view.
  void setParaViewComponentsForView();
  /**
   * Create the requested view on the main window.
   * @param container the UI widget to associate the view mode with
   * @param v the view mode to set on the main window
   * @return the requested view
   */
  ViewBase *setMainViewWidget(QWidget *container, ModeControlWidget::Views v);
  /// Helper function to swap current and hidden view pointers.
  void swapViews();
};

#endif // MDVIEWERWIDGET_H_
