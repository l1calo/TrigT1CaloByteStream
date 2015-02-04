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
#include "GaudiKernel/ToolHandle.h"

#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"
#include "TrigT1CaloMappingToolInterfaces/IL1CaloMappingTool.h"

#include "xAODTrigL1Calo/TriggerTower.h"
#include "xAODTrigL1Calo/TriggerTowerContainer.h"

#include "CaloUserHeader.h"
#include "SubBlockHeader.h"
#include "SubBlockStatus.h"

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
  StatusCode processPpmWord_(uint32_t word, int indata);
  StatusCode processPpmBlock_();
  StatusCode processPpmBlockR4V1_();
  StatusCode processPpmStandardR4V1_();

  uint32_t getPpmBytestreamField_(uint8_t numBits);

  StatusCode addTriggerTowers_(
      uint8_t create,
      uint8_t module,
      uint8_t channel,
      const std::vector<uint8_t>& lcpVal,
      const std::vector<uint8_t>& lcpExt,
      const std::vector<uint8_t>& lcpSat,
      const std::vector<uint8_t>& lcpPeak,

      const std::vector<uint8_t>& ljeVal,
      const std::vector<uint8_t>& ljeLow,
      const std::vector<uint8_t>& ljeHigh,
      const std::vector<uint8_t>& ljeRes,

      const std::vector<uint16_t>& adcVal,
      const std::vector<uint8_t>& adcExt,
      const std::vector<int16_t>& pedCor,
      const std::vector<uint8_t>& pedEn);

private:
  uint16_t m_crates;
  uint16_t m_modules;
  uint16_t m_channels;
  uint16_t m_dataSize;
  uint16_t m_maxSlinks;

private:
  /// Channel mapping tool
  ToolHandle<LVL1::IL1CaloMappingTool> m_ppmMaps;

private:
  CaloUserHeader m_caloUserHeader;
  SubBlockHeader m_subBlockHeader;
  SubBlockStatus m_subBlockStatus;

  std::vector<uint32_t> m_sourceIDs;
  L1CaloSrcIdMap* m_srcIdMap;

  uint32_t m_rodRunNumber;
  uint16_t m_rodVer;
  uint8_t m_verCode;

  std::vector<uint32_t> m_ppBlock;
  uint32_t m_ppPointer;
  uint32_t m_ppMaxBit;

private:
  xAOD::TriggerTowerContainer* m_triggerTowers;

};

// ===========================================================================
}// end namespace
// ===========================================================================
#endif
