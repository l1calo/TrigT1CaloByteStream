#ifndef TRIGT1CALOBYTESTREAM_PPMBYTESTREAMTOOL2_H
#define TRIGT1CALOBYTESTREAM_PPMBYTESTREAMTOOL2_H
// ===========================================================================
// Includes
// ===========================================================================
// STD:
// ===========================================================================
#include <stdint.h>

#include <map>
#include <string>
#include <vector>
// ===========================================================================
// Atheana:
// ===========================================================================
#include "AthenaBaseComps/AthAlgTool.h"
#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"
#include "ByteStreamData/RawEvent.h"
#include "DataModel/DataVector.h"
#include "eformat/SourceIdentifier.h"

#include "GaudiKernel/ServiceHandle.h"
#include "GaudiKernel/ToolHandle.h"


#include "xAODTrigL1Calo/TriggerTower.h"
#include "xAODTrigL1Calo/TriggerTowerContainer.h"

// ===========================================================================
// Forward declarations
// ===========================================================================
class IInterface;
class InterfaceID;
class SegMemSvc;

namespace xAOD {
  class TriggerTower_v2;
}

// ===========================================================================
namespace LVL1BS {
// ===========================================================================
// class L1CaloErrorByteStreamTool;
class L1CaloSrcIdMap;
// class PpmSubBlock;

/** Tool to perform ROB fragments to trigger towers and trigger towers
 *  to raw data conversions.
 *
 *  Based on ROD document version 1_09h.
 *
 *  @author Peter Faulkner
 */

class PpmByteStreamTool2: public AthAlgTool {

public:
  PpmByteStreamTool2(const std::string& type, const std::string& name,
      const IInterface* parent);
  virtual ~PpmByteStreamTool2();

  /// AlgTool InterfaceID
  static const InterfaceID& interfaceID();

  virtual StatusCode initialize();
  virtual StatusCode finalize();

  /// Convert ROB fragments to trigger towers
  StatusCode convert(const IROBDataProviderSvc::VROBFRAG& robFrags,
      xAOD::TriggerTowerContainer* ttCollection);

  /// Convert trigger towers to bytestream
  StatusCode convert(const xAOD::TriggerTowerContainer* ttCollection,
      RawEventWrite* re);

  /// Return reference to vector with all possible Source Identifiers
  const std::vector<uint32_t>& sourceIDs(const std::string& sgKey);

private:
  void reserveMemory();
  private:
  // typedef DataVector<LVL1::TriggerTower2> TriggerTowerCollection;
  // typedef std::vector<LVL1::TriggerTower2*> TriggerTowerVector;

  typedef enum {
    Data, Spare, Muon
  } ChannelsType;
  /// Services
  ServiceHandle<SegMemSvc> m_sms;
  /// Source ID converter
  L1CaloSrcIdMap* m_srcIdMap;
  /// Sub-detector type
  const eformat::SubDetector m_subDetector;
  int m_crates;
  int m_modules;
  int m_channels;
  int m_dataSize;
  int m_maxSlinks;

  ChannelsType m_channelsType;

  /// ROB source IDs
  std::vector<uint32_t> m_sourceIDs;
  std::vector<uint32_t> m_sourceIDsSpare;
  std::vector<uint32_t> m_sourceIDsMuon;

  /// TriggerTower pool vectors
  // TriggerTowerVector m_ttData;
  // TriggerTowerVector m_ttSpare;
  // TriggerTowerVector m_ttMuon;

};
// ===========================================================================
}// end namespace
// ===========================================================================
#endif
