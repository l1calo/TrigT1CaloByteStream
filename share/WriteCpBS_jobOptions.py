# CP container to bytestream conversion
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__CpByteStreamTool("CpByteStreamTool")
StreamBS = AthenaOutputStream( "StreamBS" )
StreamBS.ItemList  += [ "1272124447#*" ]
