
#include "TrigT1Calo/JetElement.h"
#include "TrigT1Calo/JEMHits.h"
#include "TrigT1Calo/JEMEtSums.h"
#include "TrigT1CaloByteStream/JepContainer.h"

JepContainer::JepContainer(const DataVector<LVL1::JetElement>* jeCollection,
                           const DataVector<LVL1::JEMHits>*    hitCollection,
			   const DataVector<LVL1::JEMEtSums>*  etCollection)
                          : m_jeCollection(jeCollection),
			    m_hitCollection(hitCollection),
			    m_etCollection(etCollection)
{
}
