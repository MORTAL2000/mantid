// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/FunctionBrowser.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"

#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QWidget>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW PlotFitAnalysisPaneView
    : public QWidget {
  Q_OBJECT

public:
  explicit PlotFitAnalysisPaneView(const double &start, const double &end,
                                   QWidget *parent = nullptr);

  void observeFitButton(Observer *listener) {
    m_fitObservable->attach(listener);
  };
  std::pair<double, double> getRange();
  Mantid::API::IFunction_sptr getFunction();
  void addSpectrum(const std::string& wsName);
  void addFitSpectrum(const std::string& wsName);
  void addFunction(Mantid::API::IFunction_sptr func);
  void updateFunction(const Mantid::API::IFunction_sptr& func);
  void fitWarning(const std::string &message);

public slots:
  void doFit();

private:
  void setupPlotFitSplitter(const double &start, const double &end);

  MantidWidgets::PreviewPlot *m_plot;
  MantidWidgets::FunctionBrowser *m_fitBrowser;
  QLineEdit *m_start, *m_end;
  QWidget *createFitPane(const double &start, const double &end);
  QSplitter *m_fitPlotLayout;
  QPushButton *m_fitButton;
  Observable *m_fitObservable;
};
} // namespace MantidWidgets
} // namespace MantidQt
