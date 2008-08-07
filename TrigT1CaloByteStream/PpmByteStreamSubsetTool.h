#ifndef TRIGT1CALOBYTESTREAM_PPMBYTESTREAMSUBSETTOOL_H
#define TRIGT1CALOBYTESTREAM_PPMBYTESTREAMSUBSETTOOL_H

#include <stdint.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "ByteStreamCnvSvcBase/ROBDataProviderSvc.h"
#include "ByteStreamData/RawEvent.h"
#include "DataModel/DataVector.h"
#include "eformat/SourceIdentifier.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/ToolHandle.h"

#include "TrigT1CaloByteStream/IPpmByteStreamSubsetTool.h"
#include "TrigT1CaloByteStream/IPpmCrateMappingTool.h"

class IInterface;

namespace LVL1 {
  class TriggerTower;
  class TriggerTowerKey;
}

namespace LVL1BS {

class ChannelCoordinate;
class L1CaloSrcIdMap;
class PpmSubBlock;

/** Tool to perform ROB fragments to trigger towers conversions.
 *
 *  Based on ROD document version 1_09h.
 *
 *  @author Peter Faulkner
 */

class PpmByteStreamSubsetTool : virtual public IPpmByteStreamSubsetTool,
                                        public AlgTool {

 public:
   PpmByteStreamSubsetTool(const std::string& type, const std::string& name,
                           const IInterface* parent);
   virtual ~PpmByteStreamSubsetTool();

   virtual StatusCode initialize();
   virtual StatusCode finalize();

   /// Convert ROB fragments to trigger towers
   virtual StatusCode convert(const IROBDataProviderSvc::VROBFRAG& robFrags,
                              DataVector<LVL1::TriggerTower>* ttCollection,
		              const std::vector<unsigned int> chanIds);

 private:
   typedef DataVector<LVL1::TriggerTower>                TriggerTowerCollection;
   typedef std::map<unsigned int, LVL1::TriggerTower*>   TriggerTowerMap;
   typedef IROBDataProviderSvc::VROBFRAG::const_iterator ROBIterator;
   typedef OFFLINE_FRAGMENTS_NAMESPACE::PointerType      RODPointer;
   typedef std::pair<std::vector<unsigned int>::const_iterator,
                     std::vector<unsigned int>::const_iterator> IteratorPair;
   typedef std::map<unsigned int, IteratorPair>          ChannelMap;

   /// Correction for had FCAL eta which is adjusted to EM value in TriggerTower
   double etaSim(const ChannelCoordinate& coord) const;

   /// Find a trigger tower given eta, phi
   LVL1::TriggerTower* findTriggerTower(double eta, double phi);

   /// Print a vector
   void printVec(const std::vector<int>& vec, const MSG::Level level) const;

   /// Tool for mappings
   ToolHandle<LVL1BS::IPpmCrateMappingTool> m_ppmMaps;
   /// class member version of retrieving MsgStream
   mutable MsgStream m_log;
   bool m_debug;

   /// Number of channels per module (may not all be used)
   int m_channels;
   /// Number of crates
   int m_crates;
   /// Number of modules per crate (may not all exist)
   int m_modules;
   /// Sub-detector type
   eformat::SubDetector m_subDetector;
   /// Zero suppression on input
   bool m_zeroSuppress;
   /// Source ID converter
   L1CaloSrcIdMap* m_srcIdMap;
   /// Trigger tower key provider
   LVL1::TriggerTowerKey* m_towerKey;
   /// Current error block
   PpmSubBlock* m_errorBlock;
   /// Vector for current PPM sub-blocks
   DataVector<PpmSubBlock> m_ppmBlocks;
   /// Trigger tower map for conversion from bytestream
   TriggerTowerMap m_ttMap;

};

} // end namespace

#endif
