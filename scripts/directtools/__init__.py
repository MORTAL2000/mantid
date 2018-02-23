from __future__ import (absolute_import, division, print_function)

import collections
from mantid import mtd
from mantid.simpleapi import DeleteWorkspace, LineProfile, OneMinusExponentialCor, Transpose
import matplotlib
from matplotlib import pyplot
import numpy
from scipy import constants
import time


def _clearlatex(s):
    """Return a string from which LaTeX formatting has been removed."""
    s = s.translate(None, '$\\^{}')
    return s


def _globalnanminmax(workspaces):
    """Return global minimum and maximum of the given workspaces."""
    workspaces = _normwslist(workspaces)
    globalMin = numpy.inf
    globalMax = -numpy.inf
    for ws in workspaces:
        cMin, cMax = nanminmax(ws)
        if cMin < globalMin:
            globalMin = cMin
        if cMax > globalMax:
            globalMax = cMax
    return globalMin, globalMax


def _label(ws, cut, width, singleWS, singleCut, singleWidth, quantity, units):
    """Return a line label for a line profile."""
    ws = _normws(ws)
    logs = SampleLogs(ws)
    wsLabel = ''
    if not singleWS:
        logs = SampleLogs(ws)
        T = numpy.mean(logs.sample.temperature)
        wsLabel = '\\#{:06d} $T$ = {:0.1f} K $E_i$ = {:0.2f} meV'.format(logs.run_number, T, logs.Ei)
    cutLabel = ''
    if not singleCut or not singleWidth:
        cutLabel = quantity + ' = {:0.2f} $\pm$ {:1.2f}'.format(cut, width) + ' ' + units
    return wsLabel + ' ' + cutLabel


def _normws(workspace):
    """Retrieve workspace from mtd if it is a string, otherwise return as-is."""
    name = str(workspace)
    if name:
        workspace = mtd[name]
    return workspace


def _normwslist(workspaces):
    """Retrieve workspaces from mtd if they are string, otherwise return as-is."""
    if not isinstance(workspaces, collections.Iterable) or isinstance(workspaces, str):
        workspaces = [workspaces]
    unnormWss = workspaces
    return [_normws(ws) for ws in unnormWss]


def _plottingtime():
    """Return a string presenting the plotting time."""
    return time.strftime('%d.%m.%Y %H:%M:%S')


def _points(edges):
    """Return bin centers."""
    return (edges[:-1] + edges[1:]) / 2


def _profiletitle(workspaces, scan, units, cuts, widths, figure):
    """Add title to line profile figure."""
    workspaces = _normwslist(workspaces)
    if not isinstance(cuts, collections.Iterable):
        cuts = [cuts]
    if not isinstance(widths, collections.Iterable):
        widths = [widths] * len(cuts)
    if len(workspaces) == 1:
        title = _singledatatitle(workspaces[0])
    else:
        logs = SampleLogs(workspaces[0])
        title = logs.instrument.name + ' ' + _plottingtime()
    if len(cuts) == 1 and len(widths) == 1:
        title = title + '\n' + scan + ' = {:0.2f} $\pm$ {:0.2f}'.format(cuts[0], widths[0]) + ' ' + units
    figure.suptitle(title)


def _profileytitle(workspace, axes):
    """Set the correct y label for profile axes."""
    if workspace.YUnit() == 'Dynamic susceptibility':
        axes.set_ylabel("$\\chi''(Q,E)$")
    else:
        axes.set_ylabel('$S(Q,E)$')


def _removesingularity(ws, epsilon):
    """Set bins at -epsilon <= x < epsilon  to zero."""
    Xs = ws.extractX()
    for i in range(ws.getNumberHistograms()):
        xs = Xs[i, :]
        ys = ws.dataY(i)
        es = ws.dataE(i)
        if len(xs) != len(ys):
            xs = _points(xs)
        xs = numpy.abs(xs)
        binIndex = numpy.argmin(xs)
        if xs[binIndex] >= -epsilon and xs[binIndex] < epsilon:
            ys[binIndex] = 0.
            es[binIndex] = 0.


def _sanitizeforlatex(s):
    """Return a string with LaTeX special characters escaped."""
    s = s.replace('_', '\\_')
    s = s.replace('#', '\\#')
    s = s.replace('@', '\\@')
    s = s.replace('&', '\\@')
    s = s.replace('$', '\\$')
    return s


def _singledatatitle(workspace):
    """Return title for a single data dataset."""
    workspace = _normws(workspace)
    logs = SampleLogs(workspace)
    T = numpy.mean(logs.sample.temperature)
    wsName = _sanitizeforlatex(str(workspace))
    title = (wsName + ' ' + logs.instrument.name + ' \\#{:06d}'.format(logs.run_number) + '\n'
             + _plottingtime() + '\n'
             + '$T$ = {:0.1f} K $E_i$ = {:0.2f} meV'.format(T, logs.Ei))
    return title


def _SofQWtitle(workspace, figure):
    """Add title to SofQW figure."""
    workspace = _normws(workspace)
    title = _singledatatitle(workspace)
    figure.suptitle(title)


def box2D(xs, vertAxis, horMin=-numpy.inf, horMax=numpy.inf, vertMin=-numpy.inf, vertMax=numpy.inf):
    """Return slicing for a 2D numpy array limited by given min and max values."""
    if len(vertAxis) > xs.shape[0]:
        vertAxis = _points(vertAxis)
    horBegin = numpy.argwhere(xs[0, :] >= horMin)[0][0]
    horEnd = numpy.argwhere(xs[0, :] < horMax)[-1][0] + 1
    vertBegin = numpy.argwhere(vertAxis >= vertMin)[0][0]
    vertEnd = numpy.argwhere(vertAxis < vertMax)[-1][0] + 1
    return slice(vertBegin, vertEnd), slice(horBegin, horEnd)


def configurematplotlib(params):
    """Set matplotlib rc parameters from the params dictionary."""
    matplotlib.rcParams.update(params)


def defaultrcParams():
    """Return a dictionary of directtools default matplotlib rc parameters."""
    params = {
        'legend.numpoints': 1,
        'text.usetex': True,
    }
    return params


def dynamicsusceptibility(workspace, temperature, outputName=None, zeroEnergyEpsilon=1e-6):
    """Convert :math:`S(Q,E)` to susceptibility :math:`\chi''(Q,E)`."""
    workspace = _normws(workspace)
    horAxis = workspace.getAxis(0)
    horUnit = horAxis.getUnit().unitID()
    doTranspose = False if horUnit == 'DeltaE' else True
    if outputName is None:
        outputName = 'CHIofQW_{}'.format(str(workspace))
    if doTranspose:
        workspace = Transpose(workspace, OutputWorkspace='__transposed_SofQW_', EnableLogging=False)
    c = 1e-3 * constants.e / constants.k / temperature
    outWS = OneMinusExponentialCor(workspace, OutputWorkspace=outputName, C=c, Operation='Multiply', EnableLogging=False)
    _removesingularity(outWS, zeroEnergyEpsilon)
    if doTranspose:
        outWS = Transpose(outWS, OutputWorkspace=outputName, EnableLogging=False)
        DeleteWorkspace('__transposed_SofQW_', EnableLogging=False)
    outWS.setYUnit("Dynamic susceptibility")
    return outWS


def mantidsubplotsetup():
    """Return a dict for the matplotlib.pyplot.subplots()."""
    return {'projection': 'mantid'}


def nanminmax(workspace, horMin=-numpy.inf, horMax=numpy.inf, vertMin=-numpy.inf, vertMax=numpy.inf):
    """Return min and max intensities of a workspace."""
    workspace = _normws(workspace)
    xs = workspace.extractX()
    ys = workspace.extractY()
    if xs.shape[1] > ys.shape[1]:
        xs = _points(xs)
    vertAxis = workspace.getAxis(1).extractValues()
    box = box2D(xs, vertAxis, horMin, horMax, vertMin, vertMax)
    ys = ys[box]
    cMin = numpy.nanmin(ys)
    cMax = numpy.nanmax(ys)
    return cMin, cMax


def plotprofile(workspace, axes=None, label='', style='l'):
    """Plot a single line profile."""
    workspace = _normws(workspace)
    lineStyle = 'solid' if 'l' in style else 'None'
    if axes is None:
        figure, axes = subplots()
    else:
        figure = axes.get_figure()
    if 'm' in style:
        markers = matplotlib.markers.MarkerStyle.filled_markers
        size = len(markers)
        markerIndex = len(axes.get_lines())
        markerIndex -= size * (markerIndex // size)
        markerStyle = matplotlib.markers.MarkerStyle.filled_markers[markerIndex]
    else:
        markerStyle = 'None'
    axes.errorbar(workspace, specNum=0, linestyle=lineStyle, marker=markerStyle, label=label, distribution=True)
    _profileytitle(workspace, axes)
    return figure, axes


def plotprofileE(workspace, axes=None, label='', style='l'):
    """Plot an existing constant E line profile."""
    workspace = _normws(workspace)
    figure, axes = plotprofile(workspace, axes, label, style)
    axes.set_xlim(xmin=0.)
    axes.set_xlabel('$Q$ (\\AA$^{-1}$)')
    axes.set_ylim(0.)
    xMin, xMax = axes.get_xlim()
    print('Auto Q-range: {}...{} \xc5-1'.format(xMin, xMax))
    return figure, axes


def plotprofileQ(workspace, axes=None, label='', style='l'):
    """Plot an existing constant Q line profile."""
    workspace = _normws(workspace)
    figure, axes = plotprofile(workspace, axes, label, style)
    axes.set_xlim(xmin=-10.)
    axes.set_xlabel('Energy (meV)')
    cMax = numpy.nanmax(workspace.readY(0))
    axes.set_ylim(ymin=0., ymax=cMax / 100.)
    xMin, xMax = axes.get_xlim()
    print('Auto E-range: {}...{} meV'.format(xMin, xMax))
    return figure, axes


def plotprofiles(direction, workspaces, cuts, widths, quantity, unit, style='l', keepCutWorkspaces=True):
    """Plot multiple line profiles from given workspaces and cuts."""
    workspaces = _normwslist(workspaces)
    if not isinstance(cuts, collections.Iterable):
        cuts = [cuts]
    if not isinstance(widths, collections.Iterable):
        widths = [widths]
    lineStyle = 'solid' if 'l' in style else 'None'
    figure, axes = subplots()
    markers = matplotlib.markers.MarkerStyle.filled_markers
    markerIndex = 0
    markerStyle = 'None'
    wsCount = 0
    cutWSList = list()
    for ws in workspaces:
        wsCount += 1
        for cut in cuts:
            for width in widths:
                # TODO: Use StoreInADS=False after issue #21731 has been fixed.
                wsStr = str(ws)
                if wsStr == '':
                    wsStr = str(wsCount)
                quantityStr = _clearlatex(quantity)
                unitStr = _clearlatex(unit)
                wsName = 'cut_{}_{}={}+-{}{}'.format(wsStr, quantityStr, cut, width, unitStr)
                if not keepCutWorkspaces:
                    wsName = '__' + wsName
                else:
                    cutWSList.append(wsName)
                line = LineProfile(ws, cut, width, Direction=direction,
                                   OutputWorkspace=wsName, EnableLogging=False)
                if 'm' in style:
                    markerStyle = markers[markerIndex]
                    markerIndex += 1
                    if markerIndex == len(markers):
                        markerIndex = 0
                label = _label(ws, cut, width, len(workspaces) == 1, len(cuts) == 1, len(widths) == 1, quantity, unit)
                axes.errorbar(line, specNum=0, linestyle=lineStyle, marker=markerStyle, label=label, distribution=True)
                if not keepCutWorkspaces:
                    DeleteWorkspace(line, EnableLogging=False)
    _profileytitle(workspaces[0], axes)
    return figure, axes, cutWSList


def plotconstE(workspaces, E, dE, style='l', keepCutWorkspaces=True):
    """Plot line profiles at constant energy."""
    figure, axes, cutWSList = plotprofiles('Horizontal', workspaces, E, dE, '$E$', 'meV', style, keepCutWorkspaces)
    _profiletitle(workspaces, '$E$', 'meV', E, dE, figure)
    axes.legend()
    axes.set_xlim(xmin=0.)
    axes.set_xlabel('$Q$ (\\AA$^{-1}$)')
    axes.set_ylim(0.)
    xMin, xMax = axes.get_xlim()
    print('Auto Q-range: {}...{} \xc5-1'.format(xMin, xMax))
    return figure, axes, cutWSList


def plotconstQ(workspaces, Q, dQ, style='l', keepCutWorkspaces=True):
    """Plot line profiles at constant momentum transfer."""
    figure, axes, cutWSList = plotprofiles('Vertical', workspaces, Q, dQ, '$Q$', '\\AA$^{-1}$', style, keepCutWorkspaces)
    _profiletitle(workspaces, '$Q$', '\\AA$^{-1}$', Q, dQ, figure)
    axes.legend()
    axes.set_xlim(xmin=-10.)
    axes.set_xlabel('Energy (meV)')
    cMin, cMax = _globalnanminmax(workspaces)
    axes.set_ylim(ymin=0., ymax=cMax / 100.)
    xMin, xMax = axes.get_xlim()
    print('Auto E-range: {}...{} meV'.format(xMin, xMax))
    return figure, axes, cutWSList


def plotSofQW(workspace, QMin=0., QMax=None, EMin=None, EMax=None, VMin=0., VMax=None, colormap='jet'):
    """Plot a 2D plot with given axis limits and return the plotting layer."""
    # Accept both workspace names and actual workspaces.
    workspace = _normws(workspace)
    isSusceptibility = workspace.YUnit() == 'Dynamic susceptibility'
    figure, axes = subplots()
    if QMin is None:
        QMin = 0.
    if QMax is None:
        dummy, QMax = validQ(workspace)
    if EMin is None:
        if isSusceptibility:
            EMin = 0.
        else:
            EMin = -10.
    if EMax is None:
        EAxis = workspace.getAxis(1).extractValues()
        EMax = numpy.amax(EAxis)
    if VMin is None:
        VMin = 0.
    if VMax is None:
        vertMax = EMax if EMax is not None else numpy.inf
        dummy, VMax = nanminmax(workspace, horMin=QMin, horMax=QMax, vertMin=EMin, vertMax=vertMax)
        VMax /= 100.
    print('Plotting intensity range: {}...{}'.format(VMin, VMax))
    contours = axes.pcolor(workspace, vmin=VMin, vmax=VMax, distribution=True, cmap=colormap)
    colorbar = figure.colorbar(contours)
    if isSusceptibility:
        colorbar.set_label("$\\chi''(Q,E)$ (arb. units)")
    else:
        colorbar.set_label('$S(Q,E)$ (arb. units)')
    axes.set_xlim(left=QMin)
    axes.set_xlim(right=QMax)
    axes.set_ylim(bottom=EMin)
    if EMax is not None:
        axes.set_ylim(top=EMax)
    axes.set_xlabel('$Q$ (\\AA$^{-1}$)')
    axes.set_ylabel('Energy (meV)')
    _SofQWtitle(workspace, figure)
    return figure, axes


def subplots(**kwargs):
    """Return matplotlib figure and axes."""
    return pyplot.subplots(subplot_kw=mantidsubplotsetup(), **kwargs)


def validQ(workspace, E=0.0):
    """Return a :math:`Q` range at given energy transfer where :math:`S(Q,E)` is defined."""
    workspace = _normws(workspace)
    vertBins = workspace.getAxis(1).extractValues()
    if len(vertBins) > workspace.getNumberHistograms:
        vertBins = _points(vertBins)
    elasticIndex = numpy.argmin(numpy.abs(vertBins - E))
    ys = workspace.readY(elasticIndex)
    validIndices = numpy.argwhere(numpy.logical_not(numpy.isnan(ys)))
    xs = workspace.readX(elasticIndex)
    lower = xs[numpy.amin(validIndices)]
    upperIndex = numpy.amax(validIndices)
    if len(xs) > len(ys):
        upperIndex = upperIndex + 1
    upper = xs[upperIndex]
    return lower, upper


def wsreport(workspace):
    """Print some useful information from sample logs."""
    workspace = _normws(workspace)
    print(str(workspace))
    logs = SampleLogs(workspace)
    print('Instrument: ' + logs.instrument.name)
    print('Run Number: {:06d}'.format(logs.run_number))
    print('Start Time: {}'.format(logs.start_time))
    print('Ei = {:0.2f} meV    lambda = {:0.2f} \xc5'.format(logs.Ei, logs.wavelength))
    meanT = numpy.mean(logs.sample.temperature)
    stdT = numpy.std(logs.sample.temperature)
    print('T = {:0.2f} +- {:4.2f} K'.format(meanT, stdT))
    minT = numpy.amin(logs.sample.temperature)
    maxT = numpy.amax(logs.sample.temperature)
    print('T in [{:0.2f},{:0.2f}]'.format(minT, maxT))
    fermiHertz = logs.FC.rotation_speed / 60.
    print('Fermi = {:0.0f} rpm = {:0.1f} Hz'.format(logs.FC.rotation_speed, fermiHertz))


class SampleLogs:
    def __init__(self, workspace):
        """Transform sample log entries from workspace into attributes of this object."""
        class Log:
            pass
        workspace = _normws(workspace)
        properties = workspace.run().getProperties()
        for p in properties:
            name = p.name
            components = name.split('.')
            obj = self
            while len(components) > 1:
                c = components.pop(0)
                if not hasattr(obj, c):
                    subdir = Log()
                    setattr(obj, c, subdir)
                    obj = subdir
                else:
                    obj = getattr(obj, c)
            setattr(obj, components[0], p.value)


# Set default matplotlib rc parameters.
configurematplotlib(defaultrcParams())
