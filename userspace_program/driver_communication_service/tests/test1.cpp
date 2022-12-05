#include "dbus_interaction.hpp"

#include "CppUTest/TestHarness.h"

TEST_GROUP(DBusTest) {
    void setup() {
        // setting up the test environment for the test group
    }

    void teardown() {
        // cleaning the test environment for the test group
    }
};

TEST(DBusTest, FirstTest) {
    STRCMP_EQUAL("Foo", "Bar");
}