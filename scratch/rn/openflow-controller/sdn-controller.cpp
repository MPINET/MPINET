#include "sdn-controller.h"

TypeId
SDNController::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ofi::LearningController")
            .SetParent<Controller>()
            .SetGroupName("Openflow")
            .AddConstructor<LearningController>()
            .AddAttribute("ExpirationTime",
                          "Time it takes for learned MAC state entry/created flow to expire.",
                          TimeValue(Seconds(0)),
                          MakeTimeAccessor(&LearningController::m_expirationTime),
                          MakeTimeChecker());
    return tid;
}

void
SDNController::ReceiveFromSwitch(Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer)
{
    if (m_switches.find(swtch) == m_switches.end())
    {
        NS_LOG_ERROR("Can't receive from this switch, not registered to the Controller.");
        return;
        
    }

    // We have received any packet at this point, so we pull the header to figure out what type of
    // packet we're handling.
    uint8_t type = GetPacketType(buffer);

    if (type == OFPT_PACKET_IN) // The switch didn't understand the packet it received, so it
                                // forwarded it to the controller.
    {
        ofp_packet_in* opi = (ofp_packet_in*)ofpbuf_try_pull(buffer, offsetof(ofp_packet_in, data));
        int port = ntohs(opi->in_port);

        // Create matching key.
        sw_flow_key key;
        key.wildcards = 0;
        flow_extract(buffer, port != -1 ? port : OFPP_NONE, &key.flow);

        uint16_t out_port = OFPP_FLOOD;
        uint16_t in_port = ntohs(key.flow.in_port);

        // If the destination address is learned to a specific port, find it.
        Mac48Address dst_addr;
        dst_addr.CopyFrom(key.flow.dl_dst);
        if (!dst_addr.IsBroadcast())
        {
            LearnState_t::iterator st = m_learnState.find(dst_addr);
            if (st != m_learnState.end())
            {
                out_port = st->second.port;
            }
            else
            {
                NS_LOG_INFO("Setting to flood; don't know yet what port " << dst_addr
                                                                          << " is connected to");
            }
        }
        else
        {
            NS_LOG_INFO("Setting to flood; this packet is a broadcast");
        }

        // Create output-to-port action
        ofp_action_output x[1];
        x[0].type = htons(OFPAT_OUTPUT);
        x[0].len = htons(sizeof(ofp_action_output));
        x[0].port = out_port;

        // Create a new flow that outputs matched packets to a learned port, OFPP_FLOOD if there's
        // no learned port.
        ofp_flow_mod* ofm = BuildFlow(key,
                                      opi->buffer_id,
                                      OFPFC_ADD,
                                      x,
                                      sizeof(x),
                                      OFP_FLOW_PERMANENT,
                                      m_expirationTime.IsZero() ? OFP_FLOW_PERMANENT
                                                                : m_expirationTime.GetSeconds());
        SendToSwitch(swtch, ofm, ofm->header.length);

        // We can learn a specific port for the source address for future use.
        Mac48Address src_addr;
        src_addr.CopyFrom(key.flow.dl_src);
        LearnState_t::iterator st = m_learnState.find(src_addr);
        if (st == m_learnState.end()) // We haven't learned our source MAC yet.
        {
            LearnedState ls;
            ls.port = in_port;
            m_learnState.insert(std::make_pair(src_addr, ls));
            NS_LOG_INFO("Learned that " << src_addr << " can be found over port " << in_port);

            // Learn src_addr goes to a certain port.
            ofp_action_output x2[1];
            x2[0].type = htons(OFPAT_OUTPUT);
            x2[0].len = htons(sizeof(ofp_action_output));
            x2[0].port = in_port;

            // Switch MAC Addresses and ports to the flow we're modifying
            src_addr.CopyTo(key.flow.dl_dst);
            dst_addr.CopyTo(key.flow.dl_src);
            key.flow.in_port = out_port;
            ofp_flow_mod* ofm2 = BuildFlow(
                key,
                -1,
                OFPFC_MODIFY,
                x2,
                sizeof(x2),
                OFP_FLOW_PERMANENT,
                m_expirationTime.IsZero() ? OFP_FLOW_PERMANENT : m_expirationTime.GetSeconds());
            SendToSwitch(swtch, ofm2, ofm2->header.length);
        }
    }
}