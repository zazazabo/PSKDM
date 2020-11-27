#include <iostream>
#include <Windows.h>

#include "utils.h"
#include "map_driver.hpp"

int __cdecl main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf("[!] please provide a path to a driver...\n");
        return -1;
    }

    std::vector<std::uint8_t> driver_data;
    utils::open_binary_file(argv[1], driver_data);
    std::printf("[+] loaded driver off disk...\n");

    const auto [result, driver_base] = 
        mapper::map_driver
        (
            GetCurrentProcessId(), // you can map the driver into whatever context you want...
            driver_data.data(),
            driver_data.size(),
            nullptr // you can pass your structure here...
        );

    std::printf("[+] driver mapping result -> 0x%x (0 == mapper_error::error_success)\n", result);
    std::printf("[+] driver base address (usermode) -> 0x%p\n", driver_base);
    std::getchar();
}