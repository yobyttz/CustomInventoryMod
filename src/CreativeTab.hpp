#pragma once

#include "EngineApi.hpp"
#include <mutex>

namespace inventorytab {

class CreativeTab {
public:
    explicit CreativeTab(EngineApi& api) : mApi(api) {}

    void onCreativeInitializer(void* initializer, void const* worldVersion);
    void reset();

private:
    EngineApi& mApi;
    std::mutex mMutex;
    void* mCreativeRegistry{nullptr};
    void* mInjectedRegistry{nullptr};
    bool mInjecting{false};

    void tryInject();
};

} // namespace inventorytab
