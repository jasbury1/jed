#include <string>
#include <vector>

#include "CSyntax.h"
#include "Model.h"

CSyntax::CSyntax()
{
}

CSyntax::~CSyntax()
{
}

bool isSeparator(int c)
{
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[]{};", c) != NULL;
}

void CSyntax::updateSyntaxHighlight(std::vector<Model::erow> &rowList, std::size_t rowIndex) const
{
    if (rowIndex >= rowList.size() || rowIndex < 0)
    {
        return;
    }
    auto &curRow = rowList[rowIndex];
    std::fill(curRow.highlight.begin(), curRow.highlight.end(), HL_NORMAL);

    // Keeps track of if the previous character was a separator
    bool prevSep = true;
    // Keeps track of if we are currently in a string
    bool inString = false;
    // Keep track of whether a string uses ' or ""
    char stringType = '\0';
    // Keeps track of if we are currently in a comment
    bool inComment = (curRow.idx > 0 && rowList[curRow.idx - 1].commentOpen);

    std::size_t i = 0;
    while (i < curRow.highlight.size())
    {
        auto c = curRow.render[i];
        auto prevHl = (i > 0) ? curRow.highlight[i - 1] : HL_NORMAL;

        if(!inComment && c == '#') {
            std::fill_n(curRow.highlight.begin() + i, curRow.highlight.size() - i, HL_KEYWORD1);
        }

        // If we aren't in a comment or a string, look for a comment start
        if (!singlelineComment.empty() && !inString && !inComment)
        {
            if (curRow.render.compare(i, singlelineComment.size(), singlelineComment) == 0)
            {
                std::fill_n(curRow.highlight.begin() + i, curRow.highlight.size() - i, HL_COMMENT);
                break;
            }
        }

        // If we aren't in a string, look for multi-line commenting
        if (!multilineCommentStart.empty() && !multilineCommentEnd.empty() && !inString)
        {
            // We are currently in a comment. Look for the comment end
            if (inComment)
            {
                curRow.highlight[i] = HL_MLCOMMENT;
                if (curRow.render.compare(i, multilineCommentEnd.size(), multilineCommentEnd) == 0)
                {
                    std::fill_n(curRow.highlight.begin() + i, multilineCommentEnd.size(), HL_MLCOMMENT);
                    i += multilineCommentEnd.size();
                    inComment = false;
                    prevSep = true;
                    continue;
                }
                else
                {
                    i++;
                    continue;
                }
            }
            // We are not in a comment. Look for a comment start
            else if (curRow.render.compare(i, multilineCommentStart.size(), multilineCommentStart) == 0)
            {
                std::fill_n(curRow.highlight.begin() + i, multilineCommentStart.size(), HL_MLCOMMENT);
                i += multilineCommentStart.size();
                inComment = true;
                continue;
            }
        }

        if (highlightStrings())
        {
            if (inString)
            {
                curRow.highlight[i] = HL_STRING;
                if (c == '\\' && i + 1 < curRow.render.size())
                {
                    curRow.highlight[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                if (c == stringType)
                    inString = false;
                i++;
                prevSep = true;
                continue;
            }
            else
            {
                if (c == '"' || c == '\'')
                {
                    inString = true;
                    stringType = c;
                    curRow.highlight[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        if (highlightNums())
        {
            if ((isdigit(c) && (prevSep || prevHl == HL_NUMBER)) ||
                (c == '.' && prevHl == HL_NUMBER))
            {
                curRow.highlight[i] = HL_NUMBER;
                i++;
                prevSep = false;
                continue;
            }
        }

        bool keywordUsed = false;
        if (prevSep)
        {
            for (auto &keyword : keywords)
            {
                if (i + keyword.size() < curRow.render.size() &&
                    curRow.render.compare(i, keyword.size(), keyword) == 0 &&
                    isSeparator(curRow.render[i + keyword.size()]))
                {
                    std::fill_n(curRow.highlight.begin() + i, keyword.size(), HL_KEYWORD1);
                    i += keyword.size();
                    keywordUsed = true;
                    break;
                }
            }
        }

        if (prevSep && !keywordUsed)
        {
            for (auto &type : types)
            {
                if (i + type.size() < curRow.render.size() &&
                    curRow.render.compare(i, type.size(), type) == 0 &&
                    isSeparator(curRow.render[i + type.size()]))
                {
                    std::fill_n(curRow.highlight.begin() + i, type.size(), HL_KEYWORD2);
                    i += type.size();
                    keywordUsed = true;
                    break;
                }
            }
        }
        if (keywordUsed)
        {
            prevSep = false;
            continue;
        }

        prevSep = isSeparator(c);
        i++;
    }

    bool statusChanged = (curRow.commentOpen != inComment);
    curRow.commentOpen = inComment;
    if (statusChanged && curRow.idx + 1 < rowList.size())
    {
        updateSyntaxHighlight(rowList, rowIndex + 1);
    }
}