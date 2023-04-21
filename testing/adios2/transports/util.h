#ifndef FLEXNVME_TEST_UTIL_H
#define FLEXNVME_TEST_UTIL_H

#include <chrono>
#include <random>
class Rng
{
private:
    std::random_device rd;

public:
    Rng()
    {
        // From
        // https://stackoverflow.com/questions/13445688/how-to-generate-a-random-number-in-c
        std::mt19937::result_type seed =
            rd() ^ ((std::mt19937::result_type)
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count() +
                    (std::mt19937::result_type)
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::high_resolution_clock::now()
                                .time_since_epoch())
                            .count());

        std::mt19937 gen(seed);
    }

    auto RandRange(size_t min, size_t max) -> size_t
    {
        std::uniform_int_distribution<unsigned> distrib(min, max);
        return distrib(rd);
    }

    auto RandString(size_t length) -> std::string
    {
        const char ASCII_RANGE_START = 33, ASCII_RANGE_END = 122;
        return RandString(length, ASCII_RANGE_START, ASCII_RANGE_END);
    }

    auto RandString(size_t length, char asciiStart, char asciiEnd)
        -> std::string
    {
        std::string data = "";

        for (int i = 0; i < length; i++)
        {
            char randomChar =
                static_cast<char>(RandRange(asciiStart, asciiEnd));
            data += randomChar;
        }

        return data;
    }
};
#endif
