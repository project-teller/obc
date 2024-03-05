#include <gtest/gtest.h>
#include <thread>

#include "hal/queue.hpp"

using namespace teller::hal;

// Uncomment the next line to run heavier stress tests
// #define STRESS_TEST

TEST(BlockingQueue, simple_send_receive)
{
    const int n = 1000;
    int res;

    BlockingQueue<int> queue(n * 2);
    ASSERT_TRUE(queue.empty());

    for (int i = 0; i < n; ++i) {
        ASSERT_TRUE(queue.send(i));
        ASSERT_TRUE(queue.receive(res));
        ASSERT_EQ(i, res);
    }

    ASSERT_TRUE(queue.empty());

    ASSERT_TRUE(queue.close());
    ASSERT_FALSE(queue.receive(res));
    ASSERT_TRUE(queue.closed());
    ASSERT_TRUE(queue.empty());
}

TEST(BlockingQueue, send_and_close)
{
    int res;
    BlockingQueue<int> queue(64);

    ASSERT_TRUE(queue.empty());
    ASSERT_FALSE(queue.closed());

    ASSERT_TRUE(queue.send(239));
    ASSERT_TRUE(queue.close());
    ASSERT_FALSE(queue.empty());
    ASSERT_TRUE(queue.closed());

    ASSERT_TRUE(queue.receive(res));
    ASSERT_EQ(239, res);

    ASSERT_FALSE(queue.receive(res));
}

TEST(BlockingQueue, send_and_clear)
{
    int res;
    BlockingQueue<int> queue(64);

    ASSERT_TRUE(queue.empty());

    ASSERT_TRUE(queue.send(239));
    ASSERT_TRUE(queue.send(42));
    ASSERT_TRUE(queue.send(27));
    ASSERT_FALSE(queue.empty());

    ASSERT_TRUE(queue.clear());
    ASSERT_TRUE(queue.empty());
}

class QueuePopWorker {

    BlockingQueue<int>& queue;

public:
    std::vector<int> results;

    QueuePopWorker(BlockingQueue<int>& queue)
        : queue(queue)
    {
    }

    void start()
    {
        int result;
        while (queue.receive(result)) {
            results.push_back(result);
        }
        ASSERT_TRUE(queue.empty());
        ASSERT_TRUE(queue.closed());
    }
};

class QueuePushWorker {

    BlockingQueue<int>& queue;
    std::vector<int> data;
    bool should_close;

public:
    QueuePushWorker(BlockingQueue<int>& queue, std::vector<int> data, bool should_close)
        : queue(queue)
        , data(data)
        , should_close(should_close)
    {
    }

    void start()
    {
        int result;
        for (auto value : data) {
            ASSERT_LE(queue.size(), queue.limit());
            ASSERT_TRUE(queue.send(value));
            ASSERT_LE(queue.size(), queue.limit());
        }
        if (should_close) {
            queue.close();
        }
    }
};

TEST(BlockingQueue, single_producer_single_consumer)
{
#ifdef STRESS_TEST
    const int runs = 1000;
    const int n = 1000;
#else
    const int runs = 10;
    const int n = 100;
#endif
    const int limit = 100;
    for (int run = 0; run < runs; ++run) {
        BlockingQueue<int> queue(limit);
        ASSERT_TRUE(queue.empty());

        std::vector<int> data;
        for (int i = 0; i < n; ++i) {
            data.push_back(i);
        }

        QueuePushWorker producer(queue, data, true);
        QueuePopWorker consumer(queue);

        std::thread consumer_thread(&QueuePopWorker::start, &consumer);
        std::thread producer_thread(&QueuePushWorker::start, &producer);

        producer_thread.join();
        consumer_thread.join();

        ASSERT_TRUE(queue.empty());
        ASSERT_TRUE(queue.closed());

        ASSERT_EQ(consumer.results.size(), n);
        for (int i = 0; i < n; ++i) {
            ASSERT_EQ(consumer.results[i], i);
        }
    }
}

TEST(BlockingQueue, multiple_producer_multilple_consumer)
{
    const int runs = 10;
#ifdef STRESS_TEST
    const int n = 10000;
#else
    const int n = 100;
#endif
    const int limit = 100;
    const int units = 10;
    for (int run = 0; run < runs; ++run) {
        BlockingQueue<int> queue(limit);
        ASSERT_TRUE(queue.empty());

        std::vector<QueuePushWorker> producers;
        std::vector<QueuePopWorker> consumers;
        for (int i = 0; i < units; ++i) {
            std::vector<int> data;
            for (int j = i * n; j < (i + 1) * n; ++j) {
                data.push_back(j);
            }
            producers.push_back(QueuePushWorker(queue, data, false));
            consumers.push_back(QueuePopWorker(queue));
        }

        std::vector<std::thread> producers_threads(units);
        std::vector<std::thread> consumers_threads(units);
        for (int i = 0; i < units; ++i) {
            producers_threads[i] = std::thread(&QueuePushWorker::start, &producers[i]);
            consumers_threads[i] = std::thread(&QueuePopWorker::start, &consumers[i]);
        }

        for (int i = 0; i < units; ++i) {
            producers_threads[i].join();
        }

        queue.close();

        for (int i = 0; i < units; ++i) {
            consumers_threads[i].join();
        }

        ASSERT_TRUE(queue.empty());
        ASSERT_TRUE(queue.closed());

        std::vector<bool> is_found(n * units, false);
        for (int i = 0; i < units; ++i) {
            for (int value : consumers[i].results) {
                ASSERT_FALSE(is_found[value]);
                ASSERT_GE(value, 0);
                ASSERT_LT(value, n * units);
                is_found[value] = true;
            }
        }

        for (int i = 0; i < n * units; ++i) {
            ASSERT_TRUE(is_found[i]);
        }
    }
}
