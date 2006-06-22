#ifndef TRIGT1CALOBYTESTREAM_JEPCONTAINER_H
#define TRIGT1CALOBYTESTREAM_JEPCONTAINER_H

#include "CLIDSvc/CLASS_DEF.h"
#include "DataModel/DataVector.h"

namespace LVL1 {
  class JetElement;
}

/** JEP container for writing bytestream.
 *
 *  Contains all the component collections needed to build the ROD.
 *
 *  @author Peter Faulkner
 */

class JepContainer {

 public:
   JepContainer(const DataVector<LVL1::JetElement>* jeCollection);

   /// Return pointer to jet element collection
   const DataVector<LVL1::JetElement>* JetElements() const;

 private:

   /// Jet element collection
   const DataVector<LVL1::JetElement>* m_jeCollection;

};

inline const DataVector<LVL1::JetElement>* JepContainer::JetElements() const
{
  return m_jeCollection;
}

CLASS_DEF(JepContainer, 1321678566, 1)

#endif
