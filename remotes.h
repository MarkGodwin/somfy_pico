
#pragma once

#include "remote.h"
#include <list>
#include <memory>

class SomfyRemotes
{
    public:
        std::shared_ptr<SomfyRemote> GetRemote(uint32_t remoteId);
        std::shared_ptr<SomfyRemote> CreateRemote();

    private:
        std::map<uint32_t, std::shared_ptr<SomfyRemote>> _remotes;
};
