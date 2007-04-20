from AthenaCommon.Configurable import *

class CpmTester( ConfigurableAlgorithm ) :
  __slots__ = { 
    'OutputLevel' : 0, # int
    'Enable' : True, # bool
    'ErrorMax' : 1, # int
    'ErrorCount' : 0, # int
    'AuditAlgorithms' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditReinitialize' : False, # bool
    'AuditExecute' : False, # bool
    'AuditFinalize' : False, # bool
    'AuditBeginRun' : False, # bool
    'AuditEndRun' : False, # bool
    'CPMTowerLocation' : 'CPMTowers', # str
    'CPMHitsLocation' : 'CPMHits', # str
    'CMMCPHitsLocation' : 'CMMCPHits', # str
    'CPMRoILocation' : 'CPMRoIs', # str
    'CPMTowerPrint' : 1, # int
    'CPMHitsPrint' : 1, # int
    'CMMCPHitsPrint' : 1, # int
    'CPMRoIPrint' : 1, # int
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(CpmTester, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'TrigT1CaloByteStream'
  def getType( self ):
      return 'CpmTester'
  pass # class CpmTester

class JemTester( ConfigurableAlgorithm ) :
  __slots__ = { 
    'OutputLevel' : 0, # int
    'Enable' : True, # bool
    'ErrorMax' : 1, # int
    'ErrorCount' : 0, # int
    'AuditAlgorithms' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditReinitialize' : False, # bool
    'AuditExecute' : False, # bool
    'AuditFinalize' : False, # bool
    'AuditBeginRun' : False, # bool
    'AuditEndRun' : False, # bool
    'JetElementLocation' : 'LVL1JetElements', # str
    'JEMHitsLocation' : 'JEMHits', # str
    'JEMEtSumsLocation' : 'JEMEtSums', # str
    'CMMJetHitsLocation' : 'CMMJetHits', # str
    'CMMEtSumsLocation' : 'CMMEtSums', # str
    'JEMRoILocation' : 'JEMRoIs', # str
    'CMMRoILocation' : 'CMMRoIs', # str
    'JetElementPrint' : 1, # int
    'JEMHitsPrint' : 1, # int
    'JEMEtSumsPrint' : 1, # int
    'CMMJetHitsPrint' : 1, # int
    'CMMEtSumsPrint' : 1, # int
    'JEMRoIPrint' : 1, # int
    'CMMRoIPrint' : 1, # int
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(JemTester, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'TrigT1CaloByteStream'
  def getType( self ):
      return 'JemTester'
  pass # class JemTester

class PpmTester( ConfigurableAlgorithm ) :
  __slots__ = { 
    'OutputLevel' : 0, # int
    'Enable' : True, # bool
    'ErrorMax' : 1, # int
    'ErrorCount' : 0, # int
    'AuditAlgorithms' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditReinitialize' : False, # bool
    'AuditExecute' : False, # bool
    'AuditFinalize' : False, # bool
    'AuditBeginRun' : False, # bool
    'AuditEndRun' : False, # bool
    'TriggerTowerLocation' : 'LVL1TriggerTowers', # str
    'ForceSlicesFADC' : 0, # int
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(PpmTester, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'TrigT1CaloByteStream'
  def getType( self ):
      return 'PpmTester'
  pass # class PpmTester

class CpByteStreamTool( ConfigurableAlgTool ) :
  __slots__ = { 
    'OutputLevel' : 7, # int
    'AuditTools' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditFinalize' : False, # bool
    'DataVersion' : 1, # int
    'DataFormat' : 1, # int
    'SlinksPerCrate' : 2, # int
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(CpByteStreamTool, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'TrigT1CaloByteStream'
  def getType( self ):
      return 'CpByteStreamTool'
  pass # class CpByteStreamTool

class CpmRoiByteStreamTool( ConfigurableAlgTool ) :
  __slots__ = { 
    'OutputLevel' : 7, # int
    'AuditTools' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditFinalize' : False, # bool
    'DataVersion' : 1, # int
    'DataFormat' : 1, # int
    'SlinksPerCrate' : 1, # int
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(CpmRoiByteStreamTool, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'TrigT1CaloByteStream'
  def getType( self ):
      return 'CpmRoiByteStreamTool'
  pass # class CpmRoiByteStreamTool

class JepByteStreamTool( ConfigurableAlgTool ) :
  __slots__ = { 
    'OutputLevel' : 7, # int
    'AuditTools' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditFinalize' : False, # bool
    'DataVersion' : 1, # int
    'DataFormat' : 1, # int
    'SlinksPerCrate' : 4, # int
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(JepByteStreamTool, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'TrigT1CaloByteStream'
  def getType( self ):
      return 'JepByteStreamTool'
  pass # class JepByteStreamTool

class JepRoiByteStreamTool( ConfigurableAlgTool ) :
  __slots__ = { 
    'OutputLevel' : 7, # int
    'AuditTools' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditFinalize' : False, # bool
    'DataVersion' : 1, # int
    'DataFormat' : 1, # int
    'SlinksPerCrate' : 1, # int
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(JepRoiByteStreamTool, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'TrigT1CaloByteStream'
  def getType( self ):
      return 'JepRoiByteStreamTool'
  pass # class JepRoiByteStreamTool

class PpmByteStreamTool( ConfigurableAlgTool ) :
  __slots__ = { 
    'OutputLevel' : 7, # int
    'AuditTools' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditFinalize' : False, # bool
    'PrintCompStats' : 0, # int
    'PedestalValue' : 10, # int
    'ZeroSuppress' : 0, # int
    'DataVersion' : 1, # int
    'DataFormat' : 1, # int
    'CompressionVersion' : 1, # int
    'SlinksPerCrate' : 4, # int
    'DefaultSlicesLUT' : 1, # int
    'DefaultSlicesFADC' : 7, # int
    'ForceSlicesFADC' : 0, # int
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(PpmByteStreamTool, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'TrigT1CaloByteStream'
  def getType( self ):
      return 'PpmByteStreamTool'
  pass # class PpmByteStreamTool
