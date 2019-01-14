#ifndef ARTICLE_H
#define ARTICLE_H

#include <string>
#include "http.h"

namespace crawler
{

class Article
{
public:
    Article(const char* d, const std::string& u, Response& res);

    ~Article() = default;

    void read_from_file(const std::string& name);

    void save_to_file();

private:
    void init_others(Response::Block& head);
    void init_content(Response::Block& body);

    void convert_encoding();

    std::string title;
    std::string author;
    std::string charset;
    std::string publishdate;
    std::string domain;
    std::string url;
    std::string description;
    std::string content;
}; // class Article

} // namespace crawler
#endif // ARTICLE_H