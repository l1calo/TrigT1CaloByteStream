# Test bytestream to RODHeader conversion
include ( "TrigT1CaloByteStream/ReadRodBS_jobOptions.py" )
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__RodTester
from AthenaCommon.AlgSequence import AlgSequence
job = AlgSequence()
job += LVL1BS__RodTester( "RodTester" )
