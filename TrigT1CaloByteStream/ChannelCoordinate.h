#ifndef TRIGT1CALOBYTESTREAM_CHANNELCOORDINATE_H
#define TRIGT1CALOBYTESTREAM_CHANNELCOORDINATE_H


/** Holds eta/phi coordinates corresponding to a PPM channel.
 *  
 *  Also includes layer and eta/phi granularities.
 *
 *  @author Peter Faulkner
 */

class ChannelCoordinate {

 public:

   enum CaloLayer { NONE, EM, HAD, FCAL2, FCAL3 };

   ChannelCoordinate();
   ChannelCoordinate(CaloLayer layer, double eta, double phi,
                                      double etaGran, double phiGran);

   // Getters
   CaloLayer layer() const;
   double    eta()   const;
   double    phi()   const;
   double    etaGranularity() const;
   double    phiGranularity() const;

   // Setters
   void setLayer(CaloLayer layer);
   void setEta(double eta);
   void setPhi(double phi);
   void setEtaGranularity(double etaGran);
   void setPhiGranularity(double phiGran);

 private:

   CaloLayer m_layer;    ///< calo layer
   double    m_eta;      ///< central eta
   double    m_phi;      ///< central phi
   double    m_etaGran;  ///< eta granularity
   double    m_phiGran;  ///< phi granularity

};

inline ChannelCoordinate::CaloLayer ChannelCoordinate::layer() const
{
  return m_layer;
}

inline double ChannelCoordinate::eta() const
{
  return m_eta;
}

inline double ChannelCoordinate::phi() const
{
  return m_phi;
}

inline double ChannelCoordinate::etaGranularity() const
{
  return m_etaGran;
}

inline double ChannelCoordinate::phiGranularity() const
{
  return m_phiGran;
}

inline void ChannelCoordinate::setLayer(ChannelCoordinate::CaloLayer layer)
{
  m_layer = layer;
}

inline void ChannelCoordinate::setEta(double eta)
{
  m_eta = eta;
}

inline void ChannelCoordinate::setPhi(double phi)
{
  m_phi = phi;
}

inline void ChannelCoordinate::setEtaGranularity(double etaGran)
{
  m_etaGran = etaGran;
}

inline void ChannelCoordinate::setPhiGranularity(double phiGran)
{
  m_phiGran = phiGran;
}

#endif
