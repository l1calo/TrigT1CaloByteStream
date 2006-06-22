
#include "TrigT1Calo/JetElement.h"
#include "TrigT1CaloByteStream/JepContainer.h"

JepContainer::JepContainer(const DataVector<LVL1::JetElement>* jeCollection)
                          : m_jeCollection(jeCollection)
{
}
