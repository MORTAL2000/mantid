// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include <cxxtest/TestSuite.h>

class FilteringUpdateProgressActionTest : public CxxTest::TestSuite {

private:
  // Actually a fake, wouldn't make sense to mock a concrete type. Effectively
  // the view.
  struct MockFilter {
    double Progress;
    std::string Message;
    void updateAlgorithmProgress(double progress, const std::string &message) {
      this->Progress = progress;
      this->Message = message;
    }
  };

  using ProgressActionType =
      Mantid::VATES::FilterUpdateProgressAction<MockFilter>;

public:
  void testCallsView() {
    // Test that is wired up correctly.
    MockFilter view;
    ProgressActionType model(&view, "message");
    model.eventRaised(10);
    TSM_ASSERT_EQUALS(
        "View and model are not wired up correctly for progress updating.", 10,
        view.Progress);
    TSM_ASSERT_EQUALS(
        "View and model are not wired up correctly for progress updating.",
        "message", view.Message);
  }

  void testIsProgressAction() {
    // Test that template works through abstract data type.
    MockFilter view;
    ProgressActionType model(&view, "message");
    Mantid::VATES::ProgressAction &imodel = model;
    imodel.eventRaised(10);
    TSM_ASSERT_EQUALS(
        "View and model are not wired up correctly for progress updating.", 10,
        view.Progress);
    TSM_ASSERT_EQUALS(
        "View and model are not wired up correctly for progress updating.",
        "message", view.Message);
  }
};