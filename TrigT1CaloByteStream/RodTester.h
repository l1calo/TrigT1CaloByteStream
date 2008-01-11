#ifndef TRIGT1CALOBYTESTREAM_RODTESTER_H
#define TRIGT1CALOBYTESTREAM_RODTESTER_H

#include <string>
#include <vector>

#include "DataModel/DataVector.h"
#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/ServiceHandle.h"
#include "StoreGate/StoreGateSvc.h"


namespace LVL1 {
  class RODHeader;
}

namespace LVL1BS {

/** Algorithm to test ROD header bytestream conversions.
 *
 *  Just prints out the contents of the RODHeader objects
 *
 *  @author Peter Faulkner
 */

class RodTester : public Algorithm {

 public:
   RodTester(const std::string& name, ISvcLocator* pSvcLocator);
   virtual ~RodTester();

   virtual StatusCode initialize();
   virtual StatusCode execute();
   virtual StatusCode finalize();

 private:
   typedef DataVector<LVL1::RODHeader> RodHeaderCollection;

   /// StoreGate service
   ServiceHandle<StoreGateSvc> m_storeGate;
   /// RODHeader container StoreGate key
   std::string m_rodHeaderLocation;
   /// Variant collection flags
   std::vector<std::string> m_flags;

};

} // end namespace

#endif
