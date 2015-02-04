#ifndef TRIGT1CALOBYTESTREAM_PPMBYTESTREAMXAODREADTOOL_H
#define TRIGT1CALOBYTESTREAM_PPMBYTESTREAMXAODREADTOOL_H
// ===========================================================================
// Includes
// ===========================================================================
// STD:
// ===========================================================================
#include <stdint.h>

#include <vector>

// ===========================================================================
// Athena:
// ===========================================================================
#include "AsgTools/AsgTool.h"
#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"

#include "xAODTrigL1Calo/TriggerTower.h"
#include "xAODTrigL1Calo/TriggerTowerContainer.h"

// ===========================================================================
// Forward declarations
// ===========================================================================


// ===========================================================================
namespace LVL1BS {

// Forward declarations
class L1CaloSrcIdMap;
// ===========================================================================

/** Tool to perform ROB fragments to trigger towers and trigger towers
 *  to raw data conversions.
 *
 *
 * @author alexander.mazurov@cern.ch
 */

class PpmByteStreamxAODReadTool: public asg::AsgTool {
	ASG_TOOL_INTERFACE(PpmByteStreamxAODReadTool)
	ASG_TOOL_CLASS0(PpmByteStreamxAODReadTool)
public:
  PpmByteStreamxAODReadTool(const std::string& name);
  virtual ~PpmByteStreamxAODReadTool(){};

  virtual StatusCode initialize();
  virtual StatusCode finalize();

  /// Convert ROB fragments to trigger towers
  StatusCode convert(
    const IROBDataProviderSvc::VROBFRAG& robFrags,
    xAOD::TriggerTowerContainer* const ttCollection
  );


  /// Return reference to vector with all possible Source Identifiers
  const std::vector<uint32_t>& sourceIDs(const std::string& sgKey);

private:
  typedef IROBDataProviderSvc::VROBFRAG::const_iterator ROBIterator;
  typedef OFFLINE_FRAGMENTS_NAMESPACE::PointerType      ROBPointer;
  typedef OFFLINE_FRAGMENTS_NAMESPACE::PointerType      RODPointer;


private:
  StatusCode processRobFragment_(const ROBIterator& robFrag);


private:
  uint16_t m_crates;
  uint16_t m_modules;
  uint16_t m_channels;
  uint16_t m_dataSize;
  uint16_t m_maxSlinks;

private:
  std::vector<uint32_t> m_sourceIDs;
  L1CaloSrcIdMap* m_srcIdMap;
};

// ===========================================================================
}// end namespace
// ===========================================================================
#endif
