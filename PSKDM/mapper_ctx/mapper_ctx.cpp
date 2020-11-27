#include "mapper_ctx.hpp"

namespace nasa
{
	mapper_ctx::mapper_ctx
	(
		nasa::mem_ctx* map_into,
		nasa::mem_ctx* map_from
	)
		:
		map_into(map_into),
		map_from(map_from),
		pml4_idx(0)
	{
		const auto map_into_pml4 = 
			reinterpret_cast<ppml4e>(
				map_into->set_page(map_into->dirbase));

		// look for an empty pml4e...
		for (auto idx = 100u; idx < 255; ++idx)
		{
			if (!map_into_pml4[idx].value)
			{
				this->pml4_idx = idx;
				break;
			}
		}
	}

	auto mapper_ctx::map(std::vector<std::uint8_t>& raw_image) -> std::pair<void*, void*>
	{
		const auto [drv_alloc, drv_entry_addr] = allocate_driver(raw_image);
		auto [drv_ppml4e, drv_pml4e] = map_from->get_pml4e(drv_alloc);

		make_kernel_access(drv_alloc);
		while (!map_from->set_pml4e(drv_ppml4e, pml4e{ NULL })) 
			continue;

		drv_pml4e.nx = false;
		drv_pml4e.user_supervisor = false;
		drv_pml4e.write = true;

		// ensure we insert the pml4e...
		while (!map_into->write_phys(
			reinterpret_cast<ppml4e>(
				map_into->dirbase) + this->pml4_idx, drv_pml4e))
			continue;

		virt_addr_t new_addr = { reinterpret_cast<void*>(drv_alloc) };
		new_addr.pml4_index = this->pml4_idx;
		return { new_addr.value, drv_entry_addr };
	}

	void mapper_ctx::call_entry(void* drv_entry, void** hook_handler) const
	{
		map_into->v_ctx->syscall<NTSTATUS(__fastcall*)(void**)>(drv_entry, hook_handler);
	}

	auto mapper_ctx::allocate_driver(std::vector<std::uint8_t>& raw_image) -> std::pair<void*, void*>
	{
		nasa::pe_image drv_image(raw_image);
		const auto process_handle =
			OpenProcess(
				PROCESS_ALL_ACCESS,
				FALSE,
				map_from->pid
			);

		if (!process_handle)
			return { {}, {} };

		drv_image.fix_imports(
			[&](const char* module_name, const char* export_name)
		{
			return reinterpret_cast<std::uintptr_t>(
				util::get_kmodule_export(
					module_name,
					export_name
				));
		});

		drv_image.map();
		const auto drv_alloc_base = 
			reinterpret_cast<std::uintptr_t>(
				VirtualAllocEx(
					process_handle,
					nullptr,
					drv_image.size(),
					MEM_COMMIT | MEM_RESERVE,
					PAGE_READWRITE
				));

		if (!drv_alloc_base)
			return { {}, {} };

		virt_addr_t new_addr = { reinterpret_cast<void*>(drv_alloc_base) };
		new_addr.pml4_index = this->pml4_idx;
		drv_image.relocate(reinterpret_cast<std::uintptr_t>(new_addr.value));

		// dont write nt headers...
		SIZE_T bytes_written = 0;
		const bool result = WriteProcessMemory
		(
			process_handle,
			reinterpret_cast<void*>((std::uint64_t)drv_alloc_base + drv_image.header_size()),
			reinterpret_cast<void*>((std::uint64_t)drv_image.data() + drv_image.header_size()),
			drv_image.size() - drv_image.header_size(),
			&bytes_written
		);

		if (!CloseHandle(process_handle))
			return { {}, {} };

		return
		{ 
			reinterpret_cast<void*>(drv_alloc_base),
				reinterpret_cast<void*>(drv_image.entry_point() + 
					reinterpret_cast<std::uintptr_t>(new_addr.value)) 
		};
	}

	void mapper_ctx::make_kernel_access(void* drv_base)
	{
		const auto [ppdpte, pdpte] = 
			map_from->get_pdpte(drv_base);

		auto ppdpte_phys = 
			reinterpret_cast<void*>((
				reinterpret_cast<std::uint64_t>(ppdpte) >> 12) << 12); // 0 the last 12 bits...

		auto pdpt_mapping = 
			reinterpret_cast<::ppdpte>(
				map_from->set_page(ppdpte_phys));

		// set pdptes to CPL0 access only and executable...
		for (auto pdpt_idx = 0u; pdpt_idx < 512; ++pdpt_idx)
		{
			if (pdpt_mapping[pdpt_idx].present)
			{
				pdpt_mapping[pdpt_idx].user_supervisor = false;
				pdpt_mapping[pdpt_idx].nx = false;
				pdpt_mapping[pdpt_idx].write = true;

				auto pd_mapping = reinterpret_cast<ppde>(
					map_from->set_page(reinterpret_cast<void*>(
						pdpt_mapping[pdpt_idx].pfn << 12)));

				// set pdes to CPL0 access only and executable...
				for (auto pd_idx = 0u; pd_idx < 512; ++pd_idx)
				{
					if (pd_mapping[pd_idx].present)
					{
						pd_mapping[pd_idx].user_supervisor = false;
						pd_mapping[pd_idx].nx = false;
						pd_mapping[pd_idx].write = true;

						auto pt_mapping = reinterpret_cast<ppte>(
							map_from->set_page(reinterpret_cast<void*>(
								pd_mapping[pd_idx].pfn << 12)));

						// set ptes to CPL0 access only and executable...
						for (auto pt_idx = 0u; pt_idx < 512; ++pt_idx)
						{
							if (pt_mapping[pt_idx].present)
							{
								pt_mapping[pt_idx].user_supervisor = false;
								pt_mapping[pt_idx].nx = false;
								pt_mapping[pt_idx].write = true;
							}
						}

						// set page back to pd...
						pd_mapping = reinterpret_cast<ppde>(
							map_from->set_page(reinterpret_cast<void*>(
								pdpt_mapping[pdpt_idx].pfn << 12)));
					}
				}

				// set page back to pdpt...
				pdpt_mapping = reinterpret_cast<::ppdpte>(
					map_from->set_page(ppdpte_phys));
			}
		}
	}
}