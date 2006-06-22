#ifndef TRIGT1CALOBYTESTREAM_JEMTESTER_H
#define TRIGT1CALOBYTESTREAM_JEMTESTER_H

#include <map>
#include <string>

#include "DataModel/DataVector.h"
#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/MsgStream.h"

class StoreGateSvc;

namespace LVL1 {
  class JetElement;
  class JetElementKey;
}

/** Algorithm to test Jet element bytestream conversions.
 *
 *  Just prints out the contents of the JetElement objects.
 *  Run before writing bytestream and after reading it and compare.
 *
 *  @author Peter Faulkner
 */

class JemTester : public Algorithm {

 public:
   JemTester(const std::string& name, ISvcLocator* pSvcLocator);
   virtual ~JemTester();

   virtual StatusCode initialize();
   virtual StatusCode execute();
   virtual StatusCode finalize();

 private:
   /// Jet element container
   typedef DataVector<LVL1::JetElement> JetElementCollection;
   /// Jet element map
   typedef std::map<unsigned int, LVL1::JetElement*> JetElementMap;

   /// Find a jet element given eta, phi
   LVL1::JetElement* findJetElement(double eta, double phi);

   /// Print the jet elements
   void printJetElements(MsgStream& log, MSG::Level level);

   /// Print a vector
   void printVec(const std::vector<int>& vec, MsgStream& log,
                                              MSG::Level level);

   /// Set up jet element map
   void setupJeMap(const JetElementCollection* jeCollection);

   /// Jet element key provider
   LVL1::JetElementKey* m_elementKey;
   /// StoreGate service
   StoreGateSvc* m_storeGate;
   /// Jet element container StoreGate key
   std::string m_jetElementLocation;

   /// Jet element map
   JetElementMap m_jeMap;

};

#endif
