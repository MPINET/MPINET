#ifndef SDN_CONTROLLER_H
#define SDN_CONTROLLER_H

#include "openflow-interface.h"



class SDNController : public LearningController
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();

    ~SDNController() override
    {
        m_learnState.clear();
    }

    void ReceiveFromSwitch(Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer) override;

  protected:
    /// Learned state
    struct LearnedState
    {
        uint32_t port; ///< Learned port.
    };

    Time m_expirationTime; ///< Time it takes for learned MAC state entry/created flow to expire.
    /// Learned state type
    typedef std::map<Mac48Address, LearnedState> LearnState_t;
    LearnState_t m_learnState; ///< Learned state data.
};

#endif // SDN_CONTROLLER_H