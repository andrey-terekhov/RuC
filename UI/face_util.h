#pragma once

template <typename StateT>
class State_Holder
{
public:
    State_Holder(StateT &state) : state(state), old_state(state)
    {
    }
    ~State_Holder()
    {
        state = old_state;
    }

private:
    StateT &state;
    StateT  old_state;
};
