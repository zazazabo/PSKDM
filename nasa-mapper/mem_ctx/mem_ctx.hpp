#pragma once
#include "../util/nt.hpp"
#include "../kernel_ctx/kernel_ctx.h"

#define PAGE_IN(addr, size) memset(addr, NULL, size)
struct pt_entries
{
	std::pair<ppml4e, pml4e>	pml4;
	std::pair<ppdpte, pdpte>	pdpt;
	std::pair<ppde, pde>		pd;
	std::pair<ppte, pte>		pt;
};

namespace physmeme
{
	class mem_ctx
	{
		friend class mapper_ctx;
	public:
		explicit mem_ctx(kernel_ctx& k_ctx, DWORD pid = GetCurrentProcessId());
		~mem_ctx();

		//
		// PTE manipulation
		//
		std::pair<ppte, pte> get_pte(void* addr, bool use_hyperspace = false);
		void set_pte(void* addr, const ::pte& pte, bool use_hyperspace = false);

		//
		// PDE manipulation
		//
		std::pair<ppde, pde> get_pde(void* addr, bool use_hyperspace = false);
		void set_pde(void* addr, const ::pde& pde, bool use_hyperspace = false);

		//
		// PDPTE manipulation
		//
		std::pair<ppdpte, pdpte> get_pdpte(void* addr, bool use_hyperspace = false);
		void set_pdpte(void* addr, const ::pdpte& pdpte, bool use_hyperspace = false);

		//
		// PML4E manipulation
		//
		std::pair<ppml4e, pml4e> get_pml4e(void* addr, bool use_hyperspace = false);
		void set_pml4e(void* addr, const ::pml4e& pml4e, bool use_hyperspace = false);

		//
		// gets dirbase (not the PTE or PFN but actual physical address)
		//
		void* get_dirbase() const;
		static void* get_dirbase(kernel_ctx& k_ctx, DWORD pid);

		void read_phys(void* buffer, void* addr, std::size_t size);
		void write_phys(void* buffer, void* addr, std::size_t size);

		template <class T>
		T read_phys(void* addr)
		{
			if (!addr) return {};
			T buffer;
			read_phys((void*)&buffer, addr, sizeof(T));
			return buffer;
		}

		template <class T>
		void write_phys(void* addr, const T& data)
		{
			if (!addr) return;
			write_phys((void*)&data, addr, sizeof(T));
		}

		std::pair<void*, void*> read_virtual(void* buffer, void* addr, std::size_t size);
		std::pair<void*, void*> write_virtual(void* buffer, void* addr, std::size_t size);

		template <class T>
		T read_virtual(void* addr)
		{
			if (!addr) return {};
			T buffer;
			read_virtual((void*)&buffer, addr, sizeof(T));
			return buffer;
		}

		template <class T>
		void write_virtual(void* addr, const T& data)
		{
			write_virtual((void*)&data, addr, sizeof(T));
		}

		//
		// linear address translation (not done by hyperspace mappings)
		//
		void* virt_to_phys(pt_entries& entries, void* addr);

		//
		// these are used for the pfn backdoor, this will be removed soon
		//
		void* set_page(void* addr);
		void* get_page() const;
		unsigned get_pid() const;

		pml4e operator[](std::uint16_t pml4_idx);
		pdpte operator[](const std::pair<std::uint16_t, std::uint16_t>& entry_idx);
		pde operator[](const std::tuple<std::uint16_t, std::uint16_t, std::uint16_t>& entry_idx);
		pte operator[](const std::tuple<std::uint16_t, std::uint16_t, std::uint16_t, std::uint16_t>& entry_idx);
	private:

		//
		// given an address fill pt entries with physical addresses and entry values.
		//
		bool hyperspace_entries(pt_entries& entries, void* addr);
		void* dirbase;
		kernel_ctx* k_ctx;
		std::uint16_t pml4e_index, pdpte_index, pde_index, pte_index, page_offset;

		/// first == physical
		/// second == virtual
		std::pair<ppdpte, ppdpte> new_pdpt;
		std::pair<ppde,ppde>      new_pd;
		std::pair<ppte, ppte>     new_pt;
		unsigned pid;
	};
}