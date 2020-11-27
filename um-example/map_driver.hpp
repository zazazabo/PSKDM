#pragma once
#include <cstddef>
#include <cstdint>
#include <map>

namespace mapper
{
	enum class mapper_error
	{
		error_success,			// everything is good!
		image_invalid,			// the driver your trying to map is invalid (are you importing things that arent in ntoskrnl?)
		load_error,			// unable to load signed driver into the kernel (are you running as admin?)
		unload_error,			// unable to unload signed driver from kernel (are all handles to this driver closes?)
		piddb_fail,			// piddb cache clearing failed... (are you using this code below windows 10?)
		init_failed,			// setting up library dependancies failed!
		failed_to_create_proc,		// was unable to create a new process to inject driver into! (RuntimeBroker.exe)
		set_mgr_failure,		// unable to stop working set manager thread... this thread can cause issues with PTM...
		zombie_process_failed
	};

	/// <summary>
	/// map a driver only into your current process...
	/// </summary>
	/// <param name="drv_image">base address of driver buffer</param>
	/// <param name="image_size">size of the driver buffer</param>
	/// <param name="entry_data">data to be sent to the entry point of the driver...</param>
	/// <returns>status of the driver being mapped, and base address of the driver...</returns>
	auto map_driver(std::uint32_t pid, std::uint8_t* drv_image, std::size_t image_size, void** entry_data) -> std::pair<mapper_error, void*>;
}