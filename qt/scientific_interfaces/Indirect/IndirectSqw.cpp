// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSqw.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Plotting/AxisID.h"

#include <QFileInfo>

using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace {
Mantid::Kernel::Logger g_log("S(Q,w)");

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
}

double roundToPrecision(double value, double precision) {
  return value - std::remainder(value, precision);
}

std::pair<double, double>
roundToWidth(std::tuple<double, double> const &axisRange, double width) {
  return std::make_pair(roundToPrecision(std::get<0>(axisRange), width) + width,
                        roundToPrecision(std::get<1>(axisRange), width) -
                            width);
}

void convertToSpectrumAxis(std::string const &inputName,
                           std::string const &outputName) {
  auto converter = AlgorithmManager::Instance().create("ConvertSpectrumAxis");
  converter->initialize();
  converter->setProperty("InputWorkspace", inputName);
  converter->setProperty("OutputWorkspace", outputName);
  converter->setProperty("Target", "ElasticQ");
  converter->setProperty("EMode", "Indirect");
  converter->execute();
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectSqw::IndirectSqw(IndirectDataReduction *idrUI, QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(plotRqwContour()));
  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(setDefaultQAndEnergy()));
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(sqwAlgDone(bool)));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbPlotSpectrum, SIGNAL(clicked()), this,
          SLOT(plotSpectrumClicked()));
  connect(m_uiForm.pbPlotContour, SIGNAL(clicked()), this,
          SLOT(plotContourClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  connect(this,
          SIGNAL(updateRunButton(bool, std::string const &, QString const &,
                                 QString const &)),
          this,
          SLOT(updateRunButton(bool, std::string const &, QString const &,
                               QString const &)));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  m_uiForm.rqwPlot2D->setXAxisLabel("Energy (meV)");
  m_uiForm.rqwPlot2D->setYAxisLabel("Q (A-1)");
#else
  m_uiForm.rqwPlot2D->setCanvasColour(QColor(240, 240, 240));
#endif
}

IndirectSqw::~IndirectSqw() {}

void IndirectSqw::setup() {}

bool IndirectSqw::validate() {
  double const tolerance = 1e-10;
  UserInputValidator uiv;

  // Validate the data selector
  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);

  // Validate Q binning
  uiv.checkBins(m_uiForm.spQLow->value(), m_uiForm.spQWidth->value(),
                m_uiForm.spQHigh->value(), tolerance);

  // If selected, validate energy binning
  if (m_uiForm.ckRebinInEnergy->isChecked())
    uiv.checkBins(m_uiForm.spELow->value(), m_uiForm.spEWidth->value(),
                  m_uiForm.spEHigh->value(), tolerance);

  auto const errorMessage = uiv.generateErrorMessage();

  // Show an error message if needed
  if (!errorMessage.isEmpty())
    emit showMessageBox(errorMessage);

  return errorMessage.isEmpty();
}

void IndirectSqw::run() {
  auto const sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  auto const sqwWsName = sampleWsName.left(sampleWsName.length() - 4) + "_sqw";
  auto const eRebinWsName = sampleWsName.left(sampleWsName.length() - 4) + "_r";

  auto const rebinString = m_uiForm.spQLow->text() + "," +
                           m_uiForm.spQWidth->text() + "," +
                           m_uiForm.spQHigh->text();

  // Rebin in energy
  bool const rebinInEnergy = m_uiForm.ckRebinInEnergy->isChecked();
  if (rebinInEnergy) {
    auto const eRebinString = m_uiForm.spELow->text() + "," +
                              m_uiForm.spEWidth->text() + "," +
                              m_uiForm.spEHigh->text();

    auto energyRebinAlg = AlgorithmManager::Instance().create("Rebin");
    energyRebinAlg->initialize();
    energyRebinAlg->setProperty("InputWorkspace", sampleWsName.toStdString());
    energyRebinAlg->setProperty("OutputWorkspace", eRebinWsName.toStdString());
    energyRebinAlg->setProperty("Params", eRebinString.toStdString());

    m_batchAlgoRunner->addAlgorithm(energyRebinAlg);
  }

  auto const eFixed = getInstrumentDetail("Efixed").toStdString();

  auto sqwAlg = AlgorithmManager::Instance().create("SofQW");
  sqwAlg->initialize();
  sqwAlg->setProperty("OutputWorkspace", sqwWsName.toStdString());
  sqwAlg->setProperty("QAxisBinning", rebinString.toStdString());
  sqwAlg->setProperty("EMode", "Indirect");
  sqwAlg->setProperty("EFixed", eFixed);
  sqwAlg->setProperty("Method", "NormalisedPolygon");
  sqwAlg->setProperty("ReplaceNaNs", true);

  BatchAlgorithmRunner::AlgorithmRuntimeProps sqwInputProps;
  sqwInputProps["InputWorkspace"] =
      rebinInEnergy ? eRebinWsName.toStdString() : sampleWsName.toStdString();

  m_batchAlgoRunner->addAlgorithm(sqwAlg, sqwInputProps);

  // Add sample log for S(Q, w) algorithm used
  auto sampleLogAlg = AlgorithmManager::Instance().create("AddSampleLog");
  sampleLogAlg->initialize();
  sampleLogAlg->setProperty("LogName", "rebin_type");
  sampleLogAlg->setProperty("LogType", "String");
  sampleLogAlg->setProperty("LogText", "NormalisedPolygon");

  BatchAlgorithmRunner::AlgorithmRuntimeProps inputToAddSampleLogProps;
  inputToAddSampleLogProps["Workspace"] = sqwWsName.toStdString();

  m_batchAlgoRunner->addAlgorithm(sampleLogAlg, inputToAddSampleLogProps);

  // Set the name of the result workspace for Python export
  m_pythonExportWsName = sqwWsName.toStdString();

  m_batchAlgoRunner->executeBatch();
}

std::size_t IndirectSqw::getOutWsNumberOfSpectra() const {
  return getADSMatrixWorkspace(m_pythonExportWsName)->getNumberHistograms();
}

/**
 * Handles plotting the S(Q, w) workspace when the algorithm chain is finished.
 *
 * @param error If the algorithm chain failed
 */
void IndirectSqw::sqwAlgDone(bool error) {
  if (!error) {
    setPlotSpectrumEnabled(true);
    setPlotContourEnabled(true);
    setSaveEnabled(true);

    setPlotSpectrumIndexMax(static_cast<int>(getOutWsNumberOfSpectra()) - 1);
  }
}

void IndirectSqw::setPlotSpectrumIndexMax(int maximum) {
  MantidQt::API::SignalBlocker blocker(m_uiForm.spWorkspaceIndex);
  m_uiForm.spWorkspaceIndex->setMaximum(maximum);
}

/**
 * Handles the Plot Input button
 *
 * Creates a colour 2D plot of the data
 */
void IndirectSqw::plotRqwContour() {
  auto &ads = AnalysisDataService::Instance();
  if (m_uiForm.dsSampleInput->isValid()) {
    auto const sampleName =
        m_uiForm.dsSampleInput->getCurrentDataName().toStdString();

    if (ads.doesExist(sampleName)) {
      auto const outputName =
          sampleName.substr(0, sampleName.size() - 4) + "_rqw";

      try {
        convertToSpectrumAxis(sampleName, outputName);
        if (ads.doesExist(outputName)) {
          auto const rqwWorkspace = getADSMatrixWorkspace(outputName);
          if (rqwWorkspace)
            m_uiForm.rqwPlot2D->setWorkspace(rqwWorkspace);
        }
      } catch (std::exception const &ex) {
        g_log.warning(ex.what());
        showMessageBox("Invalid file. Please load a valid reduced workspace.");
      }
    }

  } else {
    emit showMessageBox("Invalid file. Please load a valid reduced workspace.");
  }
}

void IndirectSqw::setDefaultQAndEnergy() {
  if (m_uiForm.dsSampleInput->isValid()) {
    setQRange(m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::YLeft));
    setEnergyRange(
        m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::XBottom));
  }
}

void IndirectSqw::setQRange(std::tuple<double, double> const &axisRange) {
  auto const qRange = roundToWidth(axisRange, m_uiForm.spQWidth->value());
  m_uiForm.spQLow->setValue(qRange.first);
  m_uiForm.spQHigh->setValue(qRange.second);
}

void IndirectSqw::setEnergyRange(std::tuple<double, double> const &axisRange) {
  auto const energyRange = roundToWidth(axisRange, m_uiForm.spEWidth->value());
  m_uiForm.spELow->setValue(energyRange.first);
  m_uiForm.spEHigh->setValue(energyRange.second);
}

void IndirectSqw::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Sqw");
  m_uiForm.dsSampleInput->setFBSuffixes(filter ? getSampleFBSuffixes(tabName)
                                               : getExtensions(tabName));
  m_uiForm.dsSampleInput->setWSSuffixes(filter ? getSampleWSSuffixes(tabName)
                                               : noSuffixes);
}

void IndirectSqw::runClicked() { runTab(); }

void IndirectSqw::plotSpectrumClicked() {
  setPlotSpectrumIsPlotting(true);

  auto const spectrumNumber = m_uiForm.spWorkspaceIndex->text().toInt();
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, true))
    plotSpectrum(QString::fromStdString(m_pythonExportWsName), spectrumNumber);

  setPlotSpectrumIsPlotting(false);
}

void IndirectSqw::plotContourClicked() {
  setPlotContourIsPlotting(true);

  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, true))
    IndirectTab::plot2D(QString::fromStdString(m_pythonExportWsName));

  setPlotContourIsPlotting(false);
}

void IndirectSqw::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatch();
}

void IndirectSqw::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void IndirectSqw::setPlotSpectrumEnabled(bool enabled) {
  m_uiForm.pbPlotSpectrum->setEnabled(enabled);
  m_uiForm.spWorkspaceIndex->setEnabled(enabled);
}

void IndirectSqw::setPlotContourEnabled(bool enabled) {
  m_uiForm.pbPlotContour->setEnabled(enabled);
}

void IndirectSqw::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void IndirectSqw::setOutputButtonsEnabled(
    std::string const &enableOutputButtons) {
  bool enable = enableOutputButtons == "enable" ? true : false;
  setPlotSpectrumEnabled(enable);
  setPlotContourEnabled(enable);
  setSaveEnabled(enable);
}

void IndirectSqw::updateRunButton(bool enabled,
                                  std::string const &enableOutputButtons,
                                  QString const message,
                                  QString const tooltip) {
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged")
    setOutputButtonsEnabled(enableOutputButtons);
}

void IndirectSqw::setPlotSpectrumIsPlotting(bool plotting) {
  m_uiForm.pbPlotSpectrum->setText(plotting ? "Plotting..." : "Plot Spectrum");
  setPlotSpectrumEnabled(!plotting);
  setPlotContourEnabled(!plotting);
  setRunEnabled(!plotting);
  setSaveEnabled(!plotting);
}

void IndirectSqw::setPlotContourIsPlotting(bool plotting) {
  m_uiForm.pbPlotContour->setText(plotting ? "Plotting..." : "Plot Contour");
  setPlotSpectrumEnabled(!plotting);
  setPlotContourEnabled(!plotting);
  setRunEnabled(!plotting);
  setSaveEnabled(!plotting);
}

} // namespace CustomInterfaces
} // namespace MantidQt
