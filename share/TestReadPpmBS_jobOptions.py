# Test PPM bytestream to TriggerTower conversion
include ( "TrigT1CaloByteStream/ReadPpmBS_jobOptions.py" )
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__PpmTester
from AthenaCommon.AlgSequence import AlgSequence
job = AlgSequence()
job += LVL1BS__PpmTester( "PpmTester" )
