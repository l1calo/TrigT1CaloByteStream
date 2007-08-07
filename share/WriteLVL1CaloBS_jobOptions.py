# TrigT1Calo object containers to bytestream conversion
from AthenaCommon.AppMgr import ServiceMgr
from ByteStreamCnvSvc.ByteStreamCnvSvcConf import ByteStreamCnvSvc
ServiceMgr += ByteStreamCnvSvc()
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__PpmByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpmRoiByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepRoiByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__PpmByteStreamTool("PpmByteStreamTool")
ByteStreamCnvSvc += LVL1BS__CpByteStreamTool("CpByteStreamTool")
ByteStreamCnvSvc += LVL1BS__CpmRoiByteStreamTool("CpmRoiByteStreamTool")
ByteStreamCnvSvc += LVL1BS__JepByteStreamTool("JepByteStreamTool")
ByteStreamCnvSvc += LVL1BS__JepRoiByteStreamTool("JepRoiByteStreamTool")
StreamBS = AthenaOutputStream( "StreamBS" )
StreamBS.ItemList += [ "6207#*" ]
StreamBS.ItemList += [ "1272124447#*" ]
StreamBS.ItemList += [ "216508938#*" ]
StreamBS.ItemList += [ "1255323120#*" ]
StreamBS.ItemList += [ "1266611723#*" ]
