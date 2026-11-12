// templates and the Rogue Wave STL that ships with BCC 5.5
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <iostream>

template <class T, int N> struct Ring {
    T buf[N];
    int head;
    Ring() : head(0) {}
    void push(const T &v) { buf[head++ % N] = v; }
    T sum() const { T s = T(); for (int i = 0; i < N; i++) s += buf[i]; return s; }
};

template <class T> T maxof(const std::vector<T> &v) { return *std::max_element(v.begin(), v.end()); }

int main()
{
    Ring<int, 4> r;
    for (int i = 1; i <= 6; i++) r.push(i * i);
    std::cout << "ring " << r.sum() << std::endl;

    std::vector<std::string> words;
    words.push_back("pear"); words.push_back("apple"); words.push_back("fig");
    std::sort(words.begin(), words.end());
    for (std::vector<std::string>::iterator it = words.begin(); it != words.end(); ++it)
        std::cout << *it << ' ';
    std::cout << "| max " << maxof(words) << std::endl;

    std::map<std::string, int> count;
    const char *text[] = { "a", "b", "a", "c", "a", "b" };
    for (int j = 0; j < 6; j++) count[text[j]]++;
    for (std::map<std::string, int>::iterator m = count.begin(); m != count.end(); ++m)
        std::cout << m->first << '=' << m->second << ' ';
    std::cout << std::endl;
    return 0;
}
