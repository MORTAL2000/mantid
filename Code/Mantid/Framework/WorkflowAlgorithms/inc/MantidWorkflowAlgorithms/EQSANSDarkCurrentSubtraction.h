#ifndef MANTID_ALGORITHMS_EQSANSDARKCURRENTSUBTRACTION_H_
#define MANTID_ALGORITHMS_EQSANSDARKCURRENTSUBTRACTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
/**

    Subtract dark current for EQSANS.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> MinEfficiency - Minimum efficiency for a pixel to be considered (default: no minimum)</LI>
    <LI> MaxEfficiency - Maximum efficiency for a pixel to be considered (default: no maximum)</LI>
    <LI> Factor        - Exponential factor for detector efficiency as a function of wavelength (default: 1.0)</LI>
    <LI> Error         - Error on Factor property (default: 0.0)</LI>
    </UL>

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport EQSANSDarkCurrentSubtraction : public API::Algorithm
{
public:
  /// (Empty) Constructor
  EQSANSDarkCurrentSubtraction() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~EQSANSDarkCurrentSubtraction() {}
  /// Algorithm's name
  virtual const std::string name() const { return "EQSANSDarkCurrentSubtraction"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow\\SANS"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSDARKCURRENTSUBTRACTION_H_*/
