# JEP RoI container to bytestream conversion
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepRoiByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__JepRoiByteStreamTool("JepRoiByteStreamTool")
StreamBS = AthenaOutputStream( "StreamBS" )
StreamBS.ItemList  += [ "1266611723#*" ]
