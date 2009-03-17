# TrigT1Calo object containers to bytestream conversion
include.block("TrigT1CaloByteStream/WriteLVL1CaloBS_jobOptions.py")

# the following include is needed to load correctly the trigger towers on/off, TT/Cells maps
include( "CaloConditions/CaloConditions_jobOptions.py" )
# To setup correctly the LArCablingService when doLAr is off in the top option.
if not rec.doLArg():
    include( "LArConditionsCommon/LArIdMap_comm_jobOptions.py" )
    include( "LArIdCnv/LArIdCnv_joboptions.py" )

from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__PpmByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpmRoiByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepRoiByteStreamTool
ToolSvc = Service("ToolSvc")
ToolSvc += LVL1BS__PpmByteStreamTool("PpmByteStreamTool",
           PpmMappingTool="LVL1BS::PpmCoolOrBuiltinMappingTool/PpmCoolOrBuiltinMappingTool")
ToolSvc += LVL1BS__CpByteStreamTool("CpByteStreamTool")
ToolSvc += LVL1BS__CpmRoiByteStreamTool("CpmRoiByteStreamTool")
ToolSvc += LVL1BS__JepByteStreamTool("JepByteStreamTool")
ToolSvc += LVL1BS__JepRoiByteStreamTool("JepRoiByteStreamTool")

StreamBS = AthenaOutputStream( "StreamBS" )
StreamBS.ItemList += [ "6207#*" ]
StreamBS.ItemList += [ "1272124447#*" ]
StreamBS.ItemList += [ "216508938#*" ]
StreamBS.ItemList += [ "1255323120#*" ]
StreamBS.ItemList += [ "1266611723#*" ]
