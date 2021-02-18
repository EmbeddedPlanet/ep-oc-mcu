/*
 * Copyright (c) 2013-2016, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <string.h>
#include "greentea-client/test_env.h"
#include "unity/unity.h"
#include "utest/utest.h"
#include "kvstore_global_api/kvstore_global_api.h"

#include "PersistentVariable.h"

#include "drivers/Timeout.h"

/**
 * Note: This test is mostly to ensure API calls are valid and build/operate properly
 * in common use cases. It does NOT test the persistence of the data itself
 */

using namespace utest::v1;
using namespace ep;

void test_case_primitive_types()
{
    static PersistentVariable<uint8_t> test_uint8(1, "test_uint8_t");
    TEST_ASSERT_EQUAL(2, test_uint8 + 1);
    test_uint8 = 3;
    TEST_ASSERT_EQUAL(4, test_uint8 + 1);

    PersistentVariable<float> test_float(2.0f, "test_float");
    TEST_ASSERT_EQUAL_FLOAT(3.5f, test_float + 1.5f);
    test_float = 3.25f;
    TEST_ASSERT_EQUAL_FLOAT(4.50f, test_float + 1.25f);
}

struct test_struct_t {
    uint8_t test_uint8 = 1;
    float test_float = 2.15f;
    int16_t test_int16 = -1204;
    char name[8];

    test_struct_t() {
        strcpy(name, "hello");
    }
};

void test_case_struct_types()
{
    static PersistentVariable<test_struct_t> test_struct(test_struct_t(),
            "test_struct");

    TEST_ASSERT_EQUAL(test_struct.get().test_uint8, 1);
    TEST_ASSERT_EQUAL_FLOAT(test_struct.get().test_float, 2.15f);
    TEST_ASSERT_EQUAL(test_struct.get().test_int16, -1204);
    TEST_ASSERT_EQUAL_STRING(test_struct.get().name, "hello");

    test_struct_t modified;
    modified.test_uint8 = 2;
    modified.test_float = 2.2f;
    modified.test_int16 = -1222;
    strcpy(modified.name, "world");

    test_struct.set(modified);

    TEST_ASSERT_EQUAL(test_struct.get().test_uint8, 2);
    TEST_ASSERT_EQUAL_FLOAT(test_struct.get().test_float, 2.2f);
    TEST_ASSERT_EQUAL(test_struct.get().test_int16, -1222);
    TEST_ASSERT_EQUAL_STRING(test_struct.get().name, "world");

}

void test_case_array_types()
{

    uint8_t initial[7]= {0, 1, 2, 3, 4, 5, 6};
    uint8_t updated[7] = { 0 };
    PersistentArray<uint8_t, 7> test_array(mbed::make_Span(initial), "test_array");

    for(int i = 0; i < 7; i++ ) {
        updated[i] = test_array.get()[i] + 1;
    }

    test_array.set(mbed::make_Span(updated));
    for(int i = 0; i < 7; i++) {
        TEST_ASSERT_EQUAL(updated[i], test_array.get()[i]);
    }

}

void test_case_string_types()
{
// TODO
}


static PersistentVariable<uint8_t> test_irq(1, "test_irq");
static volatile uint8_t interrupt_val;
static volatile bool interrupt_done = false;
mbed::Timeout _timeout;

void test_case_interrupt_handler() {
    interrupt_val = test_irq;
    interrupt_done = true;
}

void test_case_interrupt() {

    interrupt_val = 0;
    _timeout.attach_us(test_case_interrupt_handler, 10000);
    while(!interrupt_done) {

    }

    TEST_ASSERT_EQUAL(interrupt_val, test_irq);

}

utest::v1::status_t greentea_failure_handler(const Case *const source, const failure_t reason)
{
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_CONTINUE;
}

// Generic test cases
Case cases[] = {
    Case("primitive types", test_case_primitive_types, greentea_failure_handler),
    Case("struct types", test_case_struct_types, greentea_failure_handler),
    Case("array types", test_case_array_types, greentea_failure_handler),
    Case("string types", test_case_string_types, greentea_failure_handler),
    Case("irq test", test_case_interrupt, greentea_failure_handler)
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(20, "default_auto");

    /** Format KVStore */
    int err = kv_reset(KV_STORE_DEFAULT_PARTITION_NAME(MBED_CONF_STORAGE_DEFAULT_KV));
    TEST_ASSERT_EQUAL_ERROR_CODE(MBED_SUCCESS, err);

    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);

int main()
{
    Harness::run(specification);
}
