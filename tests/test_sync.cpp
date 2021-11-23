extern "C" {
#include "sync.h"
}

#include "gtest/gtest.h"

namespace {

    TEST(TestTsf, testTsfInit) {
        tsf_timer tsf;
        init_TSF(&tsf, 400);

        ASSERT_EQ(tsf.current_time, 0);
        ASSERT_EQ(tsf.base_time, 400);
    }

    TEST(TestTsf, testTsfUpdate) {
        tsf_timer tsf;
        init_TSF(&tsf, 1337);

        ASSERT_EQ(tsf.current_time, 0);
        ASSERT_EQ(tsf.base_time, 1337);

        update_TSF(&tsf, 9000);
        ASSERT_EQ(tsf.current_time, 9000 - 1337);
    }

    TEST(TestTsf, testTsfChangeBaseTimeToEarlier) {
        tsf_timer tsf;

        uint64_t world_time = 120;
        uint64_t base_time = 100;
        uint64_t other_base_time = 30;

        init_TSF(&tsf, base_time);

        ASSERT_EQ(tsf.current_time, 0);
        ASSERT_EQ(tsf.base_time, base_time);

        update_TSF(&tsf, world_time);
        ASSERT_EQ(tsf.current_time, world_time - base_time);

        // Other base time is 30, so their current time is 90 (world_time - other_base)
        int64_t diff = tsf.current_time - (world_time - other_base_time);
        change_base_time_TSF(&tsf, diff);

        ASSERT_EQ(tsf.base_time, other_base_time);
    }

    TEST(TestTsf, testTsfChangeBaseTimeToLater) {
        tsf_timer tsf;

        uint64_t world_time = 600;
        uint64_t base_time = 100;
        uint64_t other_base_time = 400;

        init_TSF(&tsf, base_time);

        ASSERT_EQ(tsf.current_time, 0);
        ASSERT_EQ(tsf.base_time, base_time);

        update_TSF(&tsf, world_time);
        ASSERT_EQ(tsf.current_time, world_time - base_time);

        // Other base time is 30, so their current time is 90 (world_time - other_base)
        int64_t diff = tsf.current_time - (world_time - other_base_time);
        change_base_time_TSF(&tsf, diff);

        ASSERT_EQ(tsf.base_time, other_base_time);
    }

    TEST(TestTsf, testTsfInDw) {
        tsf_timer tsf;

        uint64_t world_time = 15 * 1024;
        uint64_t base_time = 0;

        init_TSF(&tsf, base_time);

        ASSERT_EQ(tsf.current_time, 0);
        ASSERT_EQ(tsf.base_time, base_time);

        update_TSF(&tsf, world_time);
        ASSERT_EQ(tsf.current_time, world_time - base_time);

        bool in_dw = nan_in_dw(tsf.current_time);
        ASSERT_EQ(in_dw, true);
    }

    TEST(TestTsf, testTsfOutDw) {
        tsf_timer tsf;

        uint64_t world_time = 16 * 1024;
        uint64_t base_time = 0;

        init_TSF(&tsf, base_time);

        ASSERT_EQ(tsf.current_time, 0);
        ASSERT_EQ(tsf.base_time, base_time);

        update_TSF(&tsf, world_time);
        ASSERT_EQ(tsf.current_time, world_time - base_time);

        bool in_dw = nan_in_dw(tsf.current_time);
        ASSERT_EQ(in_dw, false);
    }

    TEST(TestSync, testWarmUp) {
        tsf_timer tsf;

        uint64_t world_time = 120000000; // 120 seconds
        uint64_t base_time = 0;

        init_TSF(&tsf, base_time);
        ASSERT_EQ(tsf.warmup_start_time, base_time);
        ASSERT_EQ(tsf.cancel_warmup, false);

        ASSERT_EQ(tsf.current_time, 0);
        ASSERT_EQ(tsf.base_time, base_time);

        update_TSF(&tsf, world_time);

        ASSERT_EQ(warmup_timer_done(&tsf, world_time), true);
    }

    TEST(TestSync, testWarmUpCancel) {
        tsf_timer tsf;

        uint64_t world_time = 120000000; // 120 seconds
        uint64_t base_time = 0;

        init_TSF(&tsf, base_time);
        ASSERT_EQ(tsf.warmup_start_time, base_time);
        ASSERT_EQ(tsf.cancel_warmup, false);

        ASSERT_EQ(tsf.current_time, 0);
        ASSERT_EQ(tsf.base_time, base_time);

        update_TSF(&tsf, world_time);
        tsf.cancel_warmup = true;

        ASSERT_EQ(warmup_timer_done(&tsf, world_time), false);
    }
}

