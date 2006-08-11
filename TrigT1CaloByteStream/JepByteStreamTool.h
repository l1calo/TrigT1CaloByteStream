#ifndef TRIGT1CALOBYTESTREAM_JEPBYTESTREAMTOOL_H
#define TRIGT1CALOBYTESTREAM_JEPBYTESTREAMTOOL_H

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

class ChannelCoordinate;
class IInterface;
class InterfaceID;
class JemCrateMappings;
class JemSubBlock;
class JepContainer;
namespace LVL1 {
  class JetElement;
  class JetElementKey;
  class JEMHits;
  class JEMEtSums;
}

/** Tool to perform ROB fragments to jet elements, jet hits and energy sums,
 *  and JEP container to raw data conversions.
 *
 *  Based on ROD document version 1_06d.
 *
 *  @author Peter Faulkner
 */

class JepByteStreamTool : public AlgTool {

 public:
   JepByteStreamTool(const std::string& type, const std::string& name,
                     const IInterface* parent);
   virtual ~JepByteStreamTool();

   /// AlgTool InterfaceID
   static const InterfaceID& interfaceID();

   virtual StatusCode initialize();
   virtual StatusCode finalize();

   /// Convert ROB fragments to jet elements
   StatusCode convert(const IROBDataProviderSvc::VROBFRAG& robFrags,
                      DataVector<LVL1::JetElement>* jeCollection);
   /// Convert ROB fragments to jet hits
   StatusCode convert(const IROBDataProviderSvc::VROBFRAG& robFrags,
                      DataVector<LVL1::JEMHits>* hitCollection);
   /// Convert ROB fragments to energy sums
   StatusCode convert(const IROBDataProviderSvc::VROBFRAG& robFrags,
                      DataVector<LVL1::JEMEtSums>* etCollection);

   /// Convert JepContainer to bytestream
   StatusCode convert(const JepContainer* jep, RawEventWrite* re);

   /// Fill a vector with all possible Source Identifiers
   void sourceIDs(std::vector<uint32_t>& vID) const;

 private:
   enum CollectionType { JET_ELEMENTS, JET_HITS, ENERGY_SUMS };

   typedef DataVector<LVL1::JetElement>                  JetElementCollection;
   typedef DataVector<LVL1::JEMHits>                     JetHitsCollection;
   typedef DataVector<LVL1::JEMEtSums>                   EnergySumsCollection;
   typedef std::map<unsigned int, LVL1::JetElement*>     JetElementMap;
   typedef std::map<int, LVL1::JEMHits*>                 JetHitsMap;
   typedef std::map<int, LVL1::JEMEtSums*>               EnergySumsMap;
   typedef IROBDataProviderSvc::VROBFRAG::const_iterator ROBIterator;
   typedef OFFLINE_FRAGMENTS_NAMESPACE::PointerType      RODPointer;

   /// Convert bytestream to given container type
   StatusCode convertBs(const IROBDataProviderSvc::VROBFRAG& robFrags,
                        CollectionType collection);

   /// Print jet element data values
   void printJeData(int channel, const ChannelCoordinate& coord,
        const std::vector<int>& emData,   const std::vector<int>& hadData,
	const std::vector<int>& emParity, const std::vector<int>& hadParity,
	const std::vector<int>& linkError, MsgStream& log, MSG::Level level);
   /// Print vector values
   void printVec(const std::vector<int>& vec, MsgStream& log,
                                              MSG::Level level);

   /// Find a jet element given eta, phi
   LVL1::JetElement* findJetElement(double eta, double phi);
   /// Find jet hits for given crate, module
   LVL1::JEMHits*    findJetHits(int crate, int module);
   /// Find energy sums for given crate, module
   LVL1::JEMEtSums*  findEnergySums(int crate, int module);

   /// Set up jet element map
   void setupJeMap(const JetElementCollection* jeCollection);
   /// Set up jet hits map
   void setupHitsMap(const JetHitsCollection* hitCollection);
   /// Set up energy sums map
   void setupEtMap(const EnergySumsCollection* enCollection);

   /// Get number of slices and triggered slice offset for next slink
   bool slinkSlices(int crate, int module, int modulesPerSlink,
                    int& timeslices, int& trigJem);

   /// Sub_block header version
   int m_version;
   /// Data compression format
   int m_dataFormat;
   /// Number of channels per module
   int m_channels;
   /// Number of crates
   int m_crates;
   /// Number of modules per crate
   int m_modules;
   /// Number of slinks per crate when writing out bytestream
   int m_slinks;
   /// Sub-detector type
   eformat::SubDetector m_subDetector;
   /// Source ID converter
   L1CaloSrcIdMap* m_srcIdMap;
   /// JEM crate mappings
   JemCrateMappings* m_jemMaps;
   /// Jet element key provider
   LVL1::JetElementKey* m_elementKey;
   /// Vector for current JEM sub-blocks
   DataVector<JemSubBlock> m_jemBlocks;
   /// Current jet elements collection
   JetElementCollection* m_jeCollection;
   /// Current jet hits collection
   JetHitsCollection*    m_hitCollection;
   /// Current energy sums collection
   EnergySumsCollection* m_etCollection;
   /// Jet element map
   JetElementMap m_jeMap;
   /// Jet hits map
   JetHitsMap    m_hitsMap;
   /// Energy sums map
   EnergySumsMap m_etMap;
   /// Event assembler
   FullEventAssembler<L1CaloSrcIdMap> m_fea;

};

#endif
