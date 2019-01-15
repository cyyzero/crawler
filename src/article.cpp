#include <iostream>
#include <fstream>
#include <memory>
#include <cstdio>
#include <iconv.h>

#include "article.h"
#include "http.h"

#define FPUTS(field) \
    fputs(field.c_str(), file.get()); \
    fputs("\n", file.get());

#define CONVERT(field) \
    if (!field.empty())                                \
    {                                                  \
        char *p = const_cast<char*>(field.c_str());    \
        in_len = field.size();                         \
        char *pp = buf;                                \
        out_len = len;                                 \
        iconv(cd, &p, &in_len, &pp, &out_len);         \
        field = std::string(buf);                      \
    }


#define FPRINT(field) fprintf(file.get(), #field ":")
 
#define GETLINE(field)                                                     \
    do {                                                                   \
        std::getline(fin, field);                                          \
        field = std::string(field.begin()+field.find(":")+1, field.end()); \
    } while (0)

#define FIND(field)                                 \
    if (head.start_with("name=\"" #field, i))       \
    {                                               \
        head.pass_until_meet("content=\"", i);      \
        std::size_t j = i;                          \
        while (head[i++] != '"')                    \
            continue;                               \
        field = std::string(head.c_str()+j, i-j-1); \
    }

namespace
{
#define ALPHA 0.6
#define BATE  10

using namespace std;

using namespace crawler;


bool getfre(Response::Block& block, int start, int lenth)
{
    int i = 0;
    std::string text = std::string(block.c_str()+ start, lenth);
    int p1, p2;
    int len = 0;
    while (1)
    {
        p1 = text.find("<", i);
        p2 = text.find(">", i);

        if (p2 == -1 && p1 == -1) // |*************************|
            break;
        else
        {
            if (p1 == -1)
            { //   |---------------->*****************|
                len += p2 + i;
                i = p2 + 1;
            }
            if (p2 == -1)
            { //  |********************<-----------------|
                len += lenth - p1;
                break;
            }
            if (p2 >= 0 && p1 >= 0)
            {
                //   |***************<---------------->************|
                //		|----------------->******************<---------->**********|
                if (p2 > p1)
                {
                    len += p2 - p1;
                    i = p2 + 1;
                }
                else
                {
                    len += p2;
                    i = p1;
                }
            }
        }
    }
    float c;
    c = (float)len / (float)(lenth);
    if (c > ALPHA)
        return false;
    else
        return true;
}

int getM(Response::Block& text, int &mzx, int num)
{
    // int M = 0;
    int *nop = new int[num];
    int dt = text.size() / num;
    for (int i = 0; i < num; i++)
    {
        if (getfre(text, i * dt, dt))
        {
            nop[i] = 1;
        }
        else
            nop[i] = 0;
    }
    // int start = 0;
    int mstart = 0;
    int flag = 0;
    int maxzz = 0;
    int count = 0;
    for (int i = 0; i < num; i++)
    {
        if (nop[i] == 1)
        {
            if (flag == 0)
            { //开始计数

                count++;
                flag = 1;
            }
            count++;
        }
        else
        {
            if (flag == 1)
            {
                if (count >= maxzz)
                {
                    maxzz = count;
                    mstart = i - count;
                }
                count = 0;
                flag = 0;
            }
        }
    }
    mzx = mstart;
    delete[] nop;
    return maxzz;
}

void gettagnum(Response::Block &text, int M, short *x)
{
    int flag1 = 0; //标记还未遇到<
    for (int i = 0; i < (int)text.size(); i++)
    {
        if (flag1 == 0)
        {
            x[i] = 0; //文本
        }
        if (flag1 == 1)
        {
            x[i] = 1;
        }
        if (text[i] == '<')
        { //开始
            flag1 = 1;
            x[i] = 1;
        }
        if (text[i] == '>')
        {
            x[i] = 1;
            flag1 = 0;
        }
    }
}


void getStart_End(short *x, int num, int &start, int &end, int M)
{
    int st = 0, en = 0;
    for (int i = 0; i < num; i++)
    {
        if (i < M)
        {
            st += x[i];
        }
        if (i > M)
        {
            en += x[i];
        }
    }
    int left = 0;

    int right = 0;
    int pstart = 0;
    int pend = num - 1;
    int mleft = 2 * left - pstart;
    int mright = 2 * right + pend;
    for (; pstart < M; pstart++)
    {
        left = left + x[pstart];
        if ((2 * left - pstart) > mleft)
        {
            mleft = (2 * left - pstart);
            start = pstart;
        }
    }
    for (; pend > M; pend--)
    {
        right = right + x[pend];
        if ((2 * right + pend) > mright)
        {
            end = pend;
            mright = (right + right + pend);
        }
    }
}

std::string getcontent(Response::Block &a, int len)
{
    int num = a.size() + 1;
    short *z;
    z = new short[num];
    int m = 0;
    gettagnum(a, m, z);            //将文本和标签字符用1 0 表示
    int num1 = a.size() / len;   //区间设置为len长度
    int zz = a.size() / num1;
    int ff;
    m = getM(a, ff, num1);         //getM()获得M点的下标
    m = (ff + m / 2) * zz;
    int start, end;
    getStart_End(z, a.size(), start, end, m); //获得文本的开始和结束位置
    delete[] z;
    
    if ((start >= 0 && start < end && end < (int)a.size()))
    {
        start = (int)a.size() / 5;
        end = start * 2;
    }

    return std::string(a.c_str()+start, end - start);
}

std::string denoising(Response::Block& body)
{
    std::string s = getcontent(body, body.size() / BATE);
    size_t i = 0, j = 0;
    while (i < s.size())
    {
        if (s[i] == '<')
        {
            while (i < s.size() && s[i++] != '>')
                continue;
        }
        else if (s[i] == '&')
        {
            while (i < s.size() && s[i++] != ';')
                continue;
        }
        else
            s[j++] = s[i++];
    }
    s[j] = '\0';
    return std::string(s.c_str());
}
} // unnamed namespace


namespace crawler
{
Article::Article(const char* d, const std::string& u, Response& res)
    : domain(d), url(u)
{
    auto head = res.get_head();
    auto body = res.get_body();

    init_others(head);
    init_content(body);
}

void Article::save_to_file()
{
    std::unique_ptr<FILE, decltype(&fclose)> file(fopen((title+".html").c_str(), "w"), &fclose);
    if (file)
    {
        FPRINT(title);
        FPUTS(title);
        FPRINT(author);
        FPUTS(author);
        FPRINT(charset);
        FPUTS(charset);
        FPRINT(publishdate);
        FPUTS(publishdate);
        FPRINT(domain);
        FPUTS(domain);
        FPRINT(url);
        FPUTS(url);
        FPRINT(description);
        FPUTS(description);
        FPUTS(content);
    }
}

void Article::read_from_file(const std::string& name)
{
    std::ifstream fin(name);

    GETLINE(title);
    GETLINE(author);
    GETLINE(charset);
    GETLINE(publishdate);
    GETLINE(domain);
    GETLINE(url);
    GETLINE(description);

    std::string line;
    while (getline(fin, line))
    {
        content.append(line);
        content.push_back('\n');
    }
}

void Article::init_others(Response::Block& head)
{
    for (std::size_t i = 0; i < head.size(); )
    {
        if (head.start_with("charset=", i))
        {
            std::size_t j = i;
            while (head[i++] != '"')
                continue;
            charset = std::string(head.c_str()+j, i-j-1);
        }
        else if (head.start_with("<title", i))
        {
            while (head[i++] != '>')
                continue;
            std::size_t j = i;
            for (; i < head.size();)
            {
                if (head.start_with("</title", i))
                {
                    title = std::string(head.c_str()+j, i-j-7);
                    break;
                }
                else
                {
                    ++i;
                }
            }
        }
        else
            FIND(publishdate)
        else
            FIND(description)
        else
            FIND(author)
        else
            ++i;
    }
}

void Article::init_content(Response::Block& body)
{
    content = denoising(body);
    convert_encoding();
}

void Article::convert_encoding()
{
    auto l = content.size();
    std::size_t len = std::max(l, decltype(l)(4096));

    char *buf = new char[len];

    if (charset.empty() || charset == "UTF-8" || charset == "utf-8")
        return;
    iconv_t cd = iconv_open("UTF-8", charset.c_str());
    // std::cout << "charset:" << charset << std::endl;
    if (cd == (iconv_t)-1)
        return;

    std::size_t in_len, out_len;

    CONVERT(title);
    CONVERT(author);
    CONVERT(description);
    CONVERT(content);

    delete [] buf;
    iconv_close(cd);
}

} // namesapce crawler

#undef CONVERT
#undef FPUTS
#undef FPRINT
#undef GETLINE
#undef FIND