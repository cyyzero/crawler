#include <iostream>
#include <fstream>
#include <unordered_set>
#include "http.h"

std::unordered_set<std::string> hosts;

int main()
{
    crawler::Request r("blog.sina.com.cn");
    auto res = r.slow_get_response();

    std::ofstream fout("test.txt");

    fout << res.get_data().c_str();
    std::cout << "head:\n\n" << res.get_head().c_str() << "\n";
    std::cout << "body:\n\n" << res.get_body().c_str() << "\n";
}