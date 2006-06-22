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
}

/** Tool to perform ROB fragments to jet elements and JEP container
 *  to raw data conversions.
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

   /// Convert JepContainer to bytestream
   StatusCode convert(const JepContainer* jep, RawEventWrite* re);

   /// Fill a vector with all possible Source Identifiers
   void sourceIDs(std::vector<uint32_t>& vID) const;

 private:
   /// Jet element container
   typedef DataVector<LVL1::JetElement>                  JetElementCollection;
   /// Jet element map
   typedef std::map<unsigned int, LVL1::JetElement*>     JetElementMap;
   /// ROB fragments iterator
   typedef IROBDataProviderSvc::VROBFRAG::const_iterator ROBIterator;
   /// ROD pointer
   typedef OFFLINE_FRAGMENTS_NAMESPACE::PointerType      RODPointer;

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

   /// Set up jet element map
   void setupJeMap(const JetElementCollection* jeCollection);

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
   /// Jet element map
   JetElementMap m_jeMap;
   /// Event assembler
   FullEventAssembler<L1CaloSrcIdMap> m_fea;

};

#endif
