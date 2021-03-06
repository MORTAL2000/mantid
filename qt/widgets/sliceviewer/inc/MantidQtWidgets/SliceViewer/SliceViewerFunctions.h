// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/VMD.h"

namespace MantidQt {
namespace SliceViewer {

/// Checks if a slice lies within a workspace or not
bool EXPORT_OPT_MANTIDQT_SLICEVIEWER doesSliceCutThroughWorkspace(
    const Mantid::Kernel::VMD &min, const Mantid::Kernel::VMD &max,
    const std::vector<Mantid::Geometry::MDHistoDimension_sptr> &dimensions);

/// Checks if rebin mode is in consistent state
bool EXPORT_OPT_MANTIDQT_SLICEVIEWER isRebinInConsistentState(
    Mantid::API::IMDWorkspace *rebinnedWS, bool useRebinMode);

/// Should perform auto color scaling on load
bool EXPORT_OPT_MANTIDQT_SLICEVIEWER shouldAutoScaleForNewlySetWorkspace(
    bool isFirstWorkspaceOpen, bool isAutoScalingOnLoad);
} // namespace SliceViewer
} // namespace MantidQt