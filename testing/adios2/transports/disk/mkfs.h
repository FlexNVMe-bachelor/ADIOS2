#ifndef MKFS_H
#define MKFS_H

#include <string>

void mkfs(std::string deviceUrl);
void mkfs(std::string deviceUrl, size_t poolCount);
void mkfs(std::string deviceUrl, size_t blockCountPrSlab, size_t poolCount);

#endif
