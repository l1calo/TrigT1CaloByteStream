#ifndef TRIGT1CALOBYTESTREAM_PPMBYTESTREAMTOOL_H
#define TRIGT1CALOBYTESTREAM_PPMBYTESTREAMTOOL_H

#include <string>
#include <vector>
#include <map>

#include "GaudiKernel/AlgTool.h"
#include "DataModel/DataVector.h"

#include "ByteStreamData/RawEvent.h"
#include "ByteStreamCnvSvcBase/FullEventAssembler.h"
#include "ByteStreamCnvSvcBase/ROBDataProviderSvc.h"

#include "TrigT1Calo/TriggerTower.h"
#include "TrigT1Calo/TriggerTowerKey.h"

#include "TrigT1CaloByteStream/ChannelCoordinate.h"
#include "TrigT1CaloByteStream/L1CaloSrcIdMap.h"

class MsgStream;
class PpmCrateMappings;
class PpmSortPermutations;
class PpmSubBlock;
class PpmErrorBlock;

/** Tool to perform ROB fragments to trigger towers and trigger towers
 *  to raw data conversions.
 *
 *  Based on ROD document version 1_06d.
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

   /// Dump data values to DEBUG
   void dumpDebug(int channel, const ChannelCoordinate& coord,
        const std::vector<int>& lut, const std::vector<int>& fadc,
	const std::vector<int>& bcidLut, const std::vector<int>& bcidFadc,
	int error, MsgStream& log);

   /// Hack for had FCAL eta which is adjusted to EM value in TriggerTower
   double etaHack(const ChannelCoordinate& coord);

   /// Find a trigger tower using separate layer maps
   LVL1::TriggerTower* findLayerTriggerTower(const ChannelCoordinate& coord);

   /// Find a trigger tower given eta, phi
   LVL1::TriggerTower* findTriggerTower(double eta, double phi);

   /// Set up separate Em and Had trigger tower maps
   void setupTTMaps(const DataVector<LVL1::TriggerTower>* ttCollection);

   /// Get number of slices and triggered slice offsets for next slink
   bool slinkSlices(int crate, int module, int modulesPerSlink,
        int& slicesLut, int& slicesFadc, int& trigLut, int& trigFadc);

   /// Sub_block header version
   int m_version;
   /// Data compression format
   int m_dataFormat;
   /// Number of channels per module (may not all be used)
   int m_channels;
   /// Number of channels per sub-block
   int m_channelsPerSubBlock;
   /// Number of error words per error block
   int m_glinkPins;
   /// Number of crates
   int m_crates;
   /// Number of modules per crate (may not all exist)
   int m_modules;
   /// Number of slinks per crate when writing out bytestream
   int m_slinks;
   /// Source ID converter
   L1CaloSrcIdMap* m_srcIdMap;
   /// PPM crate mappings
   PpmCrateMappings* m_ppmMaps;
   /// Sort permutation provider
   PpmSortPermutations* m_sortPerms;
   /// Trigger tower key provider
   LVL1::TriggerTowerKey* m_towerKey;
   /// Current error block
   PpmErrorBlock* m_errorBlock;
   /// Vector for current PPM sub-blocks
   DataVector<PpmSubBlock> m_ppmBlocks;
   /// Trigger tower map for conversion from bytestream
   std::map<int, LVL1::TriggerTower*> m_ttMap;
   /// Trigger tower map for conversion EM to bytestream
   std::map<int, LVL1::TriggerTower*> m_ttEmMap;
   /// Trigger tower map for conversion Had to bytestream
   std::map<int, LVL1::TriggerTower*> m_ttHadMap;
   /// Event assembler
   FullEventAssembler<L1CaloSrcIdMap> m_fea;

};

#endif
