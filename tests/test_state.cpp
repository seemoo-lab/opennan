extern "C" {
#include "state.h"
}

#include "gtest/gtest.h"

namespace {
    TEST(TestState, testRand) {
        uint8_t min = 0;
        uint8_t max = 255;
        uint32_t result = 0;

        for (int i = 0; i < 20000; ++i) {
            result = get_rand_num(min, max);
            ASSERT_LE(result, max);
            ASSERT_GE(result, min);
        }
    }

    TEST(TestState, testIncrement) {
        struct nan_state nstate;
        struct ether_addr addr = {0x50, 0x6f, 0x9a, 0x42, 0x42, 0x42,};;

        init_nan_state(&nstate, addr);
        increment_MP(&nstate);

        ASSERT_EQ(nstate.master_preference, 1);

        nstate.master_preference = 255;
        increment_MP(&nstate);

        ASSERT_EQ(nstate.master_preference, 255);
    }
}

