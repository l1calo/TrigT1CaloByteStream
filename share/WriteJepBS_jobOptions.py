# JEP container to bytestream conversion
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__JepByteStreamTool("JepByteStreamTool")
StreamBS = AthenaOutputStream( "StreamBS" )
StreamBS.ItemList  += [ "1255323120#*" ]
