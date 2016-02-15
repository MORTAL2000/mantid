#pylint: disable=no-init
from mantid import config, logger, AlgorithmFactory
from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
import os.path

class FuryFitMultiple(PythonAlgorithm):

    _input_ws = None
    _function = None
    _fit_type = None
    _start_x = None
    _end_x = None
    _spec_min = None
    _spec_max = None
    _intensities_constrained = None
    _minimizer = None
    _max_iterations = None
    _save = None
    _plot = None

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        #pylint: disable=anomalous-backslash-in-string
        return "Fits an \*\_iqt file generated by I(Q,t)."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '', direction=Direction.Input),
                             doc='The _iqt.nxs InputWorkspace used by the algorithm')
        self.declareProperty(name='Function', defaultValue='',
                             doc='The function to use in fitting')
        self.declareProperty(name='FitType', defaultValue='',
                             doc='The type of fit being carried out')
        self.declareProperty(name='StartX', defaultValue=0.0,
                             doc="The first value for X")
        self.declareProperty(name='EndX', defaultValue=0.2,
                             doc="The last value for X")
        self.declareProperty(name='SpecMin', defaultValue=0,
                             doc='Minimum spectra in the worksapce to fit')
        self.declareProperty(name='SpecMax', defaultValue=0,
                             doc='Maximum spectra in the worksapce to fit')
        self.declareProperty(name='Minimizer', defaultValue='Levenberg-Marquardt',
                             doc='The minimizer to use in fitting')
        self.declareProperty(name="MaxIterations", defaultValue=500,
                             doc="The Maximum number of iterations for the fit")
        self.declareProperty(name='ConstrainIntensities', defaultValue=False,
                             doc="If the Intensities should be constrained during the fit")
        self.declareProperty(name='Save', defaultValue=False,
                             doc="Should the Output of the algorithm be saved to working directory")
        self.declareProperty(name='Plot', defaultValue='None', validator=StringListValidator(['None', 'Intensity', 'Tau', 'Beta', 'All']),
                             doc='Switch Plot Off/On')
        self.declareProperty(MatrixWorkspaceProperty('OutputResultWorkspace', '', direction=Direction.Output),
                             doc='The outputworkspace containing the results of the fit data')
        self.declareProperty(ITableWorkspaceProperty('OutputParameterWorkspace', '', direction=Direction.Output),
                             doc='The outputworkspace containing the parameters for each fit')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceGroup', '', direction=Direction.Output),
                             doc='The OutputWorkspace group Data, Calc and Diff, values for the fit of each spectra')


    def validateInputs(self):
        self._get_properties()
        issues = dict()

        maximum_possible_spectra = self._input_ws.getNumberHistograms()
        maximum_possible_x = self._input_ws.readX(0)[self._input_ws.blocksize() - 1]
        # Validate SpecMin/Max

        if self._spec_max > maximum_possible_spectra:
            issues['SpecMax'] = ('SpecMax must be smaller or equal to the number of spectra in the input workspace, %d' % maximum_possible_spectra)
        if self._spec_min < 0:
            issues['SpecMin'] = 'SpecMin can not be less than 0'
        if self._spec_max < self._spec_min:
            issues['SpecMax'] = 'SpecMax must be more than or equal to SpecMin'

        # Validate Start/EndX
        if self._end_x > maximum_possible_x:
            issues['EndX'] = ('EndX must be less than the highest x value in the workspace, %d' % maximum_possible_x)
        if self._start_x < 0:
            issues['StartX'] = 'StartX can not be less than 0'
        if self._start_x > self._end_x:
            issues['EndX'] = 'EndX must be more than StartX'

        return issues

    def _get_properties(self):
        self._input_ws = self.getProperty('InputWorkspace').value
        self._function = self.getProperty('Function').value
        self._fit_type = self.getProperty('FitType').value
        self._start_x = self.getProperty('StartX').value
        self._end_x = self.getProperty('EndX').value
        self._spec_min = self.getProperty('SpecMin').value
        self._spec_max = self.getProperty('SpecMax').value
        self._intensities_constrained = self.getProperty('ConstrainIntensities').value
        self._minimizer = self.getProperty('Minimizer').value
        self._max_iterations = self.getProperty('MaxIterations').value
        self._save = self.getProperty('Save').value
        self._plot = self.getProperty('Plot').value

    def PyExec(self):
        from IndirectDataAnalysis import (getWSprefix, convertToElasticQ,
                                          createFuryMultiDomainFunction,
                                          transposeFitParametersTable,
                                          convertParametersToWorkspace,
                                          furyFitSaveWorkspaces,
                                          furyfitPlotSeq)

        # Run FuryFitMultiple algorithm from indirectDataAnalysis
        nHist = self._input_ws.getNumberHistograms()
        output_workspace = getWSprefix(self._input_ws.getName()) + 'fury_1Smult_s0_to_' + str(nHist-1)

        option = self._fit_type[:-2]
        logger.information('Option: '+ option)
        logger.information('Function: '+ self._function)

        #prepare input workspace for fitting
        tmp_fit_workspace = "__furyfit_fit_ws"
        if self._spec_max is None:
            CropWorkspace(InputWorkspace=self._input_ws, OutputWorkspace=tmp_fit_workspace,
                          XMin=self._start_x, XMax=self._end_x,
                          StartWorkspaceIndex=self._spec_min)
        else:
            CropWorkspace(InputWorkspace=self._input_ws, OutputWorkspace=tmp_fit_workspace,
                          XMin=self._start_x, XMax=self._end_x,
                          StartWorkspaceIndex=self._spec_min, EndWorkspaceIndex=self._spec_max)

        ConvertToHistogram(tmp_fit_workspace, OutputWorkspace=tmp_fit_workspace)
        convertToElasticQ(tmp_fit_workspace)

        #fit multi-domian functino to workspace
        multi_domain_func, kwargs = createFuryMultiDomainFunction(self._function, tmp_fit_workspace)
        Fit(Function=multi_domain_func,
            InputWorkspace=tmp_fit_workspace,
            WorkspaceIndex=0,
            Output=output_workspace,
            CreateOutput=True,
            Minimizer=self._minimizer,
            MaxIterations=self._max_iterations,
            **kwargs)

        params_table = output_workspace + '_Parameters'
        transposeFitParametersTable(params_table)

        #set first column of parameter table to be axis values
        x_axis = mtd[tmp_fit_workspace].getAxis(1)
        axis_values = x_axis.extractValues()
        for i, value in enumerate(axis_values):
            mtd[params_table].setCell('axis-1', i, value)

        #convert parameters to matrix workspace
        result_workspace = output_workspace + "_Result"
        parameter_names = ['A0', 'Intensity', 'Tau', 'Beta']
        convertParametersToWorkspace(params_table, "axis-1", parameter_names, result_workspace)

        #set x units to be momentum transfer
        axis = mtd[result_workspace].getAxis(0)
        axis.setUnit("MomentumTransfer")

        result_workspace = output_workspace + '_Result'
        fit_group = output_workspace + '_Workspaces'

        sample_logs  = {'start_x': self._start_x, 'end_x': self._end_x, 'fit_type': self._fit_type,
                        'intensities_constrained': self._intensities_constrained, 'beta_constrained': True}

        CopyLogs(InputWorkspace=self._input_ws, OutputWorkspace=result_workspace)
        CopyLogs(InputWorkspace=self._input_ws, OutputWorkspace=fit_group)

        log_names = [item[0] for item in sample_logs]
        log_values = [item[1] for item in sample_logs]
        AddSampleLogMultiple(Workspace=result_workspace, LogNames=log_names, LogValues=log_values)
        AddSampleLogMultiple(Workspace=fit_group, LogNames=log_names, LogValues=log_values)

        DeleteWorkspace(tmp_fit_workspace)

        if self._save:
            save_workspaces = [result_workspace]
            furyFitSaveWorkspaces(save_workspaces)

        if self._plot != 'None':
            furyfitPlotSeq(result_workspace, Plot)

        self.setProperty('OutputResultWorkspace', result_workspace)
        self.setProperty('OutputParameterWorkspace', params_table)
        self.setProperty('OutputWorkspaceGroup', fit_group)


AlgorithmFactory.subscribe(FuryFitMultiple)
