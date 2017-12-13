/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef VIRTUAL_LAYER_PLUS_HELPER_H
#define VIRTUAL_LAYER_PLUS_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"
#include <string>

namespace ns3 {

class Node;
class AttributeValue;

/**
 * \ingroup virtual-layer
 *
 * \brief Setup a virtual-layer stack to be used as a shim between IPv4 and a generic NetDevice.
 */
class VirtualLayerPlusHelper
{
public:
  /*
   * Construct a VirtualLayerPlusHelper
   */
  VirtualLayerPlusHelper ();
  /**
   * Set an attribute on each ns3::VirtualLayerPlusDevice created by
   * VirtualLayerPlusHelper::Install.
   *
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   */
  void SetDeviceAttribute (std::string n1,
                           const AttributeValue &v1);

  /**
   * \brief Install the VirtualLayer stack on top of an existing NetDevice.
   *
   * This function requires a set of properly configured NetDevices
   * passed in as the parameter "c". The new NetDevices will have to
   * be used instead of the original ones. In this way these
   * VirtualLayer devices will behave as shims between the NetDevices
   * passed in and IPv4.
   *
   * \note IPv4 stack must be installed \a after VirtualLayer,
   * using the VirtualLayerPlus NetDevices. See the example in the
   * examples directory.
   *
   *
   * \param c the NetDevice container
   * \return a container with the newly created VirtualLayerPlusNetDevices
   */
  NetDeviceContainer Install (NetDeviceContainer c);

  /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model. Return the number of streams (possibly zero) that
  * have been assigned. The Install() method should have previously been
  * called by the user.eam number to the random variables
  * used by this model. Return the number of streams (possibly zero) that
  * have been assigned. The Install() method should have previously been
  * called by the user.
  *
  * \param c NetDeviceContainer of the set of net devices for which the
  *          VirtualLayerPlusNetDevice should be modified to use a fixed stream
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this helper
  */
  int64_t AssignStreams (NetDeviceContainer c, int64_t stream);

private:
  ObjectFactory m_deviceFactory; //!< Object factory
};

} // namespace ns3


#endif /* VIRTAL_LAYER_PLUS_HELPER_H */
