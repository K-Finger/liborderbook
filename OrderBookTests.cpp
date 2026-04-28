#include <iostream>

#include "Types.h"
#include "Order.h"
#include "OrderBook.h"
#include "OrderBook.cpp"

#define IS_TRUE(x) { if (!(x)) std::cout << __FUNCTION__ << " failed on line " << __LINE__ << std::endl; }

void test_addOrder()
{
    OrderBook orderBook
    IS_TRUE 
}

int main(void) {
    test_addOrder();
}