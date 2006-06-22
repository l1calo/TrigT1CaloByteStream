#ifndef TRIGT1CALOBYTESTREAM_JEMCRATEMAPPINGS_H
#define TRIGT1CALOBYTESTREAM_JEMCRATEMAPPINGS_H

#include <vector>

class ChannelCoordinate;

/** JEM crate/module/channel to eta/phi mappings
 *
 *  Source: "ATLAS Level-1 Calorimeter Trigger: Jet/Energy Processor
 *           Module, Project Specification" version 1.0
 *  The remappings at extreme eta are from a private communication
 *  from Steve Hillier.
 *
 *  @author Peter Faulkner
 */

class JemCrateMappings {

 public:
   JemCrateMappings();

   /// Return eta, phi mapping for given crate/module/channel
   bool mapping(int crate, int module, int channel,
                                       ChannelCoordinate& coord) const;
   /// Return the number of crates
   static int  crates();
   /// Return the number of modules per crate
   static int  modules();
   /// Return the number of jet element channels per module
   static int  channels();
   /// Return true if module corresponds to extreme eta
   static bool forward(int module);

 private:

   static const int s_crates   = 2;
   static const int s_modules  = 16;
   static const int s_channels = 44;

   static const int s_modulesPerQuadrant = 8;
   static const int s_extremeNegModule   = 0;
   static const int s_extremePosModule   = 7;
   static const int s_etaBinsPerRow      = 4;

   static const double s_phiGran;
   static const double s_etaGran;

   //  Extreme eta data vectors
   std::vector<double> m_etasNegative;
   std::vector<double> m_granNegative;
   std::vector<double> m_etasPositive;
   std::vector<double> m_granPositive;

   //  Extreme eta remapping vectors
   std::vector<int>    m_negChans;
   std::vector<int>    m_posChans;

};

#endif
