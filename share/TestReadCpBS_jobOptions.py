# Test CP bytestream to CPMTower/... conversion
include ( "TrigT1CaloByteStream/ReadCpBS_jobOptions.py" )
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpmTester
from AthenaCommon.AlgSequence import AlgSequence
job = AlgSequence()
job += LVL1BS__CpmTester( "CpmTester" )
