#include <iostream>

#include "SyntaxEngine.h"

SyntaxEngine::SyntaxEngine()
{
}

SyntaxEngine::~SyntaxEngine()
{
}

void SyntaxEngine::setFileType(const std::string &extension)
{
    for (const auto &keyword : keywords)
    {
        termTrie.insert(keyword, HL_KEYWORD1);
    }

    for (const auto &type : types)
    {
        termTrie.insert(type, HL_KEYWORD2);
    }
}

void SyntaxEngine::updateSyntaxHighlight(std::vector<Model::erow> &rowList, std::size_t rowIndex) const
{
    if (rowIndex >= rowList.size() || rowIndex < 0)
    {
        return;
    }
    auto &curRow = rowList[rowIndex];
    std::fill(curRow.highlight.begin(), curRow.highlight.end(), HL_NORMAL);

    bool inComment = (curRow.idx > 0 && rowList[curRow.idx - 1].commentOpen);

    std::size_t i = 0;
    while(i < curRow.highlight.size() && isspace(curRow.render[i]))
    {
        ++i;
    }
    if(curRow.render[i] == '#')
    {
        std::fill(curRow.highlight.begin() + i, curRow.highlight.end(), HL_SPECIAL);
        while(!isSeparator(curRow.render[i])) {
            ++i;
        }
    }
    while (i < curRow.highlight.size())
    {
        auto c = curRow.render[i];

        // We are in a multiline comment. Look for the end
        if (inComment)
        {
            processMultilineComment(i, inComment, curRow);
        }
        // Look for the start of a multi-line comment
        else if (curRow.render.compare(i, multilineCommentStart.size(), multilineCommentStart) == 0)
        {
            std::fill_n(curRow.highlight.begin() + i, multilineCommentStart.size(), HL_MLCOMMENT);
            i += multilineCommentStart.size();
            inComment = true;
        }
        // Look for a single line comment
        else if (curRow.render.compare(i, singlelineComment.size(), singlelineComment) == 0)
        {
            std::fill_n(curRow.highlight.begin() + i, curRow.highlight.size() - i, HL_COMMENT);
            i = curRow.render.size();
        }
        // Look for the start of a string
        else if (c == '\'' || c == '"')
        {
            processString(i, curRow);
        }
        // Look for the start of a number
        else if (isdigit(c) || (c == '.' && c < curRow.render.size() && isdigit(curRow.render[i + 1])))
        {
            processNumber(i, curRow);
        }
        // Look for separation characters
        else if (isSeparator(c))
        {
            //processSeparator(i, curRow);
            ++i;
        }
        // Process a word
        else
        {
            processWord(i, curRow);
        }
    }

    // Check for changes that continue onto the next line
    bool statusChanged = (curRow.commentOpen != inComment);
    curRow.commentOpen = inComment;
    if (statusChanged && curRow.idx + 1 < rowList.size())
    {
        updateSyntaxHighlight(rowList, rowIndex + 1);
    }
}

void SyntaxEngine::processString(std::size_t &idx, Model::erow &row) const
{
    auto c = row.render[idx];
    row.highlight[idx] = HL_STRING;
    ++idx;
    if (idx == row.highlight.size())
    {
        return;
    }

    while (idx < row.highlight.size() - 1 && row.render[idx] != c)
    {
        row.highlight[idx] = HL_STRING;
        // Backslashes denote the next character is part of the string
        if (row.render[idx] == '\\' && idx + 1 < row.render.size())
        {
            row.highlight[idx + 1] = HL_STRING;
            ++idx;
        }
        ++idx;
    }
    // The end of the string is terminated properly
    if (row.render[idx] == c)
    {
        row.highlight[idx] = HL_STRING;
    }
    // The end of the string is missing a terminator
    else
    {
        row.highlight[idx] = HL_ERROR;
    }
    ++idx;
}

void SyntaxEngine::processNumber(std::size_t &idx, Model::erow &row) const
{
    while (idx < row.highlight.size() && (isdigit(row.render[idx]) || row.render[idx] == '.'))
    {
        row.highlight[idx] = HL_NUMBER;
        ++idx;
    }
}

void SyntaxEngine::processSeparator(std::size_t &idx, Model::erow &row) const
{
    while (idx < row.render.size() && isSeparator(row.render[idx]))
    {
        ++idx;
    }
}

void SyntaxEngine::processMultilineComment(std::size_t &idx, bool &inComment, Model::erow &row) const
{
    while (idx < row.highlight.size() && inComment)
    {
        if (idx <= row.render.size() - multilineCommentEnd.size() &&
            row.render.compare(idx, multilineCommentEnd.size(), multilineCommentEnd) == 0)
        {
            std::fill_n(row.highlight.begin() + idx, multilineCommentEnd.size(), HL_MLCOMMENT);
            idx += multilineCommentEnd.size();
            inComment = false;
        }
        else
        {
            row.highlight[idx] = HL_MLCOMMENT;
            ++idx;
        }
    }
}

void SyntaxEngine::processWord(std::size_t &idx, Model::erow &row) const
{
    std::string word = "";
    while (idx < row.highlight.size() && !isSeparator(row.render[idx]))
    {
        word += row.render[idx];
        ++idx;
    }
    auto hlVal = termTrie.get(word);
    if (hlVal != -1)
    {
        std::fill_n(row.highlight.begin() + idx - word.size(), word.size(), hlVal);
    }
    else if (idx < row.highlight.size() && row.render[idx] == '(')
    {
        std::fill_n(row.highlight.begin() + idx - word.size(), word.size(), HL_FUNCTION);
    }
}
