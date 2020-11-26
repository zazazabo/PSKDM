#include "map_driver.hpp"
#include "mapper_ctx/mapper_ctx.hpp"
#include "vdm_ctx/vdm_ctx.hpp"
#include "vdm/vdm.hpp"
#include "set_mgr/set_mgr.hpp"

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

		vdm::read_phys_t _read_phys = 
			[&](void* addr, void* buffer, std::size_t size) -> bool
		{
			return vdm::read_phys(addr, buffer, size);
		};

		vdm::write_phys_t _write_phys =
			[&](void* addr, void* buffer, std::size_t size) -> bool
		{
			return vdm::write_phys(addr, buffer, size);
		};

		vdm::vdm_ctx v_ctx(_read_phys, _write_phys);
		nasa::mem_ctx my_proc(&v_ctx, GetCurrentProcessId());
		nasa::mem_ctx runtime_broker(&v_ctx, runtime_broker_pid);
		nasa::mapper_ctx mapper(&my_proc, &runtime_broker);

		const auto result = 
			set_mgr::stop_setmgr(v_ctx, 
				set_mgr::get_setmgr_pethread(v_ctx));

		if (result != STATUS_SUCCESS)
			return { mapper_error::set_mgr_failure, nullptr };

		const auto [drv_base, drv_entry] = mapper.map(drv_buffer);
		if (!drv_base || !drv_entry)
			return { mapper_error::init_failed, nullptr };

		mapper.call_entry(drv_entry, entry_data);
		if (!vdm::unload_drv(drv_handle, drv_key))
			return { mapper_error::unload_error, nullptr };

		return { mapper_error::error_success, drv_base };
	}
}