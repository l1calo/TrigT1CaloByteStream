#ifndef TRIGT1CALOBYTESTREAM_TRIGGERTOWERSELECTIONTOOL_H
#define TRIGT1CALOBYTESTREAM_TRIGGERTOWERSELECTIONTOOL_H

#include <stdint.h>
#include <vector>

#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/ToolHandle.h"

#include "TrigT1CaloByteStream/IPpmCrateMappingTool.h"
#include "TrigT1CaloByteStream/ITriggerTowerSelectionTool.h"

namespace LVL1BS {

class L1CaloSrcIdMap;

/** TriggerTower subset selection for bytestream.
 *
 *  @author Peter Faulkner
 */

class TriggerTowerSelectionTool : virtual public ITriggerTowerSelectionTool,
                                          public AlgTool {

 public:

   TriggerTowerSelectionTool(const std::string& type, const std::string& name,
                                                const IInterface* parent);
   virtual ~TriggerTowerSelectionTool();

   virtual StatusCode initialize();
   virtual StatusCode finalize();

   /// Return a list of TT channel IDs for given eta/phi range
   virtual void channelIDs(double etaMin, double etaMax,
                           double phiMin, double phiMax,
			   std::vector<unsigned int>& chanIds);
   /// Return a list of ROB IDs for given list of TT channel IDs
   virtual void robIDs(const std::vector<unsigned int>& chanIds,
                             std::vector<uint32_t>& robs);

 private:

   /// Tool for mappings
   ToolHandle<LVL1BS::IPpmCrateMappingTool> m_mappingTool;
   /// Source ID converter
   L1CaloSrcIdMap* m_srcIdMap;
   /// class member version of retrieving MsgStream
   mutable MsgStream m_log;
   bool m_debug;
   /// TT eta bins
   std::vector<double> m_etaBins;

};

} // end namespace

#endif
