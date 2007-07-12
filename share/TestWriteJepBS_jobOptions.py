# Test JEP Container to bytestream conversion
include ( "TrigT1CaloByteStream/WriteJepBS_jobOptions.py" )
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JemTester
from AthenaCommon.AlgSequence import AlgSequence
job = AlgSequence()
job += LVL1BS__JemTester( "JemTester" )
