/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "virtual-layer-plus-helper.h"
#include "ns3/log.h"
#include "ns3/virtual-layer-plus-net-device.h"
#include "ns3/node.h"
#include "ns3/names.h"

NS_LOG_COMPONENT_DEFINE ("VirtualLayerPlusHelper");

namespace ns3 {

VirtualLayerPlusHelper::VirtualLayerPlusHelper ()
{
  NS_LOG_FUNCTION (this);
  m_deviceFactory.SetTypeId ("ns3::VirtualLayerPlusNetDevice");
}

void VirtualLayerPlusHelper::SetDeviceAttribute (std::string n1,
                                          const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this);
  m_deviceFactory.Set (n1, v1);
}

NetDeviceContainer VirtualLayerPlusHelper::Install (const NetDeviceContainer c)
{
  NS_LOG_FUNCTION (this);

  NetDeviceContainer devs;

  for (uint32_t i = 0; i < c.GetN (); ++i)
    {
      Ptr<NetDevice> device = c.Get (i);
      NS_ASSERT_MSG (device != 0, "No NetDevice found in the node " << int(i) );

      Ptr<Node> node = device->GetNode ();
      NS_LOG_LOGIC ("**** Install VirtualLayer on node " << node->GetId ());

      Ptr<VirtualLayerPlusNetDevice> dev = m_deviceFactory.Create<VirtualLayerPlusNetDevice> ();
      devs.Add (dev);
      node->AddDevice (dev);
      dev->SetNetDevice (device);
    }
  return devs;
}

int64_t VirtualLayerPlusHelper::AssignStreams (NetDeviceContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<NetDevice> netDevice;
  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      netDevice = (*i);
      Ptr<VirtualLayerPlusNetDevice> dev = DynamicCast<VirtualLayerPlusNetDevice> (netDevice);
      if (dev)
        {
          currentStream += dev->AssignStreams (currentStream);
        }
    }
  return (currentStream - stream);
}

} // namespace ns3
