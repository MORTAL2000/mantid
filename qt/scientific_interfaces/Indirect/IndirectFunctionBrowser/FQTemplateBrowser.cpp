// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "FQTemplateBrowser.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/ButtonEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/CompositeEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

#include <limits>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * Constructor
 * @param parent :: The parent widget.
 */
FQTemplateBrowser::FQTemplateBrowser(QWidget *parent)
    : FunctionTemplateBrowser(parent), m_presenter(this) {
  connect(&m_presenter, SIGNAL(functionStructureChanged()), this,
          SIGNAL(functionStructureChanged()));
}

void FQTemplateBrowser::createProperties() {
  m_parameterManager->blockSignals(true);
  m_boolManager->blockSignals(true);
  m_enumManager->blockSignals(true);

  m_fitType = m_enumManager->addProperty("Fit Type");
  m_browser->addProperty(m_fitType);
  updateDataType(DataType::WIDTH);

  m_parameterManager->blockSignals(false);
  m_enumManager->blockSignals(false);
  m_boolManager->blockSignals(false);
}

void FQTemplateBrowser::setDataType(const QStringList& allowedFunctionsList) {
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setEnumNames(m_fitType, allowedFunctionsList);
  m_enumManager->setValue(m_fitType, 0);
}

void FQTemplateBrowser::setEnumValue(int enumIndex) {
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, enumIndex);
}

void FQTemplateBrowser::addParameter(const QString& parameterName,
                                     const QString& parameterDescription) {
  auto newParameter = m_parameterManager->addProperty(parameterName);
  m_parameterManager->setDescription(newParameter,
                                     parameterDescription.toStdString());
  m_parameterManager->setDecimals(newParameter, 6);
  m_fitType->addSubProperty(newParameter);
  m_parameterMap.insert(parameterName, newParameter);
  m_parameterNames.insert(newParameter, parameterName);
}

int FQTemplateBrowser::getCurrentDataset() {
  return m_presenter.getCurrentDataset();
}

void FQTemplateBrowser::setFunction(const QString &funStr) {
  m_presenter.setFunction(funStr);
}

IFunction_sptr FQTemplateBrowser::getGlobalFunction() const {
  return m_presenter.getGlobalFunction();
}

IFunction_sptr FQTemplateBrowser::getFunction() const {
  return m_presenter.getFunction();
}

void FQTemplateBrowser::setNumberOfDatasets(int n) {
  m_presenter.setNumberOfDatasets(n);
}

int FQTemplateBrowser::getNumberOfDatasets() const {
  return m_presenter.getNumberOfDatasets();
}

void FQTemplateBrowser::setDatasetNames(const QStringList &names) {
  m_presenter.setDatasetNames(names);
}

QStringList FQTemplateBrowser::getGlobalParameters() const {
  return m_presenter.getGlobalParameters();
}

QStringList FQTemplateBrowser::getLocalParameters() const {
  return m_presenter.getLocalParameters();
}

void FQTemplateBrowser::setGlobalParameters(const QStringList &globals) {
  m_presenter.setGlobalParameters(globals);
}

void FQTemplateBrowser::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;
  if (prop == m_fitType) {
    auto fitType = m_enumManager->enumNames(prop)[m_enumManager->value(prop)];
    m_presenter.setFitType(fitType);
  }
}

void FQTemplateBrowser::globalChanged(QtProperty *, const QString &, bool) {}

void FQTemplateBrowser::parameterChanged(QtProperty *prop) {
  auto isGlobal = m_parameterManager->isGlobal(prop);
  m_presenter.setGlobal(m_parameterNames[prop], isGlobal);
  if (m_emitParameterValueChange) {
    emit parameterValueChanged(m_parameterNames[prop],
                               m_parameterManager->value(prop));
  }
}

void FQTemplateBrowser::parameterButtonClicked(QtProperty *prop) {
  emit localParameterButtonClicked(m_parameterNames[prop]);
}

void FQTemplateBrowser::updateMultiDatasetParameters(const IFunction &fun) {
  m_presenter.updateMultiDatasetParameters(fun);
}

void FQTemplateBrowser::updateMultiDatasetParameters(const ITableWorkspace &) {}

void FQTemplateBrowser::updateParameters(const IFunction &fun) {
  m_presenter.updateParameters(fun);
}

void FQTemplateBrowser::setParameterValue(const QString& parameterName,
                                          double parameterValue,
                                          double parameterError) {
  m_parameterManager->setValue(m_parameterMap[parameterName], parameterValue);
  m_parameterManager->setError(m_parameterMap[parameterName], parameterError);
}

void FQTemplateBrowser::setCurrentDataset(int i) {
  m_presenter.setCurrentDataset(i);
}

void FQTemplateBrowser::updateParameterNames(const QMap<int, QString> &) {}

void FQTemplateBrowser::updateParameterDescriptions(
    const QMap<int, std::string> &) {}

void FQTemplateBrowser::setErrorsEnabled(bool enabled) {
  ScopedFalse _false(m_emitParameterValueChange);
  m_parameterManager->setErrorsEnabled(enabled);
}

void FQTemplateBrowser::clear() {
  m_parameterManager->clear();
  m_parameterMap.clear();
  m_parameterNames.clear();
}

void FQTemplateBrowser::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_presenter.updateParameterEstimationData(std::move(data));
}

void FQTemplateBrowser::popupMenu(const QPoint &) {}

void FQTemplateBrowser::setParameterPropertyValue(QtProperty *prop,
                                                  double value, double error) {
  if (prop) {
    ScopedFalse _false(m_emitParameterValueChange);
    m_parameterManager->setValue(prop, value);
    m_parameterManager->setError(prop, error);
  }
}

void FQTemplateBrowser::setGlobalParametersQuiet(const QStringList &globals) {
  ScopedFalse _false(m_emitParameterValueChange);
  for (auto &parameterName : m_parameterMap.keys()) {
    if (globals.contains(parameterName)) {
      m_parameterManager->setGlobal(m_parameterMap[parameterName], true);
    } else {
      m_parameterManager->setGlobal(m_parameterMap[parameterName], false);
    }
  }
}

void FQTemplateBrowser::setBackgroundA0(double) {}
void FQTemplateBrowser::setResolution(std::string const &,
                                      TableDatasetIndex const &) {}
void FQTemplateBrowser::setResolution(
    const std::vector<std::pair<std::string, int>> &) {}
void FQTemplateBrowser::setQValues(const std::vector<double> &) {}

void FQTemplateBrowser::updateDataType(DataType dataType) {
  emit dataTypeChanged(dataType);
}

void FQTemplateBrowser::spectrumChanged(int) {}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt