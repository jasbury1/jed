#include <array>
#include <string>
#include <unordered_map>

#include "Trie.h"

Trie::Trie()
{
}

Trie::~Trie()
{
}

void Trie::insert(const std::string &word, int nodeVal)
{
    if (word.empty())
    {
        return;
    }

    TrieNode *cur = root.get();
    for (const char &c : word)
    {
        if (cur->children.count(c) == 0)
        {
            cur->children.emplace(c, std::make_unique<TrieNode>());
        }
        cur = cur->children[c].get();
    }
    cur->val = nodeVal;
}

int Trie::get(const std::string& word) const
{
    if (word.empty())
    {
        return -1;
    }

    TrieNode *cur = root.get();
    for (const char &c : word)
    {
        if (cur->children.count(c) == 0)
        {
            return -1;
        }
        cur = cur->children[c].get();
    }
    return cur->val;
}
