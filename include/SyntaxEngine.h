#ifndef SYNTAX_ENGINE_H
#define SYNTAX_ENGINE_H

#include <string>
#include <vector>

#include "Trie.h"
#include "Model.h"

class SyntaxEngine
{
public:
    enum highlightMode
    {
        HL_NORMAL = 0,
        HL_COMMENT,
        HL_MLCOMMENT,
        HL_KEYWORD1,
        HL_KEYWORD2,
        HL_STRING,
        HL_NUMBER,
        HL_MATCH,
        HL_ERROR
    };

    SyntaxEngine();
    ~SyntaxEngine();

    void setFileType(const std::string& extension);
    void updateSyntaxHighlight(std::vector<Model::erow> &rowList, std::size_t rowIndex) const;

    bool highlightStrings() const { return true; }
    bool highlightNums() const { return true; }

    std::string fileType() const { return "c"; }

private:
    Trie termTrie;

    std::vector<std::string> keywords = {
        "switch", "if", "while", "for", "break", "continue", "return", "else",
        "struct", "union", "typedef", "static", "enum", "class", "case"};

    std::vector<std::string> types = {
        "int", "long", "double", "float", "char", "unsigned", "signed", "void"};

    std::string brackets{"()[]{}"};
    std::string separaters { ",.+-/*=~%<>;()[]{}"};

    bool isSeparator(int c) const {return isspace(c) || separaters.find(c) != std::string::npos;}


    std::string singlelineComment{"//"};
    std::string multilineCommentStart{"/*"};
    std::string multilineCommentEnd{"*/"};

    void processString(std::size_t &idx, Model::erow& row) const;
    void processNumber(std::size_t& idx, Model::erow& row) const;
    void processSeparator(std::size_t& idx, Model::erow& row) const;
    void processWord(std::size_t &idx, Model::erow& row) const;
    void processMultilineComment(std::size_t &idx, bool& inComment, Model::erow& row) const;

};

#endif // SYNTAX_ENGINE_H