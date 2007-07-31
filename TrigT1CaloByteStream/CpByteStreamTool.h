#ifndef TRIGT1CALOBYTESTREAM_CPBYTESTREAMTOOL_H
#define TRIGT1CALOBYTESTREAM_CPBYTESTREAMTOOL_H

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "ByteStreamCnvSvcBase/FullEventAssembler.h"
#include "ByteStreamCnvSvcBase/ROBDataProviderSvc.h"
#include "ByteStreamData/RawEvent.h"
#include "DataModel/DataVector.h"
#include "eformat/SourceIdentifier.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/MsgStream.h"

#include "TrigT1CaloByteStream/L1CaloSrcIdMap.h"

class IInterface;
class InterfaceID;

namespace LVL1 {
  class CMMCPHits;
  class CPMHits;
  class CPMTower;
  class CPBSCollection;
  class TriggerTowerKey;
}

namespace LVL1BS {

class ChannelCoordinate;
class CmmCpSubBlock;
class CpmCrateMappings;
class CpmSubBlock;

/** Tool to perform ROB fragments to CPM towers, CPM hits and CMM-CP hits,
 *  and CP container to raw data conversions.
 *
 *  Based on ROD document version 1_09h.
 *
 *  @author Peter Faulkner
 */

class CpByteStreamTool : public AlgTool {

 public:
   CpByteStreamTool(const std::string& type, const std::string& name,
                     const IInterface* parent);
   virtual ~CpByteStreamTool();

   /// AlgTool InterfaceID
   static const InterfaceID& interfaceID();

   virtual StatusCode initialize();
   virtual StatusCode finalize();

   /// Convert ROB fragments to CPM towers
   StatusCode convert(const IROBDataProviderSvc::VROBFRAG& robFrags,
                      DataVector<LVL1::CPMTower>* ttCollection);
   /// Convert ROB fragments to CPM hits
   StatusCode convert(const IROBDataProviderSvc::VROBFRAG& robFrags,
                      DataVector<LVL1::CPMHits>* hitCollection);
   /// Convert ROB fragments to CMM-CP hits
   StatusCode convert(const IROBDataProviderSvc::VROBFRAG& robFrags,
                      DataVector<LVL1::CMMCPHits>* hitCollection);

   /// Convert CP Container to bytestream
   StatusCode convert(const LVL1::CPBSCollection* cp, RawEventWrite* re);

   /// Fill a vector with all possible Source Identifiers
   void sourceIDs(std::vector<uint32_t>& vID) const;

 private:

   enum CollectionType { CPM_TOWERS, CPM_HITS, CMM_CP_HITS };

   typedef DataVector<LVL1::CPMTower>                    CpmTowerCollection;
   typedef DataVector<LVL1::CPMHits>                     CpmHitsCollection;
   typedef DataVector<LVL1::CMMCPHits>                   CmmCpHitsCollection;
   typedef std::map<unsigned int, LVL1::CPMTower*>       CpmTowerMap;
   typedef std::map<int, LVL1::CPMHits*>                 CpmHitsMap;
   typedef std::map<int, LVL1::CMMCPHits*>               CmmCpHitsMap;
   typedef IROBDataProviderSvc::VROBFRAG::const_iterator ROBIterator;
   typedef OFFLINE_FRAGMENTS_NAMESPACE::PointerType      RODPointer;

   /// Convert bytestream to given container type
   StatusCode convertBs(const IROBDataProviderSvc::VROBFRAG& robFrags,
                        CollectionType collection);
   /// Unpack CMM-CP sub-block
   StatusCode decodeCmmCp(CmmCpSubBlock& subBlock, int trigCmm);
   /// Unpack CPM sub-block
   StatusCode decodeCpm(CpmSubBlock& subBlock, int trigCpm,
                                               CollectionType collection);

   /// Find a CPM tower given eta, phi
   LVL1::CPMTower*  findCpmTower(double eta, double phi);
   /// Find CPM hits for given crate, module
   LVL1::CPMHits*   findCpmHits(int crate, int module);
   /// Find CMM-CP hits for given crate, data ID
   LVL1::CMMCPHits* findCmmCpHits(int crate, int dataID);

   /// Set up CPM tower map
   void setupCpmTowerMap(const CpmTowerCollection* ttCollection);
   /// Set up CPM hits map
   void setupCpmHitsMap(const CpmHitsCollection* hitCollection);
   /// Set up CMM-CP hits map
   void setupCmmCpHitsMap(const CmmCpHitsCollection* hitCollection);

   /// Get number of slices and triggered slice offset for next slink
   bool slinkSlices(int crate, int module, int modulesPerSlink,
                    int& timeslices, int& trigJem);
   /// Get number of CMM slices and triggered slice offset for current crate
   bool slinkSlicesCmm(int crate, int& timeslices, int& trigCmm);

   /// Hardware crate number offset
   int m_crateOffsetHw;
   /// Software crate number offset
   int m_crateOffsetSw;
   /// Sub_block header version
   int m_version;
   /// Data compression format
   int m_dataFormat;
   /// Number of channels per module
   int m_channels;
   /// Number of crates
   int m_crates;
   /// Number of CPM modules per crate
   int m_modules;
   /// Number of slinks per crate when writing out bytestream
   int m_slinks;
   /// Default number of CPM slices in simulation
   int m_dfltSlicesCpm;
   /// Default number of CMM slices in simulation
   int m_dfltSlicesCmm;
   /// Force number of CPM slices in bytestream
   int m_forceSlicesCpm;
   /// Force number of CMM slices in bytestream
   int m_forceSlicesCmm;
   /// ROB source IDs
   std::vector<uint32_t> m_sourceIDs;
   /// Sub-detector type
   eformat::SubDetector m_subDetector;
   /// Source ID converter
   L1CaloSrcIdMap* m_srcIdMap;
   /// CPM crate mappings
   CpmCrateMappings* m_cpmMaps;
   /// Trigger tower key provider
   LVL1::TriggerTowerKey* m_towerKey;
   /// Vector for current CPM sub-blocks
   DataVector<CpmSubBlock> m_cpmBlocks;
   /// Vector for current CMM-CP hit0 sub-blocks
   DataVector<CmmCpSubBlock> m_cmmHit0Blocks;
   /// Vector for current CMM-CP hit1 sub-blocks
   DataVector<CmmCpSubBlock> m_cmmHit1Blocks;
   /// Current CPM tower collection
   CpmTowerCollection*  m_ttCollection;
   /// Current CPM hits collection
   CpmHitsCollection*   m_hitCollection;
   /// Current CMM-CP hits collection
   CmmCpHitsCollection* m_cmmHitCollection;
   /// CPM tower map
   CpmTowerMap  m_ttMap;
   /// CPM hits map
   CpmHitsMap   m_hitsMap;
   /// CMM-CP hits map
   CmmCpHitsMap m_cmmHitsMap;
   /// ROD Status words
   std::vector<uint32_t>* m_rodStatus;
   /// ROD status map
   std::map<uint32_t, std::vector<uint32_t>* > m_rodStatusMap;
   /// Event assembler
   FullEventAssembler<L1CaloSrcIdMap> m_fea;

};

} // end namespace

#endif
