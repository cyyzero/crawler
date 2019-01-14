#include <fcntl.h>
#include <unistd.h>
#include "http.h"
#include "crawl.h"

// #define DEBUG

using namespace crawler;

std::string host(HOST);

int main()
{
    crawl(host);
}

