
#pragma once

#include "remote.h"
#include <list>
#include <memory>

class SomfyRemotes
{
    public:
        std::shared_ptr<SomfyRemote> GetRemote(int32_t remoteId);
        std::shared_ptr<SomfyRemote> CreateRemote();

    private:

};
