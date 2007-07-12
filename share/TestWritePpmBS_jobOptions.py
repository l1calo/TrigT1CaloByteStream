# Test TriggerTower to PPM bytestream conversion
include ( "TrigT1CaloByteStream/WritePpmBS_jobOptions.py" )
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__PpmTester
from AthenaCommon.AlgSequence import AlgSequence
job = AlgSequence()
job += LVL1BS__PpmTester( "PpmTester" )
