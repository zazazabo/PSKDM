#include "map_driver.hpp"
#include "mapper_ctx/mapper_ctx.hpp"
#include "vdm_ctx/vdm_ctx.h"
#include "vdm/vdm.hpp"

namespace mapper
{
	auto map_driver(std::uint8_t* drv_image, std::size_t image_size, void** entry_data) -> std::pair<mapper_error, void*>
	{
		std::vector<std::uint8_t> drv_buffer(drv_image, image_size + drv_image);
		if (!drv_buffer.size())
			return { mapper_error::image_invalid, nullptr };

		const auto [drv_handle, drv_key] = vdm::load_drv();
		if (drv_handle == INVALID_HANDLE_VALUE || drv_key.empty())
			return { mapper_error::load_error, nullptr };

		const auto runtime_broker_pid = 
			util::start_runtime_broker();

		if (!runtime_broker_pid)
			return { mapper_error::failed_to_create_proc, nullptr };	

		vdm::vdm_ctx v_ctx;
		nasa::mem_ctx my_proc(v_ctx, GetCurrentProcessId());
		nasa::mem_ctx runtime_broker(v_ctx, runtime_broker_pid);
		nasa::mapper_ctx mapper(my_proc, runtime_broker);

		const auto [drv_base, drv_entry] = mapper.map(drv_buffer);

		if (!drv_base || !drv_entry)
			return { mapper_error::init_failed, nullptr };

		mapper.call_entry(drv_entry, entry_data);

		if (!vdm::unload_drv(drv_handle, drv_key))
			return { mapper_error::unload_error, nullptr };

		return { mapper_error::error_success, drv_base };
	}
}