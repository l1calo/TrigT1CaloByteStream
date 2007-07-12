# Test CP RoI bytestream to CPMRoI conversion
include ( "TrigT1CaloByteStream/ReadCpmRoiBS_jobOptions.py" )
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpmTester
from AthenaCommon.AlgSequence import AlgSequence
job = AlgSequence()
job += LVL1BS__CpmTester( "CpmTester" )
