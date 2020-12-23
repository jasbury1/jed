#ifndef TRIE_H
#define TRIE_H

#include <unordered_map>
#include <memory>

class Trie
{
public:

    Trie();
    ~Trie();

    void insert(const std::string& word, int newVal);
    int get(const std::string& word) const;

private:
    struct TrieNode {
        int val = -1;
        std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    };

    std::unique_ptr<TrieNode> root = std::make_unique<TrieNode>();
    
};

#endif // TRIE_H