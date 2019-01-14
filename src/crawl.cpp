#define MULTI_THREAD
#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>

#include "http.h"
#include "article.h"
#include "crawl.h"

#ifdef MULTI_THREAD
#include "safe_set.h"
#include "block_queue.h"
#include "thread_pool.h"
#else
#include <unordered_set>
#include <queue>
#endif

using namespace crawler;

#ifdef MULTI_THREAD
std::atomic_int count;
Safe_set<std::string> hosts_set;
Block_queue<std::string> hosts_queue;
#else
int count;
std::unordered_set<std::string> hosts_set;
std::queue<std::string> hosts_queue;
#endif

namespace crawler
{

void get_urls(Response::Block& body);

void crawl(std::string host)
{
#ifdef MULTI_THREAD
    thread_pool<20> pool;
#endif

    hosts_set.insert(host);
    hosts_set.insert(host+"/");

    if (mkdir(host.c_str(), 0777) == -1)
    {
        if (errno != EEXIST)
        {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }
    if (chdir(host.c_str()) == -1)
    {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

#ifdef MULTI_THREAD
    std::thread t ([&] () {
#endif
        Request r(host);

        Response res = r.slow_get_response();

        get_urls(res.get_body());
#ifdef MULTI_THREAD
    });

    t.detach();
#endif



    while (true)
    {
        auto && h = hosts_queue.front();
#ifdef MULTI_THREAD
        pool.add_job( [host = std::move(h)] () {
            process_one(std::move(host));
        });
#else
        process_one(h);
#endif
        hosts_queue.pop();
    }
}

void process_one(std::string host)
{
    // std::cout << '\t' << host.size() << " " << host << std::endl;
    // std::cout << "THREAD ID: " << std::this_thread::get_id() << std::endl;
    try
    {
        auto p = host;
        Request r(p);
        Response res = r.fast_get_response();

        crawler::Response::Block& body = res.get_body();
        std::cout << "HOST:" << count++ << " " << host << "\n ";
        // if (count == 300)
        // {
        //     exit(1);
        // }

        get_urls(body);

        Article article(HOST, host, res);
        article.save_to_file();
    }
    catch (...)
    {
        // throw;
    }
}

void get_urls(Response::Block& body)
{
    // const char *p = "href=\"http://" HOST;
    // std::cout << p << "\n";
    for (std::size_t i = 0; i < body.size();)
    {
        if (body.start_with("href=\"http://" HOST, i))
        {
            std::size_t j = i;
            while (body[i] != '\"')
                ++i;
            body[i] = '\0';
            std::string url(HOST);
            url.append(body.begin() + j);
            if (hosts_set.find(url) == hosts_set.end())
            {
                // std::cout << "insert" << std::endl;
                hosts_set.insert(url);
                hosts_queue.push(std::move(url));
                // std::cout << "push" << hosts_queue.size();
            }
        }
        else
        {
            ++i;
        }
    }
}
} // namespace crawler