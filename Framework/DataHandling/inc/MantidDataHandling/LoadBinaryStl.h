// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <utility>



#include <utility>



#include "MantidDataHandling/LoadStl.h"

namespace Mantid {
namespace Kernel {
class BinaryStreamReader;
}

namespace Geometry {
class MeshObject;
}
namespace DataHandling {

class DLLExport LoadBinaryStl : public LoadStl {
public:
  static constexpr int HEADER_SIZE = 80;
  static constexpr uint32_t TRIANGLE_DATA_SIZE = 50;
  static constexpr uint32_t TRIANGLE_COUNT_DATA_SIZE = 4;
  static constexpr uint32_t VECTOR_DATA_SIZE = 12;
  LoadBinaryStl(std::string filename, ScaleUnits scaleType)
      : LoadStl(std::move(std::move(filename)), scaleType) {}
  LoadBinaryStl(std::string filename, ScaleUnits scaleType,
                ReadMaterial::MaterialParameters params)
      : LoadStl(std::move(std::move(filename)), scaleType, std::move(std::move(params))) {}
  std::unique_ptr<Geometry::MeshObject> readStl() override;
  static bool isBinarySTL(const std::string& filename);

private:
  void readTriangle(Kernel::BinaryStreamReader, uint32_t &vertexCount);
};

} // namespace DataHandling
} // namespace Mantid