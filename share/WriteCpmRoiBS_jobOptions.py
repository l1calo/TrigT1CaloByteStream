# CPMRoI container to bytestream conversion
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpmRoiByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__CpmRoiByteStreamTool("CpmRoiByteStreamTool")
StreamBS = AthenaOutputStream( "StreamBS" )
StreamBS.ItemList  += [ "216508938#*" ]
