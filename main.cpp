#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <thread>

#include "concurrent_queue.h"

uint32_t SieveOfEratosthenes(uint32_t n)
{
    // Create a boolean array "prime[0..n]" and initialize
    // all entries it as true. A value in prime[i] will
    // finally be false if i is Not a prime, else true.
    std::vector<bool> prime(n + 1,true);

    for (uint32_t p = 2; p*p <= n; p++)
    {
        // If prime[p] is not changed, then it is a prime
        if (prime[p] == true)
        {
            // Update all multiples of p
            for (uint32_t i = p * 2; i <= n; i += p)
                prime[i] = false;
        }
    }

    // Print sum of all prime numbers
    uint32_t    sum = 0;
    for (uint32_t p = 2; p <= n; p++)
    {
        if (prime[p])
        {
            sum += p;
        }
    }

    return sum;
}

typedef std::function< uint32_t(    const std::uniform_int_distribution<> & dis,
                                    const uint32_t                          uiNumInts) > SieveFunc;

uint32_t SerialSieve(   const std::uniform_int_distribution<> & dis,
                        const uint32_t                          uiNumInts)
{
    uint32_t        sum = 0;
    std::mt19937    gen(0);

    for (uint32_t i = 0; i < uiNumInts; i++)
    {
        sum += SieveOfEratosthenes(dis(gen));
    }

    return sum;
}


concurrent_queue<uint32_t>  producer;
concurrent_queue<uint32_t>  output;
uint32_t                    sum = 0;

void ProducerThreadFn(  const std::uniform_int_distribution<> & dis,
                        const uint32_t                          uiNumInts )
{
    std::mt19937                gen(0);

    for (uint32_t i = 0; i < uiNumInts; i++)
    {
        producer.push(dis(gen));
    }
}

void ConsumerThreadFn()
{
    while (!producer.empty())
    {
        uint32_t  v;
        producer.wait_and_pop(v);
        output.push(SieveOfEratosthenes(v));
    }
}

const uint32_t  NUM_THREADS = 32;

uint32_t ParallelSieve( const std::uniform_int_distribution<> & dis,
                        const uint32_t                          uiNumInts)
{
    std::mt19937                gen(0);

    std::thread ProducerThread(ProducerThreadFn, dis, uiNumInts);
    std::thread ConsumerThreads[NUM_THREADS];

    for ( auto & t : ConsumerThreads )
    {
        t = std::thread(ConsumerThreadFn);
    }

    ProducerThread.join();

    for (auto & t : ConsumerThreads)
    {
        t.join();
    }

    while (!output.empty())
    {
        uint32_t  v;
        output.wait_and_pop(v);
        sum += v;
    }

    return sum;
}

void RunTest(   const std::uniform_int_distribution<> & dis,
                const uint32_t                          uiNumInts,
                const SieveFunc &                       sieveFn     )
{
    sum = 0;
    auto start = std::chrono::steady_clock::now();
    sum = sieveFn( dis, uiNumInts );
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    std::cout << "\n" << std::chrono::duration <double, std::milli>(diff).count() << " ms\n";
    std::cout << "\nSum = " << sum << "\n";
    std::string strResult;
    (sum == 4106564866) ? strResult = "True" : strResult = "False";
    std::cout << strResult << "\n";
}

void main(void)
{
    const uint32_t  MAX_RAND_INT    = 1000000;
    const uint32_t  NUM_RAND_INTS   = 100000;

    //std::random_device rd;
    //std::mt19937 gen(rd());

    std::uniform_int_distribution<> dis(1, MAX_RAND_INT);

    //RunTest( dis, NUM_RAND_INTS, SerialSieve );
    RunTest( dis, NUM_RAND_INTS, ParallelSieve );

    std::cout << "\nPress any key\n";
    std::cin.ignore();
}
