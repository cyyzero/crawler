#ifndef CRAWL_H
#define CRAWL_H

#define HOST "blog.sina.com.cn"

#include <string>
#include "http.h"

namespace crawler
{

void crawl(std::string host);
void process_one(std::string host);
void get_urls(const Response::Block& body);

};

#endif // CRAWL_H