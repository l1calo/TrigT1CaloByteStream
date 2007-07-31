#ifndef TRIGT1CALOBYTESTREAM_L1CALORODSTATUS_H
#define TRIGT1CALOBYTESTREAM_L1CALORODSTATUS_H

#include <map>
#include <vector>

#include "ByteStreamData/RawEvent.h"

namespace LVL1BS {

class L1CaloSrcIdMap;

/** Utility to set the ROD status words
 *
 *  @author Peter Faulkner
 */

class L1CaloRodStatus {

 public:
   L1CaloRodStatus();
   ~L1CaloRodStatus();

   typedef std::vector<uint32_t>               RODStatusWords;
   typedef std::map<uint32_t, RODStatusWords*> RODStatusMap;

   /// Set the ROD status words for given ROD IDs
   static void setStatus(RawEventWrite* re, const RODStatusMap& rodMap,
                         L1CaloSrcIdMap* srcIdMap);

 private:

   typedef OFFLINE_FRAGMENTS_NAMESPACE_WRITE::SubDetectorFragment DetFrag;
   typedef OFFLINE_FRAGMENTS_NAMESPACE_WRITE::ROSFragment         RosFrag;
   typedef OFFLINE_FRAGMENTS_NAMESPACE_WRITE::ROBFragment         RobFrag;

};

} // end namespace

#endif
