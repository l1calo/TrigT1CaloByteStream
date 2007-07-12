# TriggerTower to PPM bytestream conversion
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__PpmByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__PpmByteStreamTool("PpmByteStreamTool")
StreamBS = AthenaOutputStream( "StreamBS" )
StreamBS.ItemList  += [ "6207#*" ]
