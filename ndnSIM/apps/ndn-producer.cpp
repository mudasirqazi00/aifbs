/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ndn-producer.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>

NS_LOG_COMPONENT_DEFINE("ndn.Producer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(Producer);

TypeId
Producer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::Producer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<Producer>()
      .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                    MakeNameAccessor(&Producer::m_prefix), MakeNameChecker())
      .AddAttribute(
         "Postfix",
         "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
         StringValue("/"), MakeNameAccessor(&Producer::m_postfix), MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                    MakeUintegerAccessor(&Producer::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), MakeTimeAccessor(&Producer::m_freshness),
                    MakeTimeChecker())
      .AddAttribute(
         "Signature",
         "Fake signature, 0 valid signature (default), other values application-specific",
         UintegerValue(0), MakeUintegerAccessor(&Producer::m_signature),
         MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&Producer::m_keyLocator), MakeNameChecker());
  return tid;
}

Producer::Producer()
{
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
Producer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
Producer::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void Producer::OnInterest(shared_ptr<const Interest> interest)
{
	LogManager::AddLogWithNodeId("ndn-producer.cpp->OnInterest.start");
//	LogManager::AddLogWithNodeId("ndn-producer.cpp->OnInterest.name", interest->getName().toUri());
	LogManager::AddLogWithNodeId("ndn-producer.cpp->OnInterest.accNames", NameHelper::join(interest->getAccNames()));

	std::vector<Name> v = interest->getAccNames();
	unsigned int size = v.size();
	for (unsigned int i = 0; i < size; i++) {

		Name n = v[i];
		auto tempInterest = std::make_shared<Interest>(interest->wireEncode());
		const_cast<Interest&>(*tempInterest).setName(n);

		//const_cast<Interest&>(*interest).setName(n);
		LogManager::AddLogWithNodeId("ndn-producer.cpp->OnInterest.name", const_cast<Interest&>(*tempInterest).getName().toUri());

		App::OnInterest(tempInterest); // tracing inside

		NS_LOG_FUNCTION(this << tempInterest);

		if (!m_active)
			return;

		Name dataName(const_cast<Interest&>(*tempInterest).getName());
		auto data = make_shared<Data>();
		data->setName(dataName);
		data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
		data->setContent(make_shared<::ndn::Buffer>(m_virtualPayloadSize));

		Signature signature;
		SignatureInfo signatureInfo(
				static_cast<::ndn::tlv::SignatureTypeValue>(255));

		if (m_keyLocator.size() > 0) {
			signatureInfo.setKeyLocator(m_keyLocator);
		}

		signature.setInfo(signatureInfo);
		signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

		data->setSignature(signature);

		NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());

		// to create real wire encoding
		data->wireEncode();

		m_transmittedDatas(data, this, m_face);
		m_appLink->onReceiveData(*data);
	}

	LogManager::AddLogWithNodeId("ndn-producer.cpp->OnInterest.completed");


	//Working code (before accumulation)
/*
	 LogManager::AddLogWithNodeId("ndn-producer.cpp->OnInterest.start");
	 LogManager::AddLogWithNodeId("ndn-producer.cpp->OnInterest.name", interest->getName().toUri());
	 LogManager::AddLogWithNodeId("ndn-producer.cpp->OnInterest.accNames", NameHelper::join(interest->getAccNames()));

	 App::OnInterest(interest); // tracing inside

	 NS_LOG_FUNCTION(this << interest);

	 if (!m_active)
	 return;

	 Name dataName(interest->getName());
	 // dataName.append(m_postfix);
	 // dataName.appendVersion();

	 auto data = make_shared<Data>();
	 data->setName(dataName);
	 data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

	 data->setContent(make_shared< ::ndn::Buffer>(m_virtualPayloadSize));

	 Signature signature;
	 SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

	 if (m_keyLocator.size() > 0) {
	 signatureInfo.setKeyLocator(m_keyLocator);
	 }

	 signature.setInfo(signatureInfo);
	 signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

	 data->setSignature(signature);

	 NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());

	 // to create real wire encoding
	 data->wireEncode();

	 m_transmittedDatas(data, this, m_face);
	 m_appLink->onReceiveData(*data);

	 LogManager::AddLogWithNodeId("ndn-producer.cpp->OnInterest.completed");
	 */
}

} // namespace ndn
} // namespace ns3
