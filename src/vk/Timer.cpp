#include "vk/Timer.hpp"

namespace ve
{
    DeviceTimer::DeviceTimer(const VulkanMainContext& vmc, VulkanCommandContext& vcc) : vmc(vmc), result_fetched(TIMER_COUNT, false)
    {
        vk::QueryPoolCreateInfo qpci{};
        qpci.sType = vk::StructureType::eQueryPoolCreateInfo;
        qpci.queryType = vk::QueryType::eTimestamp;
        qpci.queryCount = TIMER_COUNT * 2;
        qp = vmc.logical_device.get().createQueryPool(qpci);
        vk::CommandBuffer& cb = vcc.get_one_time_graphics_buffer();
        reset_all(cb);
        vcc.submit_graphics(cb, true);
        vk::PhysicalDeviceProperties pdp = vmc.physical_device.get().getProperties();
        timestamp_period = pdp.limits.timestampPeriod;
    }

    void DeviceTimer::self_destruct()
    {
        vmc.logical_device.get().destroyQueryPool(qp);
    }

    void DeviceTimer::reset_all(vk::CommandBuffer& cb)
    {
        cb.resetQueryPool(qp, 0, TIMER_COUNT * 2);
    }

    void DeviceTimer::reset(vk::CommandBuffer& cb, const std::vector<TimerNames>& timers)
    {
        // bundle all contiguous timer resets into one resetQueryPool call
        std::vector<TimerNames> sorted_timers(timers);
        std::sort(sorted_timers.begin(), sorted_timers.end());
        std::pair<uint32_t, uint32_t> range(0, 0);
        for (const auto& t : timers)
        {
            if (range.first + range.second / 2 == t)
            {
                range.second += 2;
            }
            else
            {
                if (range.second > 0) cb.resetQueryPool(qp, range.first, range.second);
                range = std::make_pair(t * 2, 2);
            }
        }
        cb.resetQueryPool(qp, range.first, range.second);
    }

    void DeviceTimer::start(vk::CommandBuffer& cb, TimerNames t, vk::PipelineStageFlagBits stage)
    {
        cb.writeTimestamp(stage, qp, t * 2);
    }

    void DeviceTimer::stop(vk::CommandBuffer& cb, TimerNames t, vk::PipelineStageFlagBits stage)
    {
        cb.writeTimestamp(stage, qp, t * 2 + 1);
        result_fetched[t] = false;
    }
} // namespace ve
