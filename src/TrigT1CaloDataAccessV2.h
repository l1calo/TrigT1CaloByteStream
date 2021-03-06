#ifndef TRIGT1CALOBYTESTREAM_TRIGT1CALODATAACCESSV2_H
#define TRIGT1CALOBYTESTREAM_TRIGT1CALODATAACCESSV2_H

#include <string>
#include <vector>

#include "TrigT1CaloByteStream/ITrigT1CaloDataAccessV2.h"
#include "AsgTools/AsgTool.h"


namespace LVL1BS {

class L1CaloByteStreamReadTool;

/** Tool to retrieve all TriggerTowers
 *  from bytestream.
 *
 *  @author Alexander Mazurov <alexander.mazurov@cern.ch>
 */

class TrigT1CaloDataAccessV2 : public asg::AsgTool,
	virtual public ITrigT1CaloDataAccessV2
{
	ASG_TOOL_CLASS(TrigT1CaloDataAccessV2, ITrigT1CaloDataAccessV2)
 public:
	TrigT1CaloDataAccessV2(const std::string& name);

   virtual StatusCode initialize();

   virtual StatusCode loadTriggerTowers(xAOD::TriggerTowerContainer& container);
   virtual StatusCode PrintTriggerTowers();

 private:
   std::string m_message;
   /// Tool that does the actual work
   ToolHandle<LVL1BS::L1CaloByteStreamReadTool> m_tool;
};

} // end namespace

#endif
