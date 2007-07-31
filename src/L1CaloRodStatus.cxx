
#include <utility>

#include "TrigT1CaloByteStream/L1CaloRodStatus.h"
#include "TrigT1CaloByteStream/L1CaloSrcIdMap.h"

namespace LVL1BS {

// Set the ROD status words for given ROD IDs

void L1CaloRodStatus::setStatus(RawEventWrite*  const re,
				const RODStatusMap& rodMap,
                                L1CaloSrcIdMap* const srcIdMap)
{
  if (rodMap.empty()) return;

  // We assume all ROD/ROBs are in one ROS

  RODStatusMap::const_iterator rodIter    = rodMap.begin();
  RODStatusMap::const_iterator rodIterEnd = rodMap.end();
  uint32_t rodId = rodIter->first;
  uint32_t robId = srcIdMap->getRobID(rodId);
  uint32_t rosId = srcIdMap->getRosID(robId);
  uint32_t detId = srcIdMap->getDetID(rosId);
  const DetFrag* det = re->first_child();
  while (det && (det->source_id() != detId)) det = det->next();
  if (!det) return;
  const RosFrag* ros = det->first_child();
  while (ros && (ros->source_id() != rosId)) ros = ros->next();
  if (!ros) return;

  // Found the ROS, match the ROBs

  std::map<uint32_t, RobFrag*> robMap;
  RobFrag* rob = const_cast<RobFrag*>(ros->first_child());
  while (rob) {
    robMap.insert(std::make_pair(rob->source_id(), rob));
    rob = const_cast<RobFrag*>(rob->next());
  }
  for (; rodIter != rodIterEnd; ++rodIter) {
    rodId = rodIter->first;
    robId = srcIdMap->getRobID(rodId);
    std::map<uint32_t, RobFrag*>::iterator robIter = robMap.find(robId);
    if (robIter != robMap.end()) {
      // Replace the status words.
      // Note - the words are not copied so the source must exist
      // until the event is written.
      rob = robIter->second;
      RODStatusWords* stat = rodIter->second;
      rob->rod_status(stat->size(), &(*stat)[0]);
    }
  }
}

} // end namespace
