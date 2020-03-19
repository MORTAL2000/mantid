// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IFunctionWrapper.h"


#include <utility>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"

void IFunctionWrapper::setFunction(const QString &name) {
  try {
    m_function = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(
        Mantid::API::FunctionFactory::Instance().createFunction(
            name.toStdString()));
    m_compositeFunction =
        boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(m_function);
    m_peakFunction =
        boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(m_function);
  } catch (...) {
    m_function.reset();
    m_compositeFunction.reset();
    m_peakFunction.reset();
  }
}

void IFunctionWrapper::setFunction(
    boost::shared_ptr<Mantid::API::IFunction> function) {
  m_function = std::move(function);
  m_compositeFunction =
      boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(m_function);
  m_peakFunction =
      boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(m_function);
}
