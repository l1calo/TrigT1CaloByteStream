#ifndef TRIGT1CALOBYTESTREAM_JEPCONTAINER_H
#define TRIGT1CALOBYTESTREAM_JEPCONTAINER_H

#include "CLIDSvc/CLASS_DEF.h"
#include "DataModel/DataVector.h"

namespace LVL1 {
  class JetElement;
  class JEMHits;
  class JEMEtSums;
}

/** JEP container for writing bytestream.
 *
 *  Contains all the component collections needed to build the ROD.
 *
 *  @author Peter Faulkner
 */

class JepContainer {

 public:
   JepContainer(const DataVector<LVL1::JetElement>* jeCollection,
                const DataVector<LVL1::JEMHits>*    hitCollection,
		const DataVector<LVL1::JEMEtSums>*  etCollection);

   /// Return pointer to jet element collection
   const DataVector<LVL1::JetElement>* JetElements() const;
   /// Return pointer to hit sums collection
   const DataVector<LVL1::JEMHits>*    JetHits()     const;
   /// Return pointer to energy sums collection
   const DataVector<LVL1::JEMEtSums>*  EnergySums()  const;

 private:

   /// Jet element collection
   const DataVector<LVL1::JetElement>* m_jeCollection;
   /// Hit sums collection
   const DataVector<LVL1::JEMHits>*    m_hitCollection;
   /// Energy sums collection
   const DataVector<LVL1::JEMEtSums>*  m_etCollection;

};

inline const DataVector<LVL1::JetElement>* JepContainer::JetElements() const
{
  return m_jeCollection;
}

inline const DataVector<LVL1::JEMHits>* JepContainer::JetHits() const
{
  return m_hitCollection;
}

inline const DataVector<LVL1::JEMEtSums>* JepContainer::EnergySums() const
{
  return m_etCollection;
}

CLASS_DEF(JepContainer, 1321678566, 1)

#endif
