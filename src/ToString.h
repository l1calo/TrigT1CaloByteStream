#ifndef TRIGT1CALOBYTESTREAM_TOSTRING_H
#define TRIGT1CALOBYTESTREAM_TOSTRING_H
#include <string>
#include "xAODTrigL1Calo/TriggerTower.h"
#include "xAODTrigL1Calo/TriggerTowerContainer.h"

namespace LVL1BS {
	std::string ToString(const xAOD::TriggerTower& tt);
	std::string ToString(const xAOD::TriggerTowerContainer& container);
}

#endif
