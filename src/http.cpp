#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <utility>

#include "http.h"
// #define DEBUG
using namespace crawler;

struct in_addr Request::host_ip;

namespace
{
    struct in_addr get_IP(const char* name)
    {
        int status;
        struct addrinfo hint;
        struct addrinfo *result;

        memset((void*)&hint, 0, sizeof(hint));
        hint.ai_flags = AI_PASSIVE;
        hint.ai_family = AF_INET;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = 0;

        if ((status = getaddrinfo(name, "http", &hint, &result)) != 0)
        {
            fprintf(stderr, "%s", gai_strerror(status));
            exit(1);
        }
        // // std::cout << result->ai_canonname << "\n";
        // for (; result; result = result->ai_next) {
        //     auto ip = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
        //     printf("%s\n", inet_ntoa(ip));
        // }
        return ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
    }

    bool start_with(const std::string& raw_content, const char* prefix, std::size_t& pos)
    {
        std::size_t i = pos;
        for (; i < raw_content.size() && *prefix && (raw_content[i] == *prefix); ++prefix, ++i)
            continue;
        if (*prefix)
        {
            return false;
        }
        else
        {
            pos = i;
            return true;
        }
    }

    void pass_until_meet(const std::string& raw_content, const char* suffix, std::size_t& pos)
    {
        for (std::size_t i = pos; i < raw_content.size();)
        {
            const char *p = suffix;
            std::size_t tmp = i;
            for (; *p && *p == raw_content[tmp]; ++p, ++tmp)
                continue;
            if (*p)
                ++i;
            else
            {
                pos = tmp;
                return;
            }
        }
    }
}; // unnamed namespace

Response::Response(std::string&& s)
    : raw_content(std::move(s)), head(raw_content), body(raw_content)
{
    // std::cout << raw_content << "\n";
    remove_unrelated();
    generate_blocks();
}

Response::Block& Response::get_head()
{
    return head;
}

Response::Block& Response::get_body()
{
    return body;
}

void Response::remove_unrelated()
{
    auto pos = raw_content.find("\r\n\r\n");
    start = pos+4;
    std::size_t i = start, j = start;
    for (; i < raw_content.size();)
    {
        // std::cout << i << "\n";
#ifdef DEBUG
        if (start_with("href=\"http://blog.sina.com.cn", i))
        {
            for (size_t x = i-23; x < i; ++x) {
                std::cout << raw_content[x];
            }
            std::cout << "\n";
        }
        else
#endif
        if (start_with("<script", i))
        {
            pass_until_meet("</script", i);
            while (raw_content[i++] != '>')
                continue;
        }
        else if (start_with("<style", i))
        {
            // std::cout << i << "\n";
            pass_until_meet("</style", i);
            while (raw_content[i++] != '>')
                continue;
        }
        else if (start_with("<!--", i))
        {
            pass_until_meet("-->", i);
        }
        else if (raw_content[i] == '\0')
            ++i;
        else
        {
            // std::cout << i << "\n";
            raw_content[j++] = raw_content[i++];
        }
    }
    raw_content[j] = '\0';
    finish = j;
    length = finish - start;
    // std::cout << raw_content.c_str();
}

void Response::generate_blocks()
{
    std::size_t head_start, head_finish, body_start, body_finish;
    head_finish = body_finish = 0;
    for (std::size_t i = start; i < finish;)
    {
        if (start_with("<head", i))
        {
            while (raw_content[i++] != '>')
                continue;
            head_start = i;
            pass_until_meet("</head", i);
            head_finish = i-6;
            if (head_start >= head_finish)
                head_finish = 0;
            while (raw_content[i++] != '>')
                continue;
            for (std::size_t j = i; j < finish; ++j)
            {
                if (start_with("<body", j))
                {
                    while (raw_content[i++] != '>')
                        continue;
                    body_start = i;
                    body_finish = finish -1;
                    reverse_pass_until_meet("ydob/<", body_finish);
                    if (body_start >= body_finish || body_finish >= finish)
                        body_finish = 0;
                    goto FINISH;
                }
            }
        }
        else
            ++i;
    }

FINISH:
    if (head_finish == 0 || body_finish == 0)
        throw std::logic_error("head_finish and body finish shouldn't be 0\n");
    raw_content[head_finish] = '\0';
    raw_content[body_finish] = '\0';
    head.init(head_start, head_finish);
    body.init(body_start, body_finish);
    // std::cout << body_start << " " << body_finish << " " << finish << "\n";
}

bool Response::start_with(const char* prefix, std::size_t& pos) const
{
    return ::start_with(raw_content, prefix, pos);
}

void Response::pass_until_meet(const char *suffix, std::size_t& pos)
{
    ::pass_until_meet(raw_content, suffix, pos);
}

void Response::reverse_pass_until_meet(const char *prefix, std::size_t& pos)
{
    for (int64_t i = pos; pos >= 0;)
    {
        const char *p = prefix;
        std::size_t tmp = i;
        for (; *p && *p == raw_content[tmp]; ++p, --tmp)
            continue;
        if (*p)
            --i;
        else
        {
            pos = tmp;
            return;
        }
    }
}

bool Response::Block::start_with(const char* prefix, std::size_t& pos) const
{
    pos += start;
    bool ans = ::start_with(data, prefix, pos);
    pos -= start;
    return ans;
}

void Response::Block::pass_until_meet(const char* suffix, std::size_t& pos) const
{
    pos += start;
    ::pass_until_meet(data, suffix, pos);
    pos -= start;
}

const char* Response::Block::c_str() const
{
    return data.c_str() + start;
}

char* Response::Block::begin()
{
    return const_cast<char*>(data.c_str() + start);
}

const char* Response::Block::begin() const
{
    return data.c_str() + start;
}

char* Response::Block::end()
{
    return const_cast<char*>(data.c_str() + finish);
}

const char* Response::Block::end() const
{
    return data.c_str() + finish;
}

std::string::size_type Response::Block::size() const
{
    return length;
}

std::string::reference Response::Block::operator[](std::size_t pos)
{
    return data[start + pos];
}

std::string::const_reference Response::Block::operator[](std::size_t pos) const
{
    return data[start + pos];
}

void Response::Block::init(std::size_t s, std::size_t f)
{
    start = s;
    finish = f;
    length = finish - start;
}

Request::Request(const std::string& url, Request::Method m, const Header& header)
{
    switch(m)
    {
    case GET:
        content.append("GET ");
        break;
    default:
        std::cerr << "Doesn't support other method now";
        exit(EXIT_FAILURE);
    }

    auto pos = url.find("/");
    std::string host;

    if (pos != std::string::npos)
    {
        host = url.substr(0, pos);
        content.append(url.substr(pos));
    }
    else
    {
        host = url;
        content.append("/");
    }

    content.append(" HTTP/1.1\r\nHost: " + host + "\r\n");

    host_url = std::move(host);

    for (const auto& head: header)
    {
        content.append(head.first + ": " + head.second + "\r\n");
    }
    content.append("\r\n");
}

Response Request::slow_get_response() const
{
    host_ip = get_IP(host_url.c_str());

    return get_response();
}

Response Request::fast_get_response() const
{
    return get_response();
}

Response Request::get_response() const
{
    int sock_fd;
    struct sockaddr_in serv_socket;
    uint16_t port = 80;

    memset(&serv_socket, 0, sizeof(serv_socket));
    serv_socket.sin_family = AF_INET;
    serv_socket.sin_port = htons(port);
    serv_socket.sin_addr = host_ip;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(sock_fd, (struct sockaddr*)&serv_socket, sizeof(serv_socket)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // std::cout << content << "\n";
    if (write(sock_fd, content.c_str(), content.size()) == -1)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    const size_t LEN = 1024ULL*4;

    char *buf = new char[LEN];

    std::string data;
    for (;;)
    {
        int len = recv(sock_fd, buf, LEN, 0);
        if (len == -1)
        {
                perror("recv");
                exit(EXIT_FAILURE);
        }
        else
        {
            if (len == 0)
                break;
            data.append(buf, len+1);
        }
    }

    close(sock_fd);
    // std::ofstream fout("test1.txt");

    // fout << data << "\n";
    return Response(std::move(data));
}