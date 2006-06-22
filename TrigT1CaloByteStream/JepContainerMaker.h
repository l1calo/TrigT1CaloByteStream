#ifndef TRIGT1CALOBYTESTREAM_JEPCONTAINERMAKER_H
#define TRIGT1CALOBYTESTREAM_JEPCONTAINERMAKER_H

#include <string>

#include "DataModel/DataVector.h"
#include "GaudiKernel/Algorithm.h"

class StoreGateSvc;

namespace LVL1 {
  class JetElement;
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
   /// Jet element container
   typedef DataVector<LVL1::JetElement> JetElementCollection;

   /// StoreGate service
   StoreGateSvc* m_storeGate;
   /// Jet element container StoreGate key
   std::string m_jetElementLocation;
   /// JEP container StoreGate key
   std::string m_jepContainerLocation;

};

#endif
