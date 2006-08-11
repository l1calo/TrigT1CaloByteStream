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
  class JEMHits;
  class JEMEtSums;
}

/** Algorithm to test JEM component bytestream conversions.
 *
 *  Just prints out the contents of the JetElement objects,
 *  JEMHits objects and JEMEtSums objects.
 *
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
   typedef DataVector<LVL1::JetElement>              JetElementCollection;
   typedef DataVector<LVL1::JEMHits>                 JetHitsCollection;
   typedef DataVector<LVL1::JEMEtSums>               EnergySumsCollection;
   typedef std::map<unsigned int, LVL1::JetElement*> JetElementMap;
   typedef std::map<int, LVL1::JEMHits*>             JetHitsMap;
   typedef std::map<int, LVL1::JEMEtSums*>           EnergySumsMap;

   /// Print the jet elements
   void printJetElements(MsgStream& log, MSG::Level level);
   /// Print the jet hits
   void printJetHits(MsgStream& log, MSG::Level level);
   /// Print the energy sums
   void printEnergySums(MsgStream& log, MSG::Level level);

   /// Print a vector
   void printVec(const std::vector<int>& vec, MsgStream& log,
                                              MSG::Level level);
   /// Print a vector (unsigned)
   void printVecU(const std::vector<unsigned int>& vec, MsgStream& log,
                                                   MSG::Level level);

   /// Set up jet element map
   void setupJeMap(const JetElementCollection* jeCollection);
   /// Set up jet hits map
   void setupHitsMap(const JetHitsCollection* hitCollection);
   /// Set up energy sums map
   void setupEtMap(const EnergySumsCollection* etCollection);

   /// Jet element key provider
   LVL1::JetElementKey* m_elementKey;
   /// StoreGate service
   StoreGateSvc* m_storeGate;
   /// Jet element container StoreGate key
   std::string m_jetElementLocation;
   /// Jet hits container StoreGate key
   std::string m_jemHitsLocation;
   /// Energy sums container StoreGate key
   std::string m_jemEtSumsLocation;
   /// Jet element print flag
   int m_jetElementPrint;
   /// Jet hits print flag
   int m_jemHitsPrint;
   /// Energy sums print flag
   int m_jemEtSumsPrint;
   /// Number of JEM modules per crate
   int m_modules;

   /// Jet element map
   JetElementMap m_jeMap;
   /// Jet hits map
   JetHitsMap    m_hitsMap;
   /// Energy sums map
   EnergySumsMap m_etMap;

};

#endif
