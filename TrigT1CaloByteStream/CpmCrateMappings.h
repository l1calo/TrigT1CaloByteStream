#ifndef TRIGT1CALOBYTESTREAM_CPMCRATEMAPPINGS_H
#define TRIGT1CALOBYTESTREAM_CPMCRATEMAPPINGS_H

namespace LVL1BS {

class ChannelCoordinate;

/** CPM crate/module/channel to eta/phi mappings
 *
 *  Source: "ATLAS Level-1 Calorimeter Trigger: Cluster Processor
 *           Module, Project Specification" version 2.03
 *
 *  @author Peter Faulkner
 */

class CpmCrateMappings {

 public:
   CpmCrateMappings();

   /// Return eta, phi mapping for given crate/module/channel
   bool mapping(int crate, int module, int channel,
                                       ChannelCoordinate& coord) const;
   /// Return the number of crates
   static int  crates();
   /// Return the number of modules per crate
   static int  modules();
   /// Return the number of CPM tower channels per module
   static int  channels();

 private:

   static const int s_crates   = 4;
   static const int s_modules  = 14;
   static const int s_channels = 80;

   static const int s_etaBinsPerRow = 4;

   static const double s_phiGran;
   static const double s_etaGran;

};

} // end namespace

#endif
