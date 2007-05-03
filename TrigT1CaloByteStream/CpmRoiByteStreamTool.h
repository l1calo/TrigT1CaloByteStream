#ifndef TRIGT1CALOBYTESTREAM_CPMROIBYTESTREAMTOOL_H
#define TRIGT1CALOBYTESTREAM_CPMROIBYTESTREAMTOOL_H

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

#include "TrigT1CaloByteStream/CpmRoiSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloSrcIdMap.h"

class IInterface;
class InterfaceID;

namespace LVL1 {
  class CPMRoI;
}

namespace LVL1BS {

/** Tool to perform ROB fragments to CPM RoI and CPM RoI to raw data
 *  conversions.
 *
 *  Based on ROD document version 1_09h.
 *
 *  @author Peter Faulkner
 */

class CpmRoiByteStreamTool : public AlgTool {

 public:
   CpmRoiByteStreamTool(const std::string& type, const std::string& name,
                        const IInterface* parent);
   virtual ~CpmRoiByteStreamTool();

   /// AlgTool InterfaceID
   static const InterfaceID& interfaceID();

   virtual StatusCode initialize();
   virtual StatusCode finalize();

   /// Convert ROB fragments to CPM RoIs
   StatusCode convert(const IROBDataProviderSvc::VROBFRAG& robFrags,
                      DataVector<LVL1::CPMRoI>* roiCollection);

   /// Convert CPM RoI to bytestream
   StatusCode convert(const DataVector<LVL1::CPMRoI>* roiCollection,
                      RawEventWrite* re);

   /// Fill a vector with all possible Source Identifiers
   void sourceIDs(std::vector<uint32_t>& vID) const;

 private:

   typedef DataVector<LVL1::CPMRoI>                      CpmRoiCollection;
   typedef std::map<uint32_t, const LVL1::CPMRoI*>       CpmRoiMap;
   typedef IROBDataProviderSvc::VROBFRAG::const_iterator ROBIterator;
   typedef OFFLINE_FRAGMENTS_NAMESPACE::PointerType      RODPointer;

   /// Set up CPM RoI map
   void setupCpmRoiMap(const CpmRoiCollection* roiCollection);

   /// Hardware crate number offset
   int m_crateOffsetHw;
   /// Software crate number offset
   int m_crateOffsetSw;
   /// Sub_block header version
   int m_version;
   /// Data compression format
   int m_dataFormat;
   /// Number of crates
   int m_crates;
   /// Number of CPM modules per crate
   int m_modules;
   /// Number of slinks per crate when writing out bytestream
   int m_slinks;
   /// Sub-detector type
   eformat::SubDetector m_subDetector;
   /// Source ID converter
   L1CaloSrcIdMap* m_srcIdMap;
   /// Sub-block for neutral format
   CpmRoiSubBlock m_subBlock;
   /// CPM RoI map
   CpmRoiMap m_roiMap;
   /// Event assembler
   FullEventAssembler<L1CaloSrcIdMap> m_fea;

};

} // end namespace

#endif
