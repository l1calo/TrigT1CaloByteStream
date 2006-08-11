#ifndef TRIGT1CALOBYTESTREAM_JEPCONTAINERMAKER_H
#define TRIGT1CALOBYTESTREAM_JEPCONTAINERMAKER_H

#include <string>

#include "DataModel/DataVector.h"
#include "GaudiKernel/Algorithm.h"

class StoreGateSvc;

namespace LVL1 {
  class JetElement;
  class JEMHits;
  class JEMEtSums;
}

/** Algorithm to make JEP container to write bytestream.
 *
 *  @author Peter Faulkner
 */

class JepContainerMaker : public Algorithm {

 public:
   JepContainerMaker(const std::string& name, ISvcLocator* pSvcLocator);
   virtual ~JepContainerMaker();

   virtual StatusCode initialize();
   virtual StatusCode execute();
   virtual StatusCode finalize();

 private:
   typedef DataVector<LVL1::JetElement> JetElementCollection;
   typedef DataVector<LVL1::JEMHits>    JetHitsCollection;
   typedef DataVector<LVL1::JEMEtSums>  EnergySumsCollection;

   /// StoreGate service
   StoreGateSvc* m_storeGate;
   /// Jet element container StoreGate key
   std::string m_jetElementLocation;
   /// Jet hits container StoreGate key
   std::string m_jemHitsLocation;
   /// Energy sums container StoreGate key
   std::string m_jemEtSumsLocation;
   /// JEP container StoreGate key
   std::string m_jepContainerLocation;

};

#endif
