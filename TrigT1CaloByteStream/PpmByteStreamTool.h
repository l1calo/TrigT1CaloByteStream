#ifndef TRIGT1CALOBYTESTREAM_PPMBYTESTREAMTOOL_H
#define TRIGT1CALOBYTESTREAM_PPMBYTESTREAMTOOL_H

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
  class TriggerTower;
  class TriggerTowerKey;
}

namespace LVL1BS {

class ChannelCoordinate;
class PpmCrateMappings;
class PpmSubBlock;

/** Tool to perform ROB fragments to trigger towers and trigger towers
 *  to raw data conversions.
 *
 *  Based on ROD document version 1_09h.
 *
 *  @author Peter Faulkner
 */

class PpmByteStreamTool : public AlgTool {

 public:
   PpmByteStreamTool(const std::string& type, const std::string& name,
                     const IInterface* parent);
   virtual ~PpmByteStreamTool();

   /// AlgTool InterfaceID
   static const InterfaceID& interfaceID();

   virtual StatusCode initialize();
   virtual StatusCode finalize();

   /// Convert ROB fragments to trigger towers
   StatusCode convert(const IROBDataProviderSvc::VROBFRAG& robFrags,
                      DataVector<LVL1::TriggerTower>* ttCollection);

   /// Convert trigger towers to bytestream
   StatusCode convert(const DataVector<LVL1::TriggerTower>* ttCollection,
                      RawEventWrite* re);

   /// Fill a vector with all possible Source Identifiers
   void sourceIDs(std::vector<uint32_t>& vID) const;

 private:
   typedef DataVector<LVL1::TriggerTower>                TriggerTowerCollection;
   typedef std::map<unsigned int, LVL1::TriggerTower*>   TriggerTowerMap;
   typedef std::map<unsigned int, const LVL1::TriggerTower*>
                                                         TriggerTowerMapConst;
   typedef IROBDataProviderSvc::VROBFRAG::const_iterator ROBIterator;
   typedef OFFLINE_FRAGMENTS_NAMESPACE::PointerType      RODPointer;

   /// Add compression stats to totals
   void addCompStats(const std::vector<uint32_t>& stats);
   /// Print compression stats
   void printCompStats(MsgStream& log, MSG::Level level) const;

   /// Hack for had FCAL eta which is adjusted to EM value in TriggerTower
   double etaHack(const ChannelCoordinate& coord) const;

   /// Find a trigger tower using separate layer maps
   const LVL1::TriggerTower* findLayerTriggerTower(const ChannelCoordinate&
                                                                       coord);

   /// Find a trigger tower given eta, phi
   LVL1::TriggerTower* findTriggerTower(double eta, double phi);

   /// Set up separate Em and Had trigger tower maps
   void setupTTMaps(const TriggerTowerCollection* ttCollection);

   /// Get number of slices and triggered slice offsets for next slink
   bool slinkSlices(int crate, int module, int modulesPerSlink,
        int& slicesLut, int& slicesFadc, int& trigLut, int& trigFadc);

   /// Sub_block header version
   int m_version;
   /// Data compression format
   int m_dataFormat;
   /// Compression version
   int m_compVers;
   /// Compression statistics print flag
   int m_printCompStats;
   /// Number of channels per module (may not all be used)
   int m_channels;
   /// Number of crates
   int m_crates;
   /// Number of modules per crate (may not all exist)
   int m_modules;
   /// Number of slinks per crate when writing out bytestream
   int m_slinks;
   /// Default number of LUT slices in simulation
   int m_dfltSlicesLut;
   /// Default number of FADC slices in simulation
   int m_dfltSlicesFadc;
   /// Force number of LUT slices in bytestream
   int m_forceSlicesLut;
   /// Force number of FADC slices in bytestream
   int m_forceSlicesFadc;
   /// Pedestal value
   int m_pedestal;
   /// Zero suppression on input
   int m_zeroSuppress;
   /// ROB source IDs
   std::vector<uint32_t> m_sourceIDs;
   /// Sub-detector type
   eformat::SubDetector m_subDetector;
   /// Source ID converter
   L1CaloSrcIdMap* m_srcIdMap;
   /// PPM crate mappings
   PpmCrateMappings* m_ppmMaps;
   /// Trigger tower key provider
   LVL1::TriggerTowerKey* m_towerKey;
   /// Current error block
   PpmSubBlock* m_errorBlock;
   /// Vector for current PPM sub-blocks
   DataVector<PpmSubBlock> m_ppmBlocks;
   /// Vector for compression statistics
   std::vector<uint32_t> m_compStats;
   /// Trigger tower map for conversion from bytestream
   TriggerTowerMap m_ttMap;
   /// Trigger tower map for conversion EM to bytestream
   TriggerTowerMapConst m_ttEmMap;
   /// Trigger tower map for conversion Had to bytestream
   TriggerTowerMapConst m_ttHadMap;
   /// ROD Status words
   std::vector<uint32_t>* m_rodStatus;
   /// ROD status map
   std::map<uint32_t, std::vector<uint32_t>* > m_rodStatusMap;
   /// Event assembler
   FullEventAssembler<L1CaloSrcIdMap> m_fea;

};

} // end namespace

#endif
