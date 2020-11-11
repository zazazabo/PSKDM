#include "../mem_ctx/mem_ctx.hpp"
#include "../pe_image/pe_image.h"

namespace nasa
{
	class mapper_ctx
	{
	public:
		explicit mapper_ctx(nasa::mem_ctx& map_into, nasa::mem_ctx& map_from);
		auto map(std::vector<std::uint8_t>& raw_image)->std::pair<void*, void*>;
		void call_entry(void* drv_entry, void** hook_handler) const;

	private:
		std::uint16_t pml4_idx;
		auto allocate_driver(std::vector<std::uint8_t>& raw_image)->std::pair<void*, void*>;
		void make_kernel_access(void* drv_base);
		nasa::mem_ctx map_into, map_from;
	};
}