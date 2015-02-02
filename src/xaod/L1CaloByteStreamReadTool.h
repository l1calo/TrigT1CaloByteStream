#ifndef TRIGT1CALOBYTESTREAM_L1CALOBYTESTREAMREADTOOL_H
#define TRIGT1CALOBYTESTREAM_L1CALOBYTESTREAMREADTOOL_H
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
#include "GaudiKernel/ServiceHandle.h"

#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"
#include "TrigT1CaloMappingToolInterfaces/IL1CaloMappingTool.h"

#include "xAODTrigL1Calo/TriggerTower.h"
#include "xAODTrigL1Calo/TriggerTowerContainer.h"

#include "xAODTrigL1Calo/CPMTower.h"
#include "xAODTrigL1Calo/CPMTowerContainer.h"

#include "CaloUserHeader.h"
#include "SubBlockHeader.h"
#include "SubBlockStatus.h"
#include "CpmWord.h"

// ===========================================================================
// Forward declarations
// ===========================================================================


// ===========================================================================
namespace LVL1BS {

// Forward declarations
class L1CaloSrcIdMap;
class CpmWord;
// ===========================================================================

/** Tool to perform ROB fragments to trigger towers and trigger towers
 *  to raw data conversions.
 *
 *
 * @author alexander.mazurov@cern.ch
 */

class L1CaloByteStreamReadTool: public asg::AsgTool {
	ASG_TOOL_INTERFACE(L1CaloByteStreamReadTool)
	ASG_TOOL_CLASS0(L1CaloByteStreamReadTool)
public:
  L1CaloByteStreamReadTool(const std::string& name);
  virtual ~L1CaloByteStreamReadTool(){};

  virtual StatusCode initialize();
  virtual StatusCode finalize();

  // =========================================================================
  /// Convert ROB fragments to trigger towers
  StatusCode convert(
    const IROBDataProviderSvc::VROBFRAG& robFrags,
    xAOD::TriggerTowerContainer* const ttCollection
  );
  StatusCode convert(xAOD::TriggerTowerContainer* const ttCollection);
  // =========================================================================
  StatusCode convert(
      const IROBDataProviderSvc::VROBFRAG& robFrags,
      xAOD::CPMTowerContainer* const cpmCollection
    );
  StatusCode convert(xAOD::CPMTowerContainer* const cppCollection);
  // =========================================================================
  /// Return reference to vector with all possible Source Identifiers
  const std::vector<uint32_t>& ppmSourceIDs();
  const std::vector<uint32_t>& cpSourceIDs();

private:
  enum class RequestType { PPM, CPM, CMX };
  typedef IROBDataProviderSvc::VROBFRAG::const_iterator ROBIterator;
  typedef OFFLINE_FRAGMENTS_NAMESPACE::PointerType      ROBPointer;
  typedef OFFLINE_FRAGMENTS_NAMESPACE::PointerType      RODPointer;


private:
  StatusCode processRobFragment_(const ROBIterator& robFrag,
      const RequestType& requestedType);
  StatusCode processPpmWord_(uint32_t word, int indata);
  StatusCode processPpmBlock_();
  StatusCode processPpmBlockR4V1_();
  StatusCode processPpmStandardR4V1_();

  StatusCode processCpWord_(uint32_t word);
  StatusCode processCpmWordR4V1_(uint32_t word);
  StatusCode processPpmStandardR3V1_(uint32_t word, int indata);

  uint32_t getPpmBytestreamField_(uint8_t numBits);

  StatusCode addTriggerTowers_(
      uint8_t create,
      uint8_t module,
      uint8_t channel,
      const std::vector<uint8_t>& lcpVal,
      const std::vector<uint8_t>& lcpBcidVec,

      const std::vector<uint8_t>& ljeVal,
      const std::vector<uint8_t>& ljeSat80Vec,

      const std::vector<uint16_t>& adcVal,
      const std::vector<uint8_t>& adcExt,
      const std::vector<int16_t>& pedCor,
      const std::vector<uint8_t>& pedEn);

  StatusCode addCpmTower_(uint8_t crate, uint8_t module, const CpmWord& word);
private:
//  uint16_t m_crates;
//  uint16_t m_modules;
//  uint16_t m_channels;
//  uint16_t m_dataSize;
//  uint16_t m_maxSlinks;

private:
  ServiceHandle<SegMemSvc> m_sms;
  /// Channel mapping tool
  ToolHandle<LVL1::IL1CaloMappingTool> m_ppmMaps;
  ToolHandle<LVL1::IL1CaloMappingTool> m_cpmMaps;
  /// Service for reading bytestream
  ServiceHandle<IROBDataProviderSvc> m_robDataProvider;

private:
  CaloUserHeader m_caloUserHeader;
  SubBlockHeader m_subBlockHeader;
  SubBlockStatus m_subBlockStatus;

  uint8_t m_subDetectorID;
  RequestType m_requestedType;
  std::vector<uint32_t> m_ppmSourceIDs;
  std::vector<uint32_t> m_cpSourceIDs;
  L1CaloSrcIdMap* m_srcIdMap;

  uint32_t m_rodRunNumber;
  uint16_t m_rodVer;
  uint8_t m_verCode;

  std::vector<uint32_t> m_ppBlock;
  uint32_t m_ppPointer;
  uint32_t m_ppMaxBit;

private:
  xAOD::TriggerTowerContainer* m_triggerTowers;
  xAOD::CPMTowerContainer* m_cpmTowers;

};

// ===========================================================================
}// end namespace
// ===========================================================================
#endif
