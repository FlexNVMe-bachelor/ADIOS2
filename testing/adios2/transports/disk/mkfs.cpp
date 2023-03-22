#include "mkfs.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>

void mkfs(std::string deviceUrl) { mkfs(deviceUrl, 0, 0); }

void mkfs(std::string deviceUrl, size_t blockCountPrSlab)
{
    mkfs(deviceUrl, blockCountPrSlab, 0);
}

void mkfs(std::string deviceUrl, size_t blockCountPrSlab, size_t poolCount)
{
    if (blockCountPrSlab == 0)
    {
        blockCountPrSlab = 32;
    }

    std::string command = "mkfs.flexalloc";
    command += " -s " + std::to_string(blockCountPrSlab);

    if (poolCount != 0)
    {
        command += " -p " + std::to_string(poolCount);
    }

    command += " " + deviceUrl;
    command += " 2>&1";

    std::cout << "mkfs.flexalloc deviceUrl: " << deviceUrl << "\n";
    std::cout << "mkfs.flexalloc comand: " << command << "\n";

    FILE *proc = popen(command.c_str(), "r");
    if (!proc)
    {
        std::cerr << "Failed to execute command\n";
        std::exit(1);
    }

    char buffer[128];
    std::string result = "";
    while (!feof(proc))
    {

        // use buffer to read and add to result
        if (fgets(buffer, 128, proc) != NULL)
        {
            result += buffer;
        }
    }

    std::cout << "mkfs.flexalloc output: " << result << "\n";
}
