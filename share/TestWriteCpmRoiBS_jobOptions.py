# Test CPMRoI Container to bytestream conversion
include ( "TrigT1CaloByteStream/WriteCpmRoiBS_jobOptions.py" )
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpmTester
from AthenaCommon.AlgSequence import AlgSequence
job = AlgSequence()
job += LVL1BS__CpmTester( "CpmTester" )
